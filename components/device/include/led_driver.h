#pragma once
#include <stdint.h>

enum LED_Sta {
    OFF,                // 长关
    ON,                 // 长开
    QUICK_BLINK,        // 短闪 开100ms 关闭500ms
    SHARP_BLINK,        // 200ms闪烁
    HEIGHT_BLINK,       // 500ms闪烁
    LOW_BLINK,          // 1s闪烁 开1000ms 关闭1000ms
    SLEEP_BLINK,        // 3s闪烁 开3000ms 关闭3000ms
    SLEEP_Q_BLINK,      // 开100ms 关闭3000ms
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
void led_set_state(uint16_t id, enum LED_Sta sta);