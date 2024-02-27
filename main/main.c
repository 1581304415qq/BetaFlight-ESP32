#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "led.h"
#include "init.h"

void run(void) {}

void app_main(void)
{
    printf("betafliht esp32!\n");

    init();
    openLED();

    run();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    closeLED();

    return;
}
