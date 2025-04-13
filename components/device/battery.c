
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "voltage.h"
#include "sdkconfig.h"
#include "battery.h"

static int timer_period = 1 * 1000;
static esp_timer_handle_t periodic_timer; // 定时器
static voltageMeter_t voltageMeter;

static callback_t battery_callback = NULL;
static void timer_callback(void* arg)
{
    voltageMeterADCRefresh();
    voltageMeterADCRead(VOLTAGE_SENSOR_ADC_VBAT, &voltageMeter);

    if (battery_callback)
        battery_callback(voltageMeter.displayFiltered);

}

void batteryInit(void) {
    voltageMeterADCInit();

    const esp_timer_create_args_t c_periodic_timer_args = {
            .callback = &timer_callback,
            .name = "periodic"
    };
    ESP_ERROR_CHECK(esp_timer_create(&c_periodic_timer_args, &periodic_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, timer_period));
}

void batteryDeinit(void) {
    esp_timer_stop(periodic_timer);
    esp_timer_delete(periodic_timer);
    voltageMeterADCDeinit();
}

void battery_register(callback_t callback) {
    battery_callback = callback;
}