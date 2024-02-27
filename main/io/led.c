#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "led.h"

#define LED_PIN GPIO_NUM_13

void openLED() {
    gpio_set_level(LED_PIN, 1);
}

void closeLED() {
    gpio_set_level(LED_PIN, 0);
}

void initLED() {

    gpio_reset_pin(LED_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

}