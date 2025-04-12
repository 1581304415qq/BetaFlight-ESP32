#include "motor.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <stdlib.h>

// 定义 PWM 参数
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT  // PWM 分辨率（0-8191）
#define LEDC_FREQUENCY      5000               // PWM 频率（Hz）

static uint8_t motor_pwm_gpio = 3;//CONFIG_ESP_MOTOR1_PIN; // PWM 信号引脚
static uint8_t motor_in1_gpio = 1;//CONFIG_ESP_IN1_PIN; // 方向控制引脚
static uint8_t motor_in2_gpio = 2;//CONFIG_ESP_IN2_PIN; // 方向控制引脚

// 初始化 GPIO
static void motor_gpio_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << motor_in1_gpio) | (1ULL << motor_in2_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// 初始化 PWM
static void motor_pwm_init() {
    // 配置定时器
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    // 配置通道
    ledc_channel_config_t channel_conf = {
        .gpio_num = motor_pwm_gpio,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .speed_mode = LEDC_HIGH_SPEED_MODE,  // 快速通道模式
        .duty = 0,  // 初始占空比为 0
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);
}


int motor_init()
{
    motor_gpio_init();
    motor_pwm_init();
    return 0;
}

// -100~100
void motor_set_speed(int16_t speed)
{
    if (speed > 0) {}
    else {}
    uint32_t ledc_duty = ledc_duty = abs(speed) / 100.0 * (2 << LEDC_DUTY_RES);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, ledc_duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// 0 left 1 right
void motor_set_direction(int8_t direct)
{
    if (direct) {}
    else {}
}

void motor_config()
{

}