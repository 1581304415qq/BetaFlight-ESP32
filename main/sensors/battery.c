
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pthread.h"

#include "battery.h"

void batteryInit(void) {
    sleep(1);
}