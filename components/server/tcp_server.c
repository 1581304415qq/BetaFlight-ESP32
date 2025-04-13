#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "storage.h"
#include "tcp_server.h"
#include "freertos/ringbuf.h"


#define SERVER_PORT_KEY "SERVER_PORT_KEY"
#define MAX_CLIENTS 5            // 最大客户端连接数
#define BUFFER_SIZE 1024         // 接收缓冲区大小

static const char* TAG = "TCP_SERVER";
static uint16_t port = 12345;          // 服务器监听的端口

#define RINGBUF_SIZE 4096        // 环形缓冲区大小
#define SEND_TASK_STACK 4096     // 发送任务堆栈大小

// 全局变量，用于控制服务器是否运行
static volatile bool server_running = false;
static TaskHandle_t tcp_server_task_handle = NULL;


// 发送任务函数
static void tcp_send_task(void* arg) {
    tcp_client_ctx_t* client = (tcp_client_ctx_t*)arg;
    uint8_t* data = NULL;
    size_t data_len = 0;

    while (1) {
        // 阻塞等待数据（最多等待 500ms）
        data = (uint8_t*)xRingbufferReceive(client->ringbuf, &data_len, pdMS_TO_TICKS(500));

        if (data != NULL) {
            // 发送数据到 TCP 客户端
            int sent = 0;
            while (sent < data_len) {
                int ret = send(client->sock, data + sent, data_len - sent, 0);
                if (ret <= 0) {
                    ESP_LOGE(TAG, "Send error: %d", errno);
                    break;
                }
                sent += ret;
            }

            // 释放环形缓冲区内存
            vRingbufferReturnItem(client->ringbuf, data);
        }

        // 检查 socket 是否有效
        if (client->sock < 0) {
            ESP_LOGI(TAG, "Socket closed, exiting send task");
            break;
        }
    }

    // 清理资源
    vRingbufferDelete(client->ringbuf);
    free(client);
    vTaskDelete(NULL);
}

// 处理客户端连接的任务
static void tcp_server_task(void* pvParameters)
{
    tcp_client_ctx_t* client = (tcp_client_ctx_t*)pvParameters;

    char buffer[BUFFER_SIZE];
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 创建服务器 Socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定 Socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }

    // 监听连接
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        ESP_LOGE(TAG, "Failed to listen on socket");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "TCP server started on port %d", port);

    client->ringbuf = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    // 创建发送任务
    xTaskCreate(tcp_send_task, "tcp_send", SEND_TASK_STACK, client, 5, &client->send_task);

    while (server_running) {
        // 接受客户端连接
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            ESP_LOGE(TAG, "Failed to accept client connection");
            continue;
        }

        ESP_LOGI(TAG, "Client connected: %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        client->sock = client_socket;
        client->is_connected = 1;
        // 处理客户端数据
        while (client->is_connected) {
            int len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Failed to receive data");
                client->is_connected = 0;
                break;
            }
            else if (len == 0) {
                ESP_LOGI(TAG, "Client disconnected");
                client->is_connected = 0;
                break;
            }
            else {
                // 处理正常数据
                buffer[len] = '\0';
                ESP_LOGI(TAG, "Received: %s", buffer);
            }
        }

        // 关闭客户端连接
        shutdown(client_socket, 0);
        close(client_socket);
        ESP_LOGI(TAG, "Client disconnected: %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    // 关闭服务器 Socket（通常不会执行到这里）
    close(server_socket);
    vTaskDelete(NULL);
}

tcp_client_ctx_t* start_server(void)
{
    if (storage_read(SERVER_PORT_KEY, &port, sizeof(port)) != 0) {
        port = 12345;
    }
    server_running = true;
    // 创建客户端上下文
    tcp_client_ctx_t* client = calloc(1, sizeof(tcp_client_ctx_t));
    client->is_connected = 0;
    // 启动 TCP 服务器任务
    if (xTaskCreate(tcp_server_task, "tcp_server", 4096, client, 5, &tcp_server_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "start erver fail");
    }

    ESP_LOGI(TAG, "TCP server task started");
    return client;
}

void stop_server()
{
    server_running = false;
}

void restart_server()
{
    stop_server();
    // 等待服务器任务结束
    while (eTaskGetState(tcp_server_task_handle) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(100)); // 每 100ms 检查一次
    }
    start_server();
}

int server_set_port(uint16_t p)
{
    port = p;
    if (storage_write(SERVER_PORT_KEY, &port, sizeof(port)) != 0) {
        ESP_LOGE(TAG, "save erver port fail");
    }
    return 0;
}

void server_send(tcp_client_ctx_t* client, const char* data, uint32_t len)
{
    if (!client->is_connected)
        return;

    BaseType_t ret = xRingbufferSend(client->ringbuf, data, len, pdMS_TO_TICKS(100));
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Ringbuffer full, data dropped");
    }
}
