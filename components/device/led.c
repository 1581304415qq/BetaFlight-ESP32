#include "led.h"
#include "led_driver.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

enum LED_ID {
    LED_WIFI,
    LED_SYS,
    LED_GRN,
    LED_COUNT,
};

static led_config leds[] = {
    {CONFIG_ESP_WIFI_LED_PIN, GPIO_MODE_OUTPUT},
    {CONFIG_ESP_RED_LED_PIN, GPIO_MODE_OUTPUT},
    {CONFIG_ESP_GRN_LED_PIN, GPIO_MODE_OUTPUT}
};

void led_init()
{
    init_led(leds, LED_COUNT);
}

void led_launch()
{
    led_set_state(LED_SYS, LOW_BLINK);
}

void led_landing()
{
    led_set_state(LED_SYS, HEIGHT_BLINK);
}

void led_cruise()
{
    led_set_state(LED_SYS, SLEEP_Q_BLINK);
}

void led_warning()
{
    led_set_state(LED_SYS, SHARP_BLINK);
}