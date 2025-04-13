#pragma once

#include <stdint.h>
#include "mpu6050_driver.h"

void imuInit(void);
void imuDeinit(void);
int imuStart(void* callback);


void setUp(void);
void imu_init_test();
void read_sensor_data();
