#pragma once
#include <stdint.h>

enum LED_ID {
    LED_SYS,
    LED_COUNT,
};

enum LED_Sta {
    OFF,                // 长关
    ON,                 // 长开
    QUICK_BLINK,        // 短闪
    SHARP_BLINK,        // 200ms闪烁
    HEIGHT_BLINK,       // 500ms闪烁
    LOW_BLINK,          // 1s闪烁
    SLEEP_BLINK,        // 3s闪烁
    SLEEP_Q_BLINK,
};

typedef struct
{
    uint16_t led_cnt;
    uint16_t on_time;
    uint16_t off_time;
    uint8_t led_pin;
    uint8_t id;
    uint8_t on_off: 1;    // 记录led开关状态
    uint8_t state: 7;      // led运行状态
} led_t;

typedef struct
{
    int pin;
    int mode;
}led_config;

void init_led(led_config* led_conf, int count);
void deinit_led();
void led_set_state(enum LED_ID id, enum LED_Sta sta);