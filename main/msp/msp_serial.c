#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <stdbool.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/soc_caps.h"
#include "msp_serial.h"
#include "msp.h"
#include "bluetooth.h"
#include "util.h"
#include "queue.h"

#define TAG "MSP Serial"

mspRegisterCallback registerFn = NULL;

static QueueHandle_t Qhandle;

void msgMessageHandle(msp_message_t message) {
    ESP_LOGI(TAG, "msp command=%d\n", message.command);

    if (NULL != registerFn) registerFn(message.header.protocol_version, message.command, message.payload, message.payload_size);
}

static void openSerial(void* arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        // .source_clk = UART_SCLK_APB,
        .rx_flow_ctrl_thresh = 122,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUFFER_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD, UART_RXD, UART_RTS, UART_CTS));
    ESP_ERROR_CHECK(uart_set_mode(UART_PORT_NUM, UART_MODE_UART));

    // Configure a temporary buffer for the incoming buffer
#define MAX_QUEUE_SIZE 300
    Queue* queue = createQueue(MAX_QUEUE_SIZE);
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    int length = 0;
    while (1) {
        // Read buffer from the UART
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(UART_PORT_NUM, buffer, (BUFFER_SIZE - 1), 100);

        ESP_LOGI(TAG, "Recv str: %d\n", length);
        if (length <= 0) {
            ESP_LOGE(TAG, "Recv empty");
            // BT_LOG("Recv empty");
            // DelayMs(200);
            continue;
        }
        for (int i = 0; i < length; i++)
        {
            enqueue(queue, buffer[i]);
        }

        sendByBt((const char*)buffer, length);

        static uint8_t payload[255];
        uint16_t command = 0, payload_len = 0;
        msp_message_t msp_message;
        int ret = parse_msp_packet2(queue, length, &msp_message.header, &command, &payload_len, payload);
        BT_LOG("parse msp ret=%d", ret);

        if (0 == ret) {
            msp_message.command = command;
            msp_message.payload_size = payload_len;
            msp_message.payload = payload;
            msgMessageHandle(msp_message);
        }

    }
}

void serialWrite(const uint8_t* data, size_t len) {
    uart_write_bytes(UART_PORT_NUM, (const char*)data, len);
}

void mspSerialInit(void) {
    Qhandle = xQueueCreate(300, sizeof(uint8_t)); // 创建一个队列
    xTaskCreate(openSerial, "uart_task", TASK_STACK_SIZE, NULL, 10, NULL);
}

void mspRegisterFn(mspRegisterCallback callback) {
    registerFn = callback;
}

void mspUnRegisterFn(mspRegisterCallback callback) {
    registerFn = NULL;
}