#include "http_server.h"
#include "http_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "storage.h"
#include "esp_random.h"
#include "cJSON.h"
#include <time.h>
#include "parse_part.h"
#include "ota.h"

#include "station.h"

#define TAG "HTTP SERVER"

#define PORT 80
#define TIMEOUT 5
#define MAX_URI_HANDLERS 15

/* Scratch buffer size */
#define ESP_VFS_PATH_MAX 15
#define SCRATCH_BUFSIZE  1024
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];
    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};


static bool isStart = false;
bool http_test(void)
{
    return isStart;
}

static struct file_server_data* server_data = NULL;
static httpd_handle_t server = NULL;

static void api_event_handler(char* id, char* query, char* content);

static esp_err_t _404_not_found(httpd_req_t* req)
{
    printf("not found\n");
    uint32_t ret = 0;
    struct file_server_data* context = (struct file_server_data*)req->user_ctx;
    char path[128];
    ret = snprintf(path, sizeof(path), "%s/%s", context->base_path, "404.html");
    char* buff = get_resouce(read_file, path, &ret);
    if (ret > 0) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        httpd_resp_send(req, (const char*)buff, ret);
        free(buff);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t favicon_get_handler(httpd_req_t* req)
{
    uint32_t favicon_ico_size = 0;
    struct file_server_data* context = (struct file_server_data*)req->user_ctx;
    char path[128];
    int ret = snprintf(path, sizeof(path), "%s/%s", context->base_path, "favicon.ico");
    char* favicon_ico = get_resouce(read_file, path, &favicon_ico_size);
    if (favicon_ico == NULL) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    else {
        httpd_resp_set_type(req, "image/x-icon");
        httpd_resp_send(req, (const char*)favicon_ico, favicon_ico_size);
        free(favicon_ico);
        return ESP_OK;
    }
}


#define SESSION_ID_LEN 16
#define SESSION_TIMEOUT 1800 // 30分钟（秒）
#define MAX_UAERNAME_LEN 32

typedef struct session {
    char id[SESSION_ID_LEN + 1];
    char username[MAX_UAERNAME_LEN];
    time_t expires;
    struct session* next;
} session_t;

static session_t* sessions = NULL;
static SemaphoreHandle_t session_mutex = NULL;

// 生成随机Session ID
static void generate_session_id(char* session_id) {
    uint32_t rnd = esp_random();
    snprintf(session_id, SESSION_ID_LEN + 1, "%08lx", rnd);
}
// 添加新Session
static void add_session(const char* username) {
    session_t* new_session = malloc(sizeof(session_t));
    generate_session_id(new_session->id);
    strlcpy(new_session->username, username, sizeof(new_session->username));
    new_session->expires = time(NULL) + SESSION_TIMEOUT;

    xSemaphoreTake(session_mutex, portMAX_DELAY);
    new_session->next = sessions;
    sessions = new_session;
    xSemaphoreGive(session_mutex);
}
// 查找有效Session
static session_t* find_valid_session(const char* id) {
    time_t now = time(NULL);
    session_t** ptr = &sessions;

    xSemaphoreTake(session_mutex, portMAX_DELAY);
    while (*ptr) {
        if (strcmp((*ptr)->id, id) == 0) {
            if ((*ptr)->expires > now) {
                (*ptr)->expires = now + SESSION_TIMEOUT; // 续期
                session_t* found = *ptr;
                xSemaphoreGive(session_mutex);
                return found;
            }
            else {
                // 移除过期Session
                session_t* temp = *ptr;
                *ptr = (*ptr)->next;
                free(temp);
            }
        }
        else {
            ptr = &(*ptr)->next;
        }
    }
    xSemaphoreGive(session_mutex);
    return NULL;
}
// 删除Session
static void remove_session(const char* id) {
    xSemaphoreTake(session_mutex, portMAX_DELAY);
    session_t** ptr = &sessions;
    while (*ptr) {
        if (strcmp((*ptr)->id, id) == 0) {
            session_t* temp = *ptr;
            *ptr = (*ptr)->next;
            free(temp);
            break;
        }
        ptr = &(*ptr)->next;
    }
    xSemaphoreGive(session_mutex);
}
// 从Cookie中获取Session ID
static char* get_session_id_from_cookie(httpd_req_t* req) {
    char cookie_header[128];
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie_header, sizeof(cookie_header)) != ESP_OK) {
        return NULL;
    }

    char* session_start = strstr(cookie_header, "session_id=");
    if (!session_start) return NULL;

    session_start += 11; // 跳过"session_id="
    char* session_end = strchr(session_start, ';');
    if (session_end) {
        *session_end = '\0';
    }
    return strdup(session_start);
}


// 检查请求中是否包含有效的会话ID，并验证会话的有效性
static bool validate_session(httpd_req_t* req) {
    char* session_id = get_session_id_from_cookie(req);
    if (!session_id) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }

    session_t* session = find_valid_session(session_id);
    free(session_id);

    if (!session) return false;
    else return true;
}

static esp_err_t root_get_handler(httpd_req_t* req)
{
    struct file_server_data* context = (struct file_server_data*)req->user_ctx;
    char path[128];
    int ret = snprintf(path, sizeof(path), "%s/%s", context->base_path, "login.html");
    uint32_t login_html_size = 0;
    char* login_html = get_resouce(read_file, path, &login_html_size);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, login_html, strlen(login_html));
}

static esp_err_t api_handler(httpd_req_t* req)
{
    bool verify_session = validate_session(req);
    if (!verify_session)
    {
        ESP_LOGW(TAG, "illegal access");
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }


    char filepath[FILE_PATH_MAX] = { 0 };
    const char* id = get_path_from_uri(filepath, ((struct file_server_data*)req->user_ctx)->base_path,
        req->uri + sizeof("/api") - 1, sizeof(filepath));
    printf("%s get api id=%s, filepath=%s\n", TAG, id, filepath);

    // 查询参数缓冲区
    char query_buffer[100] = { 0 };

    // 获取查询字符串长度
    size_t query_len = httpd_req_get_url_query_len(req);

    if (query_len > 0) {
        // 确保缓冲区大小足够
        if (query_len >= sizeof(query_buffer)) {
            ESP_LOGE(TAG, "Query string too long");
            return ESP_FAIL;
        }

        // 获取完整的查询字符串
        if (httpd_req_get_url_query_str(req, query_buffer, sizeof(query_buffer)) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get query string");
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Full Query String: %s", query_buffer);
    }

    // POST数据
    char content[256];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    // 读取请求体
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        // 处理接收错误
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            // 超时错误处理
            // httpd_resp_send_408(req);
        }
        // return ESP_FAIL;
    }
    else {
        // 确保字符串以 null 结尾
        content[ret] = '\0';
        ESP_LOGD(TAG, "Recv %s", content);
    }

    // 分发请求处理
    api_event_handler(id, query_buffer, content);

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 主页处理
static esp_err_t home_get_handler(httpd_req_t* req) {
    char* session_id = get_session_id_from_cookie(req);
    if (!session_id) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }

    session_t* session = find_valid_session(session_id);
    free(session_id);

    if (!session) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }

    struct file_server_data* context = (struct file_server_data*)req->user_ctx;
    char path[128];
    int ret = snprintf(path, sizeof(path), "%s/%s", context->base_path, "index.html");
    uint32_t buf_len = 0;
    char* home_html = get_resouce(read_file, path, &buf_len);
    if (buf_len > 0) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        ret = httpd_resp_send(req, home_html, buf_len);
        free(home_html);
    }
    else ret = ESP_FAIL;
    return ret;
}

// 登录POST处理
static esp_err_t login_post_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "content len=%d", req->content_len);

    char content[256];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';
    ESP_LOGI(TAG, "%s", content);
    // 简单验证（实际应使用安全验证）
    char* username = strstr(content, "username=");
    char* password = strstr(content, "password=");
    if (!username || !password) {
        return httpd_resp_send_404(req);
    }

    username += 9; // 跳过"username="
    password += 9; // 跳过"password="
    char* end_user = strchr(username, '&');
    if (end_user) *end_user = '\0';
    char* end_pass = strchr(password, '&');
    if (end_pass) *end_pass = '\0';

    // 示例验证（替换为实际验证逻辑）
    if (strcmp(username, "admin") != 0 || strcmp(password, "1234") != 0) {
        httpd_resp_set_status(req, "401 Unauthorized");
        return httpd_resp_send(req, "Invalid credentials", HTTPD_RESP_USE_STRLEN);
    }

    if (strlen(username) > MAX_UAERNAME_LEN) {
        httpd_resp_set_status(req, "401 Unauthorized");
        return httpd_resp_send(req, "Invalid credentials", HTTPD_RESP_USE_STRLEN);
    }
    // 创建Session
    add_session(username);

    // 设置Cookie并重定向
    session_t* session = sessions;
    char header[256];
    snprintf(header, sizeof(header),
        "Location: /home\r\n"
        "Set-Cookie: session_id=%s; Path=/; HttpOnly\r\n",  // 注意结尾的 \r\n
        session->id);
    // 设置HTTP状态码和头部
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/home");
    httpd_resp_set_hdr(req, "Set-Cookie", header + sizeof("Location: /home\r\n") - 1);

    // 发送空响应体完成重定向
    return httpd_resp_send(req, NULL, 0);
}

// 注销处理
static esp_err_t logout_get_handler(httpd_req_t* req) {
    char* session_id = get_session_id_from_cookie(req);
    if (session_id) {
        remove_session(session_id);
        free(session_id);
    }

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_set_hdr(req, "Set-Cookie", "session_id=; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    return httpd_resp_send(req, NULL, 0);
}

#define MAX_FILE_SIZE   (2*1024*1024)
#define MAX_FILE_SIZE_STR "1MB"

static void void_function(void) {}
#define read_header_value void_function

static void read_header_name(char* name)
{
    ota_ready(0);
}

static void data_handle(struct parser* p, char* str, size_t len)
{
    if (len > 0) {
        ota(str, len);
        p->recv_count += len;
        printf("data handle len %u count %u\n", len, p->recv_count);
    }
    else
        ESP_LOGE(TAG, "File write failed!");

}
static void complect_handle(struct parser* p)
{
    ESP_LOGI(TAG, "http recv %d data\n", p->recv_count);
    ota_end(true);
}

static esp_err_t update_handle(httpd_req_t* req)
{
    bool verify_session = validate_session(req);
    if (!verify_session)
    {
        ESP_LOGW(TAG, "illegal access");
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }

    // size检查
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "File size must be less than "
            MAX_FILE_SIZE_STR "!");
        return ESP_FAIL;
    }

    int content_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
    char* content_type = NULL, * boundary = NULL;
    if (content_len > 1) {
        content_type = malloc(content_len);
        ESP_RETURN_ON_FALSE(content_type, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, content_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Content-Type: %s", content_type);
            boundary = strstr(content_type, "boundary=");
            if (boundary == NULL) {
                free(content_type);
                return ESP_FAIL;
            }
            boundary += strlen("boundary=");
        }
    }
    else {
        return ESP_FAIL;
    }
    printf(LOG_FMT("boundray=%s"), boundary);

    struct parser parser;
    parser_init(
        &parser,
        boundary,
        SCRATCH_BUFSIZE,
        read_header_name,
        read_header_value,
        data_handle,
        complect_handle);


    free(content_type);
    char* buf = ((struct file_server_data*)req->user_ctx)->scratch;
    int received;
    int remaining = req->content_len;
    parser.content_length = req->content_len;

    while (remaining > 0) {
        ESP_LOGD(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        // printf("\033[33m%.*s\033[0m", received, buf);
        parse_part(&parser, buf, received);

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }
    ESP_LOGI(TAG, "Update file reception complete");
    httpd_resp_sendstr(req, "File uploaded successfully");

    parser_free(&parser);
    return ESP_OK;
}


#define REGISTER_HANDLER(method_p, uri_p, handler_p)   do { httpd_uri_t httpd_uri = { \
.uri = uri_p,                                                \
.method = method_p,                                          \
.handler = handler_p,                                       \
.user_ctx = server_data                                    \
};                                                         \
httpd_register_uri_handler(server, &httpd_uri);            \
} while (0)
static void register_handler(void)
{
    REGISTER_HANDLER(HTTP_GET, "/favicon.ico", favicon_get_handler);
    REGISTER_HANDLER(HTTP_GET, "/", root_get_handler);
    REGISTER_HANDLER(HTTP_GET, "/api/*", api_handler);
    REGISTER_HANDLER(HTTP_POST, "/api/*", api_handler);
    REGISTER_HANDLER(HTTP_POST, "/update", update_handle);

    REGISTER_HANDLER(HTTP_POST, "/login", login_post_handler);
    REGISTER_HANDLER(HTTP_GET, "/logout", logout_get_handler);
    REGISTER_HANDLER(HTTP_GET, "/home", home_get_handler);

}

esp_err_t http_server_init(const char* base_path)
{
    if (server_data) {
        ESP_LOGE(TAG, "File server already inited");
        return ESP_ERR_INVALID_STATE;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path,
        sizeof(server_data->base_path));
    session_mutex = xSemaphoreCreateMutex();
    return ESP_OK;
}

void http_server_deinit()
{
    free(server_data);
    server_data = NULL;
    vSemaphoreDelete(session_mutex);
}

esp_err_t http_server_start(void)
{
    if (isStart) return -1;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_resp_headers = 2048;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = MAX_URI_HANDLERS;
    config.stack_size = 10 * 1024;
    config.ctrl_port = 32760;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start http server! err=%d", ret);
        return ESP_FAIL;
    }
    register_handler();
    isStart = true;
    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    return ESP_OK;
}

void http_server_stop(void)
{
    if (!isStart)return;
    ESP_LOGI(TAG, "Stoping HTTP Server");
    httpd_stop(&server);
    isStart = false;
    return;
}

static void api_event_handler(char* id, char* query, char* content)
{
    ESP_LOGI(TAG, "[%s] id=%s ,query=%s ,content=%s", __FUNCTION__, id, query, content);
    if (!strcmp(id, "/get-device-status")) {
        // 解析查询参数
        char param[32];
        if (httpd_query_key_value(query, "value", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(TAG, "value参数值: %s", param);
        }
        // bool camera_enable = camera_test();
        // bool bluetooth_enable = bluetooth_test();
        // bool wifi_enable = wifi_test();
        // char* ssid = wifi_get_ssid();
        // char* password = wifi_get_password();

#define status_fmt "{\"bluetooth\":{\"enabled\":%s},\"wifi\":{\"enabled\":%s,\"ssid\":\"%s\",\"password\":\"%s\"},\"camera\":{\"enabled\":%s,\"mode\":\"%s\"}}"
        // char* status[512];
        // int ret = snprintf(status, sizeof(status), status_fmt,
        //     bluetooth_enable ? "true" : "false",
        //     wifi_enable ? "true" : "false",
        //     ssid, password,
        //     camera_enable ? "true" : "false", "HD"
        // );

    }
    else if (!strcmp(id, "/apply-settings")) {
        cJSON* root = cJSON_Parse(content);
        if (root == NULL)return;

        cJSON* bluetooth = cJSON_GetObjectItem(root, "bluetooth");
        if (bluetooth != NULL) {
            cJSON* enabled = cJSON_GetObjectItem(bluetooth, "enabled");
            // if (enabled && cJSON_IsTrue(enabled))
            //     bluetooth_start();
            // else
            //     bluetooth_stop();
        }

        cJSON* wifi = cJSON_GetObjectItem(root, "wifi");
        if (wifi != NULL) {
            cJSON* enabled = cJSON_GetObjectItem(wifi, "enabled");
            // if (enabled && cJSON_IsTrue(enabled))
            //     wifi_start_sta();
            // else
            //     wifi_stop_sta();

            cJSON* ssid = cJSON_GetObjectItem(wifi, "ssid");
            cJSON* password = cJSON_GetObjectItem(wifi, "password");
            if (ssid && strlen(ssid->valuestring) > 0 &&
                password && strlen(password->valuestring) > 0) {
                pullux_wifi_set_ssid_passwd(ssid->valuestring, password->valuestring);
            }
        }

        cJSON* camera = cJSON_GetObjectItem(root, "camera");
        if (camera != NULL) {
            cJSON* enabled = cJSON_GetObjectItem(camera, "enabled");
            // if (enabled && cJSON_IsTrue(enabled)) {
            //     int camera_ret = camera_start();
            // }
            // else
                // camera_stop();

            cJSON* mode = cJSON_GetObjectItem(camera, "mode");

            // if (mode != NULL) {
            //     if (!strcmp(mode->valuestring, "SD")) {}
            //     else if (!strcmp(mode->valuestring, "HD")) {}
            //     else if (!strcmp(mode->valuestring, "FHD")) {}
            //     else {}
            // }
        }


        cJSON* display = cJSON_GetObjectItem(root, "display");
        if (display != NULL) {
            cJSON* offset_x = cJSON_GetObjectItem(display, "offset-x");
            cJSON* offset_y = cJSON_GetObjectItem(display, "offset-y");
            if (offset_x == NULL || offset_y == NULL)
            {
                ESP_LOGI(TAG, "display set error.");
                return;
            }

        }

        cJSON_Delete(root);
    }

}




