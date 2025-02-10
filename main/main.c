#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "init.h"

void run(void) {}

void app_main(void)
{
    printf("betaflight esp32!\n");

    init();

    run();

    while (1)
        vTaskDelay(2000 / portTICK_PERIOD_MS);



    return;
}
