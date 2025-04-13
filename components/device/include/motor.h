#pragma once 
#include <stdint.h>

int motor_init();
void motor_set_speed(int16_t speed);
void motor_set_direction(int8_t direct);