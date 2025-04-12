#include "led.h"
#include "led_driver.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

enum LED_ID {
    LED_SYS,
    LED_COUNT,
};

static led_config leds[] = { {CONFIG_ESP_LED_PIN, GPIO_MODE_OUTPUT} };

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