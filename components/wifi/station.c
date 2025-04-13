#include "station.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "lwip/raw.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/prot/tcp.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include "lwip/dhcp.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "storage.h"

static const char* TAG = "WIFI";

#define WIFI_SSID_KEY      "wifi-ssid"
#define WIFI_PASSWORD_KEY  "wifi-password"
#define WIFI_SSID      CONFIG_STA_WIFI_SSID
#define WIFI_PASS      CONFIG_STA_WIFI_PASSWORD
#define MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define HOSTNAME       CONFIG_AP_WIFI_SSID

#define AP_WIFI_SSID CONFIG_AP_WIFI_SSID
#define AP_WIFI_PASSWORDD CONFIG_AP_WIFI_PASSWORD
#define AP_WIFI_CHANNEL CONFIG_AP_WIFI_CHANNEL

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_AP_START      BIT2

static int s_retry_num = 0;
static int wifi_mode = 0;
static esp_netif_t* esp_netif = NULL;


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_AP_START);
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
            MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason:%d",
            MAC2STR(event->mac), event->aid, event->reason);
    }
}

/* Initialize wifi station */
esp_netif_t* wifi_init_sta(void)
{
    uint8_t ssid[32], password[64];
    if (storage_read(WIFI_SSID_KEY, ssid, sizeof(ssid)) != 0 ||
        storage_read(WIFI_PASSWORD_KEY, password, sizeof(password)) != 0)
    {
        return NULL;
    }

    esp_netif_t* esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
            * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    memcpy(wifi_sta_config.sta.ssid, ssid, sizeof(ssid));
    memcpy(wifi_sta_config.sta.password, password, sizeof(password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return esp_netif_sta;
}


/* Initialize soft AP */
esp_netif_t* wifi_init_softap(void)
{
    esp_netif_t* esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = AP_WIFI_SSID,
            .ssid_len = strlen(AP_WIFI_SSID),
            .channel = AP_WIFI_CHANNEL,
            .password = AP_WIFI_PASSWORDD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(AP_WIFI_PASSWORDD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
        AP_WIFI_SSID, AP_WIFI_PASSWORDD, AP_WIFI_CHANNEL);

    // 设置AP的IP地址等信息
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 1);  // AP的IP地址
    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);  // 网关地址
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);  // 子网掩码
    esp_netif_dhcps_stop(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));
    esp_netif_set_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    esp_netif_dhcps_start(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));

    return esp_netif_ap;
}

void pullux_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_wifi_event_group = xEventGroupCreate();

    // 注册WiFi事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Initialize STA */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_netif = wifi_init_sta();
    if (esp_netif != NULL) {
        // 设置主机名
        ESP_ERROR_CHECK(esp_netif_set_hostname(esp_netif, HOSTNAME));

        ESP_ERROR_CHECK(esp_wifi_start());
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
        if (bits & WIFI_CONNECTED_BIT) {
            // 成功：执行网络操作
            ESP_LOGI(TAG, "Successfully connected to Wi-Fi.");
            wifi_mode = 1;
            return;
        }
        else if (bits & WIFI_FAIL_BIT) {
            // 失败：处理重连或错误
            ESP_LOGI(TAG, "Failed to connect to Wi-Fi.");
            esp_wifi_stop();
        }
        else {
            // 超时处理
            ESP_LOGE(TAG, "Wifi start up error.");
            esp_wifi_stop();
        }
    }

    /* Initialize AP */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    esp_netif = wifi_init_softap();
    if (esp_netif != NULL) {
        ESP_ERROR_CHECK(esp_wifi_start());
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_FAIL_BIT | WIFI_AP_START,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
        if (bits & WIFI_AP_START) {
            // 成功：启动AP
            ESP_LOGI(TAG, "AP started successfully.");
            wifi_mode = 2;
        }
        else {
            // 超时处理
            ESP_LOGE(TAG, "Wifi start up error.");
            esp_wifi_stop();
        }
    }
}

void pullux_wifi_deinit(void)
{
    if (wifi_mode == 1) {}
    else if (wifi_mode == 2) {
        ESP_ERROR_CHECK(esp_wifi_stop());
        if (esp_netif)esp_netif_destroy_default_wifi(esp_netif);
    }
}

void pullux_wifi_set_max_tx_power(int8_t power) {
    esp_wifi_set_max_tx_power(power);
}

void pullux_wifi_set_ssid_passwd(const char* ssid, const char* password)
{
    if (strlen(ssid) > 0 && strlen(password) > 0) {
        storage_write(WIFI_SSID_KEY, ssid, strlen(ssid));
        storage_write(WIFI_PASSWORD_KEY, password, strlen(password));
    }
}

void pullux_wifi_restart(void)
{
    esp_wifi_stop();
    esp_wifi_start();
}
