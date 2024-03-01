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

    struct sensorStatus {   // 传感器状态位
        uint16_t acc : 1;
        uint16_t baro : 1;
        uint16_t mag : 1;
        uint16_t gps : 1;
        uint16_t rangefinder : 1;
        uint16_t gyro : 1;
    } sensors;

    uint32_t flightModeFlags; // 飞行模式位(部分)

    uint8_t currentPidProfileIndex;

    uint16_t averageSystemLoad;

    uint8_t pidProfileCount;

    uint8_t currentControlRateProfileIndex;

    uint8_t byteCount;
    // flightModeFlags + 4

    uint8_t armingDisableFlagsCount;
    uint32_t armingDisableFlags;

    uint8_t rebootRequired;
    uint16_t coreTemperature; // CPU温度

} __attribute__((packed)) msp_status_t;


typedef struct {
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
} __attribute__((packed)) hsvColor_t;

// MSP_RAW_IMU
typedef struct {
    uint16_t accel[3];
    uint16_t gyro[3];
    uint16_t mag[3];
} __attribute__((packed)) msp_raw_imu_t;

// MSP_RAW_GPS
typedef struct {
    uint8_t fix_type;        // GPS解状态,取值0-3
    uint8_t num_satellites;  // 可见卫星数量

    int32_t latitude;       // 纬度,单位 0.0000001 度
    int32_t longitude;      // 经度,单位 0.0000001 度
    uint16_t altitude;       // 高度,放大了100倍,单位米

    uint16_t speed;          // 地面速度, 单位厘米/秒
    uint16_t course;         // 地面航向, 单位 0.1度

    uint16_t hdop;           // 水平精度因子

} __attribute__((packed)) msp_raw_gps_t;

// MSP_COMP_GPS
typedef struct {
    uint16_t distanceToHome;
    uint16_t directionToHome;
    uint8_t update;
}__attribute__((packed)) msp_comp_gps_t;

// MSP_GPSSVINFO
typedef struct {
    uint8_t channel;
    uint8_t svid;
    uint8_t quality;
    uint8_t cno;
}__attribute__((packed)) svinfo_t;

typedef struct {
    uint8_t numChannel;
    svinfo_t svinfo[10];
}__attribute__((packed)) gps_svinfo_t;

// MSP_ATTITUDE
typedef struct {
    uint16_t roll;
    uint16_t pitch;
    uint16_t yaw;
} __attribute__((packed)) msp_attitude_t;

// MSP_ALTITUDE
typedef struct {
    uint32_t estimatedAltitudeCm;
    uint16_t estimatedVario;
} __attribute__((packed)) msp_altitude_t;

// MSP_ANALOG
typedef struct
{
    uint8_t legacyBatteryVoltage; // 电池电压值, 单位 0.1V。
    uint16_t mAhDrawn;      // 从电池消耗的总毫安时数。
    uint16_t rssi;          // 接收信号强度。(这里没有用到)
    uint16_t amperage;      // 电池电压l
    uint16_t batteryVoltage;      // 实时电流值,单位 0.01A。
} __attribute__((packed)) msp_analog_t;


typedef struct {
    uint8_t id;
    uint16_t displayFiltered;
    // 过滤后显示的电压值
} __attribute__((packed)) msp_voltageMeter_t;

typedef struct {
    uint8_t id;
    uint16_t mAhDrawn;
    // 从电池耗费的毫安时容量

    int16_t amperage;
    // 电流计测量的实时电流值,单位 0.001A (毫安)
} __attribute__((packed)) msp_currentMeter_t;

typedef struct {

    uint8_t cellCount;
    // 电池串数

    uint16_t batteryCapacity;
    // 电池容量,单位 mAh

    uint8_t batteryVoltageLegacy;
    // 传统电压值,单位 0.1V

    uint16_t mAhDrawn;
    // 从电池耗费的毫安时容量

    int16_t amperage;
    // 电流值,单位 0.01A

    uint8_t batteryState;
    // 电池状态位

    uint16_t batteryVoltage;
    // 电压值,单位 0.01V

} __attribute__((packed)) msp_batteryState_t;



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


// MSP_CURRENT_METER_CONFIG
typedef struct
{
    uint8_t currentMeterCount;
    uint8_t adcSensorSubframeLength;
    uint8_t id;
    uint8_t adcVal;
    uint16_t adcScale;
    uint16_t adcOffset;
} __attribute__((packed)) msp_current_meter_config_t;

// MSP_VOLTAGE_METER_CONFIG
typedef struct {
    uint8_t adcSensorSubframeLength; // 子帧长度
    uint8_t voltageMeterADCtoIDMap; // 传感器ID
    uint8_t voltageSensorTypeAdcResistorDivider; // 传感器类型
    uint8_t vbatscale;  // 比例系数
    uint8_t vbatresdivval;// 分压比值
    uint8_t vbatresdivmultiplier; //分压倍乘系数
} __attribute__((packed)) msp_voltage_meter_config_t;

typedef struct {
    uint8_t maxVoltageSensorADC; // 电压传感器数量
    msp_voltage_meter_config_t mspVoltageMeterConfig[10];
} __attribute__((packed)) voltage_sensor_adc_t;

// MSP_BATTERY_CONFIG
typedef struct {

    uint8_t minCellVoltage; // 最小电池单体电压,放大10倍
    uint8_t maxCellVoltage; // 最大电池单体电压,放大10倍
    uint8_t warningCellVoltage; // 电池警告电压阈值,放大10倍

    uint16_t batteryCapacity; // 电池容量,单位mAh

    uint8_t voltageSource; // 电压采集源 
    uint8_t currentSource; // 电流采集源

    uint16_t vbatMin; // 最小电池总电压,单位cV
    uint16_t vbatMax; // 最大电池总电压,单位cV 
    uint16_t vbatWarn; // 电池总电压警告阈值,单位cV

} __attribute__((packed)) mspBatteryConfig_t;

typedef struct {
    uint16_t value[4];
}__attribute__((packed)) debugValue_t;

typedef struct {
    uint16_t min;
    uint16_t max;
    uint16_t middle;
    uint8_t rate;
    uint8_t forwardFromChannel;
    uint32_t reversedSources;
}__attribute__((packed)) msp_servo_configurations_t;

typedef struct customServoMixer_s {
    uint8_t targetChannel;          // 混控规则对应的通道
    uint8_t inputSource;            // 输入源
    int8_t rate;                    // 混控率
    uint8_t speed;                  // 转速
    int8_t min;                     // 最低油门
    int8_t max;                     // 最高油门  
    uint8_t box;                    // 模式
}__attribute__((packed)) mspCustomServoMixer_t;


typedef struct {
    uint16_t value;
} __attribute__((packed)) mspRcData_t;


#define LED_MODE_COUNT 6
#define LED_DIRECTION_COUNT 6
#define LED_SPECIAL_COLOR_COUNT 11
typedef struct {
    hsvColor_t modeColors[LED_MODE_COUNT][LED_DIRECTION_COUNT];
    hsvColor_t specialColors[LED_SPECIAL_COLOR_COUNT];

    uint8_t auxChannel;
    uint8_t flag;
    uint8_t ledstrip_aux_channel;

}  __attribute__((packed)) msp_ledStripConfig_t;


typedef struct {
    uint16_t flag1;
    uint16_t flag2;
    // yaw_p_limit - yaw P限制
    uint16_t yaw_p_limit;
    uint8_t flag4;
    // 电池电压PID补偿
    uint8_t vbatPidCompensation;

    // feedforward_transition - 前馈过渡
    uint8_t feedforward_transition;

    uint8_t flag6;
    uint8_t flag7;
    uint8_t flag8;
    uint8_t flag9;

    // rateAccelLimit - 角速度加速度限制 
    uint16_t rateAccelLimit;
    // yawRateAccelLimit - 偏航角速度加速度限制
    uint16_t yawRateAccelLimit;
    // angle_limit - 角度限制
    uint8_t angle_limit;

    uint8_t levelSensitivity;
    // itermThrottleThreshold - 油门积分项阈值 
    uint16_t itermThrottleThreshold;
    // anti_gravity_gain - 反重力增益
    uint16_t anti_gravity_gain;
    uint16_t dtermSetpointWeight;

    // iterm_rotation - 积分项旋转
    uint8_t iterm_rotation;
    uint8_t smart_feedforward;

    // iterm_relax - 积分项放松
    uint8_t iterm_relax;

    // iterm_relax_type - 积分项放松类型
    uint8_t iterm_relax_type;

    // abs_control_gain - 绝对控制增益
    uint8_t abs_control_gain;

    // throttle_boost - 油门提升
    uint8_t throttle_boost;

    // acro_trainer_angle_limit - 特技飞行训练角度限制
    uint8_t acro_trainer_angle_limit;

    // pidF[3] - PID F项比例因子数组(Roll/Pitch/Yaw)
    uint16_t pidF[3];
    uint8_t antiGravityMode;

    // d_min[3] - D最小值数组(Roll/Pitch/Yaw) 
    uint8_t d_min[3];

    // d_min_gain - D最小值增益
    uint8_t d_min_gain;

    // d_min_advance - D最小值提前量
    uint8_t d_min_advance;

    // use_integrated_yaw - 使用集成式偏航
    uint8_t use_integrated_yaw;

    // integrated_yaw_relax - 集成式偏航放松  
    uint8_t integrated_yaw_relax;

    uint8_t iterm_relax_cutoff;

    // motor_output_limit - 电机输出限制
    uint8_t motor_output_limit;

    // auto_profile_cell_count - 自动适配电池节数
    uint8_t auto_profile_cell_count;

    // dyn_idle_min_rpm - 动态空闲最低转速
    uint8_t dyn_idle_min_rpm;

    // feedforward_averaging - 前馈平均
    uint8_t feedforward_averaging;

    // feedforward_smooth_factor - 前馈平滑因子
    uint8_t feedforward_smooth_factor;

    // feedforward_boost - 前馈增强  
    uint8_t feedforward_boost;

    // feedforward_max_rate_limit - 前馈最大速率限制
    uint8_t feedforward_max_rate_limit;

    // feedforward_jitter_factor - 前馈抖动因子
    uint8_t feedforward_jitter_factor;

    // vbat_sag_compensation - 电池电压下降补偿
    uint8_t vbat_sag_compensation;

    // thrustLinearization - 推力线性化
    uint8_t thrustLinearization;

    // tpa_mode - TPA模式  
    uint8_t tpa_mode;

    // tpa_rate - TPA速率
    uint8_t tpa_rate;
    uint16_t tpa_breakpoint;
}__attribute__((packed)) msp_pidProfile_t;