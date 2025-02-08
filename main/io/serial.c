#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_err.h"
#include "string.h"

#define TASK_STACK_SIZE 1025*5

static RingbufHandle_t ringBufHandle;

void sendTask(void* param)
{
    size_t itemSize = 0;
    for (;;) {
        char buffer[1024] = { 0 };
        char* item = (char*)xRingbufferReceive(ringBufHandle, &itemSize, pdMS_TO_TICKS(1000));
        if (item != NULL) {
            memcpy(buffer, item, itemSize);
            printf("%s", buffer);
            vRingbufferReturnItem(ringBufHandle, item);
        }
        else {
            vTaskDelay(200);
        }

    }
}

void serialInit()
{
    ringBufHandle = xRingbufferCreate(1024, RINGBUF_TYPE_BYTEBUF);

    xTaskCreate(sendTask, "uart_send_task", TASK_STACK_SIZE, NULL, 8, NULL);

}

esp_err_t serialWrite(const char* buffer, uint32_t len)
{
    UBaseType_t res = xRingbufferSend(ringBufHandle, buffer, len, pdMS_TO_TICKS(1000));
    return ESP_OK;
}
