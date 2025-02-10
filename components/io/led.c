#include "led.h"
#include "led_driver.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

#define LED_COUNT 1
static led_config leds[] = { {CONFIG_ESP_LED_PIN, GPIO_MODE_OUTPUT} };

void led_init()
{
    init_led(leds, LED_COUNT);
}