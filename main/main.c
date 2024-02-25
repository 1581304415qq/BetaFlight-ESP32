#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "init.h"

void run(void) {}

void app_main(void)
{
    printf("betafliht esp32!\n");

    init();

    run();

    return;
}
