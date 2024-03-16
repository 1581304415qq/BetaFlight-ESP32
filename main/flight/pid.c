#include "pid.h"
#include <stdio.h>
typedef struct
{
    float power;
    float integral;
    float derivative;
} pid_param_t;

/**
 *@brief PID控制算法
 * @param target: 目标温度
 * @param current: 当前温度
 * @param kp: 比例系数
 * @param ki: 积分系数
 * @param kd: 微分系数
 * @param integral: 前一时刻积分值
 * @param derivative: 前一时刻微分值
 * @param delta_t: 时间步长
 * @return pid_param_t
 */
pid_param_t pid_control(float target, float current, float kp, float ki, float kd, float integral, float derivative, float delta_t) {
    float error = target - current;
    integral = integral + error * delta_t;
    derivative = (error - derivative) / delta_t;
    float power = kp * error + ki * integral + kd * derivative;
    return (pid_param_t){ power, integral, derivative };
}

