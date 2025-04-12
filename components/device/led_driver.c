#include "led_driver.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdbool.h>

#define TAG "led-driver"


#define GPIO_INIT(SEL, MOD) { \
    gpio_config_t io_conf = {}; \
    io_conf.intr_type = GPIO_INTR_DISABLE;\
    io_conf.mode = MOD;\
    io_conf.pin_bit_mask = SEL;\
    io_conf.pull_up_en = 0;\
    io_conf.pull_down_en = 0;\
    gpio_config(&io_conf);\
}while (0)

#define GPIO_ON(p)         gpio_set_level((p), 1);
#define GPIO_OFF(p)        gpio_set_level((p), 0);


#define TIMER_PERIOD 100*1000
static int led_count = 0;
static led_t** leds = NULL;
static esp_timer_handle_t periodic_timer; // 定时器
static bool isStart = false;

bool led_test()
{
    return isStart;
}

static void led_on(led_t* led)
{
    // printf("led turn on %d\n",led->led_cnt);
    if (led && led->on_off == 0)
    {
        GPIO_ON(led->led_pin);
        led->on_off = 1;
    }
}

static void led_off(led_t* led)
{
    if (led && led->on_off)
    {
        GPIO_OFF(led->led_pin);
        led->on_off = 0;
    }
}

static void led_off_all()
{
    if (leds == NULL)return;
    for (int i = 0; i < led_count; i++)
        if (leds[i] != NULL)
            led_off(leds[i]);
}

// 定时器用于led闪烁
static void timer_100ms_callback(void* arg)
{
    led_t* led;
    for (int i = 0; i < led_count; i++)
    {
        led = leds[i];
        if (NULL == led) continue;
        // 亮灭处理
        if (led->state == OFF)
        {
            led_off(led);
        }
        else if (led->state == ON)
        {
            led_on(led);
        }
        else if (led->state > ON)
        {
            // 开的时间
            if (led->led_cnt >= led->on_time && led->on_off)
            {
                led_off(led);
                led->led_cnt = 0;
            }
            // 关的时间
            if (led->led_cnt >= led->off_time && !led->on_off)
            {
                led_on(led);
                led->led_cnt = 0;
            }
            // 时间计数
            led->led_cnt++;
        }
    }
}

static void start_timer(void)
{
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TIMER_PERIOD));
}

static void stop_timer(void)
{
    esp_timer_stop(periodic_timer);
    led_off_all();
}

/**
 * @brief Create a led object
 *
 * @param p gpio pin index
 * @param m 模式 参考GPIO功能复用表
 * @return led_t*
 */
static led_t* create_led(uint16_t id, gpio_num_t pin, gpio_mode_t mode)
{
    led_t* led = malloc(sizeof(led_t));
    led->id = id;
    led->led_pin = pin;
    led->on_off = 0;
    led->led_cnt = 0;
    led->on_time = 0;
    led->off_time = 0;
    led->state = 0;
    leds[id] = led;
    return led;
}

static void destroy_led(led_t* led)
{
    if (led == NULL)return;
    led_off(led);
    leds[led->id] = NULL;
    free(led);
}

/**
 * @brief 设置led闪烁频率，一个周期为10ms
 *
 * @param led led
 * @param on_time 打开时间长度  单位为100ms
 * @param off_time 关闭时间长度 单位为100ms
 */
static void led_flash_set(led_t* led, uint16_t on_time, uint16_t off_time)
{
    if (led == NULL)return;
    led->led_cnt = 0;
    led->on_time = on_time;
    led->off_time = off_time;
}

static void led_open(led_t* led)
{
    if (led == NULL)return;
    led->state = ON;
}

static void led_close(led_t* led)
{
    if (led == NULL)return;
    led->state = OFF;
}

static void led_mode(led_t* led, enum LED_Sta state)
{
    if (led == NULL || led->state == state)return;
    led->state = state;
    switch (state)
    {
    case OFF:
        led_close(led);
        break;
    case ON:
        led_open(led);
        break;
    case QUICK_BLINK:
        led_flash_set(led, 1, 5);
        break;
    case SHARP_BLINK:
        led_flash_set(led, 2, 2);
        break;
    case HEIGHT_BLINK:
        led_flash_set(led, 5, 5);
        break;
    case LOW_BLINK:
        led_flash_set(led, 10, 10);
        break;
    case SLEEP_BLINK:
        led_flash_set(led, 30, 30);
        break;
    case SLEEP_Q_BLINK:
        led_flash_set(led, 1, 30);
        break;
    default:
        break;
    }
}

static void led_flash_set_duty(led_t* led, float duty)
{
    led_flash_set(led, TIMER_PERIOD * duty, TIMER_PERIOD * (1 - duty));
}

/*
    占空比设置led亮灭时间
*/
static void led_flash_set_duty_on(led_t* led, uint8_t interval, float duty)
{
    led_flash_set(led, TIMER_PERIOD * interval * duty, TIMER_PERIOD * interval * (1 - duty));
}

void init_led(led_config* led_conf, int count)
{
    ESP_LOGI(TAG, "leds inited\n");
    if (isStart) return;

    const esp_timer_create_args_t c_periodic_timer_args = {
            .callback = &timer_100ms_callback,
            .name = "periodic"
    };
    ESP_ERROR_CHECK(esp_timer_create(&c_periodic_timer_args, &periodic_timer));

    led_count = count;
    leds = malloc(sizeof(led_t*) * count);

    uint64_t gpio_mask = 0;

    for (int i = 0; i < count; i++) {
        create_led(i, led_conf[i].pin, led_conf[i].mode);
        GPIO_INIT(1ULL << led_conf[i].pin, led_conf[i].mode);
    }
    start_timer();
    isStart = true;
}

void deinit_led()
{
    ESP_LOGI(TAG, "leds deinited\n");
    if (!isStart) return;

    stop_timer();
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    for (int i = 0; i < led_count; i++)
        destroy_led(leds[i]);
    free(leds);
    led_count = 0;
    isStart = false;
}

void led_set_state(uint16_t id, enum LED_Sta sta)
{
    if (leds[id] != NULL)
        led_mode(leds[id], sta);
}


void led_open_all()
{
    for (int i = 0; i < led_count; i++)
        if (leds[i] != NULL)
            led_open(leds[i]);
}

void led_close_all()
{
    for (int i = 0; i < led_count; i++)
        if (leds[i] != NULL)
            led_close(leds[i]);
}
