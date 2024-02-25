#pragma once

#include <stdint.h>

// msp_api_version
typedef struct {
    uint8_t protocol_version;
    uint8_t api_version_major;
    uint8_t api_version_minor;
} __attribute__((packed)) msp_api_version_t;

// msp_fc_version
typedef struct {
    uint8_t fc_version_major;
    uint8_t fc_version_minor;
    uint8_t fc_version_patch_level;
} __attribute__((packed)) msp_fc_version_t;

// msp_status
typedef struct
{
    uint16_t taskDeltaTimeUs; // pid任务循环间隔
    uint16_t i2cErrorCount; // i2c错误计数

    uint16_t sensorFlags;   // 传感器状态位

    uint32_t flightModeFlags; // 飞行模式位(部分)
    uint8_t flightModeFlagsCount; // 飞行模式位数
    uint8_t flightModeFlagsExtra[15]; // 额外飞行模式位

    uint8_t currentPidProfileIndex;

    uint16_t averageSystemLoad;

    uint8_t pidProfileCount;

    uint8_t currentControlRateProfileIndex;

    uint8_t armingDisableFlagsCount;
    uint32_t armingDisableFlags;

    uint8_t rebootRequired : 1; // 1位

    uint16_t coreTemperature; // CPU温度

} __attribute__((packed)) msp_status_t;



// MSP_RAW_IMU
typedef struct {
    uint16_t accel[3];
    uint16_t gyro[3];
    uint16_t mag[3];
} __attribute__((packed)) msp_raw_imu_t;

// msp uid
typedef struct {
    uint32_t id_1;
    uint32_t id_2;
    uint32_t id_3;
} __attribute__((packed)) msp_uid_t;

// MSP_ACC_TRIM
typedef struct
{
    uint16_t pitch;
    uint16_t roll;
} __attribute__((packed)) msp_acc_trim_t;

// MSP_MIXER_CONFIG
typedef struct
{
    uint8_t mixer_mode;
    uint8_t yaw_motors_reversed;
} __attribute__((packed)) msp_mixer_config_t;

