#pragma once
#include <stdint.h>

#include "freertos/ringbuf.h"
#include "freertos/task.h"

typedef struct {
    uint8_t is_connected;
    int sock;                    // 客户端 socket
    RingbufHandle_t ringbuf;     // 数据环形缓冲区
    TaskHandle_t send_task;      // 发送任务句柄
} tcp_client_ctx_t;

tcp_client_ctx_t* start_server(void);
void stop_server();
void restart_server();
int server_set_port(uint16_t p);

void server_send(tcp_client_ctx_t* client, const char* data, uint32_t len);


