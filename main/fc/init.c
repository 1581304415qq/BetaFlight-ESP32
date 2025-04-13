#include "msp.h"
#include "sdkconfig.h"
#include "msp_serial.h"
#include "msp_protocol.h"
#include "msp_protocol_v2_betaflight.h"
#include "system.h"
#include "stdbool.h"
#include "msp_data.h"
#include "msp.h"
#include "esp_log.h"
#include "init.h"
#include "util.h"
#include "led.h"
#include "voltage.h"
#include "pid.h"
#include "imu.h"
#include "http_server.h"
#include "tcp_server.h"

#define FC_VARIANT "BTFL"

#define TAG "BTFL"

extern double yaw, pitch, roll;

typedef bool (*RegisterMspEventCallack)(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len);

static RegisterMspEventCallack registerMspEventCallack[0x3FFF] = { 0 };

static void mspCommonProcess(uint8_t version, uint16_t command, uint8_t* payload, uint16_t payloadLen) {
    ESP_LOGI(TAG, "mspCommonProcess ver=%u, com=%u\n", version, command);

    bool handle = true;
    static uint8_t dst[300] = { 0 };
    uint16_t dst_len = 0;
    uint8_t reply_buf[256] = { 0 };
    uint16_t reply_len = 0;
    msp_message_t msp_message = { 0 };
    msp_message.command = command;
    msp_message.header.protocol_version = version;
    msp_message.header.direction_flag = '>';

    switch (command) {
        // INFO
    case MSP_API_VERSION:
        msp_api_version_t api_version;
        api_version.protocol_version = 0;
        api_version.api_version_major = 1;
        api_version.api_version_minor = 46;

        reply_len = sizeof(msp_api_version_t);
        memcpy(reply_buf, &api_version, reply_len);
        break;

    case MSP_FC_VARIANT:
        reply_len = snprintf((char*)reply_buf, 256, "%s", FC_VARIANT);

        break;


    case MSP_FC_VERSION:
        msp_fc_version_t fc_version;
        fc_version.fc_version_major = 4;
        fc_version.fc_version_minor = 5;
        fc_version.fc_version_patch_level = 0;

        reply_len = sizeof(msp_fc_version_t);
        memcpy(reply_buf, &fc_version, reply_len);

        break;

    case MSP_BOARD_INFO:
        int boardIdentifier = 23;
        uint16_t hardwareRevision = 1;
        reply_len = snprintf((char*)reply_buf, 256, "%d%hu%u", boardIdentifier, hardwareRevision, 0);

        break;

    case MSP_BUILD_INFO:
        char* buildDate = "Feb 25 2024";
        char* buildTime = "12:25:28";
        char* shortGitRevision = "0.02.01";
        reply_len = snprintf((char*)reply_buf, 256, "%s%s%s", buildDate, buildTime, shortGitRevision);
        break;

    case MSP_NAME:
#define CRAFT_NAME "YAMATO-FLIGHT"
        reply_len = strlen(CRAFT_NAME);
        memcpy(reply_buf, (char*)CRAFT_NAME, reply_len);
        break;

        //   COMMAND


    case MSP_BATTERY_CONFIG:  // 32
        mspBatteryConfig_t mspBatteryConfig;
        mspBatteryConfig.minCellVoltage = 0;
        mspBatteryConfig.maxCellVoltage = 0;
        mspBatteryConfig.warningCellVoltage = 0;
        mspBatteryConfig.batteryCapacity = 0;
        mspBatteryConfig.voltageSource = 0;
        mspBatteryConfig.currentSource = 0;
        mspBatteryConfig.vbatMin = 0;
        mspBatteryConfig.vbatMax = 0;
        mspBatteryConfig.vbatWarn = 0;

        reply_len = sizeof(mspBatteryConfig_t);
        memcpy(reply_buf, &mspBatteryConfig, reply_len);
        break;

    case MSP_SET_BATTERY_CONFIG:
        // todo 电池配置
        mspBatteryConfig_t mspBatteryConfigSave;
        memcpy(&mspBatteryConfigSave, payload, sizeof(mspBatteryConfig_t));
        // vbatlevel_warn1 in MWC2.3 GUI
        // batteryConfigMutable()->vbatmincellvoltage = sbufReadU8(src) * 10;
        break;



    case MSP_CURRENT_METER_CONFIG:  // 40
        msp_current_meter_config_t msp_current_meter_config;
        msp_current_meter_config.currentMeterCount = 1;
        msp_current_meter_config.adcSensorSubframeLength = 6;
        msp_current_meter_config.id = 1;
        msp_current_meter_config.adcVal = 232u;
        msp_current_meter_config.adcScale = 123;
        msp_current_meter_config.adcOffset = 0;

        reply_len = sizeof(msp_current_meter_config_t);
        memcpy(reply_buf, &msp_current_meter_config, reply_len);
        break;
    case MSP_SET_CURRENT_METER_CONFIG:
        // todo 设置电流表配置
        break;

    case MSP_LED_COLORS:
#define LED_CONFIGURABLE_COLOR_COUNT 16
        hsvColor_t led_colors[LED_CONFIGURABLE_COLOR_COUNT];
        for (uint8_t i = 0; i < LED_CONFIGURABLE_COLOR_COUNT; i++)
        {
            led_colors[i].hue = 233;
            led_colors[i].saturation = 2;
            led_colors[i].value = 45;
        }

        reply_len = LED_CONFIGURABLE_COLOR_COUNT * sizeof(hsvColor_t);
        memcpy(reply_buf, led_colors, reply_len);
        break;

    case MSP_SET_LED_COLORS:
        break;

    case MSP_LED_STRIP_CONFIG:

        break;
    case MSP_SET_LED_STRIP_CONFIG:
        break;

    case MSP_VOLTAGE_METER_CONFIG:
        voltage_sensor_adc_t voltage_sensor_adc;
        voltage_sensor_adc.maxVoltageSensorADC = 1;
        for (uint8_t i = 0; i < voltage_sensor_adc.maxVoltageSensorADC; i++)
        {
            voltage_sensor_adc.mspVoltageMeterConfig[i].adcSensorSubframeLength = 5;
            voltage_sensor_adc.mspVoltageMeterConfig[i].voltageMeterADCtoIDMap = i;
            voltage_sensor_adc.mspVoltageMeterConfig[i].voltageSensorTypeAdcResistorDivider = 0;
            voltage_sensor_adc.mspVoltageMeterConfig[i].vbatscale = 1;
            voltage_sensor_adc.mspVoltageMeterConfig[i].vbatresdivval = 12;
            voltage_sensor_adc.mspVoltageMeterConfig[i].vbatresdivmultiplier = 2;
        }

        reply_len = 1 + voltage_sensor_adc.maxVoltageSensorADC + sizeof(msp_voltage_meter_config_t);
        memcpy(reply_buf, &voltage_sensor_adc, reply_len);
        break;

    case MSP_SET_VOLTAGE_METER_CONFIG:
        // todo 保存电压传感器数据
        break;

    case MSP_PID_CONTROLLER:
#define PID_CONTROLLER_BETAFLIGHT 1
        reply_buf[0] = PID_CONTROLLER_BETAFLIGHT;
        reply_len = 1;
        break;

    case MSP_PID:
#define  PID_ITEM_COUNT 5
        pidf_t pid[PID_ITEM_COUNT] = { 0 };
        for (int i = 0; i < PID_ITEM_COUNT; i++) {

        }
        reply_len = sizeof(pidf_t) * PID_ITEM_COUNT;
        memcpy(reply_buf, pid, reply_len);
        break;

    case MSP_RC_TUNING:
        msp_rc_tuning_t msp_rc_tuning = { 0 };

        reply_len = sizeof(msp_rc_tuning_t);
        memcpy(reply_buf, &msp_rc_tuning, reply_len);
        break;

    case MSP_PIDNAMES:
        reply_len = sizeof(pidNames);
        memcpy(reply_buf, pidNames, reply_len);
        break;

    case MSP_FILTER_CONFIG:
        msp_filter_config_t msp_filter_config = { 0 };
        reply_len = sizeof(msp_filter_config_t);
        memcpy(reply_buf, &msp_filter_config, reply_len);
        break;

    case MSP_RC_DEADBAND:
        msp_rc_deadband_t msp_rc_deadband = { 0 };
        reply_len = sizeof(msp_rc_deadband_t);
        memcpy(reply_buf, &msp_rc_deadband, reply_len);
        break;
    case MSP_MOTOR_CONFIG:
        msp_motor_config_t msp_motor_config = { 0 };
        reply_len = sizeof(msp_motor_config_t);
        memcpy(reply_buf, &msp_motor_config, reply_len);
        break;
    case MSP_SET_ARMING_DISABLED:
        // todo 禁飞处理
        break;

    case MSP_ADVANCED_CONFIG:
        msp_advanced_config_t msp_advanced_config = { 0 };

        reply_len = sizeof(msp_advanced_config_t);
        memcpy(reply_buf, &msp_advanced_config, reply_len);
        break;

    case MSP_SIMPLIFIED_TUNING:
        break;
    case MSP_CALCULATE_SIMPLIFIED_PID:
        break;
    case MSP_CALCULATE_SIMPLIFIED_DTERM:
        break;
    case MSP_VALIDATE_SIMPLIFIED_TUNING:
        break;

    case MSP_ACC_TRIM:
        msp_acc_trim_t msp_acc_trim;
        msp_acc_trim.pitch = 22;
        msp_acc_trim.roll = 33;

        reply_len = sizeof(msp_acc_trim_t);
        memcpy(reply_buf, &msp_acc_trim, reply_len);
        break;

    case MSP_MIXER_CONFIG:
        msp_mixer_config_t msp_mixer_config;
        msp_mixer_config.mixer_mode = 1;
        msp_mixer_config.yaw_motors_reversed = 1;

        reply_len = sizeof(msp_mixer_config_t);
        memcpy(reply_buf, &msp_mixer_config, reply_len);
        break;

    case MSP_SONAR_ALTITUDE:
        uint32_t sonarAltitude = 230;
        reply_len = sizeof(uint32_t);
        memcpy(reply_buf, &sonarAltitude, reply_len);
        break;

    case MSP_PID_ADVANCED:
        msp_pidProfile_t msp_pidProfile;
        msp_pidProfile.pidF[0] = 2;

        reply_len = sizeof(msp_pidProfile_t);
        memcpy(reply_buf, &msp_pidProfile, reply_len);
        break;

    case MSP_SET_RTC:
        int32_t secs = (int32_t)readU32(payload, 0);
        uint16_t millis = readU16(payload, 4);
        break;

    case MSP_STATUS_EX:
    case MSP_STATUS:
        msp_status_t msp_status = { 0 };
        msp_status.taskDeltaTimeUs = 28;
        msp_status.i2cErrorCount = -1;

        msp_status.sensors.gyro = 1;
        msp_status.sensors.acc = 1;
        msp_status.sensors.baro = 1;
        msp_status.sensors.mag = 1;
        msp_status.sensors.sonar = 1;
        msp_status.sensors.gps = 1;
        msp_status.sensors.gpsmag = 1;

        msp_status.flightModeFlags = -1;
        msp_status.currentPidProfileIndex = 2;
        msp_status.averageSystemLoad = 24;
        msp_status.pidProfileCount = 13;
        msp_status.coreTemperature = 65;
        msp_status.pidProfileCount = -1;
        msp_status.currentControlRateProfileIndex = -1;

        reply_len = sizeof(msp_status_t);
        memcpy(reply_buf, &msp_status, reply_len);
        break;

    case MSP_RAW_IMU:
        msp_raw_imu_t msp_raw_imu;
        for (uint8_t i = 0; i < 3; i++)
        {
            msp_raw_imu.accel[i] = 125;
            msp_raw_imu.gyro[i] = 160;
            msp_raw_imu.mag[i] = 225;
        }

        reply_len = sizeof(msp_raw_imu_t);
        memcpy(reply_buf, &msp_raw_imu, reply_len);

        break;

    case MSP_RAW_GPS:
        msp_raw_gps_t msp_raw_gps;
        msp_raw_gps.fix_type = 5;
        msp_raw_gps.num_satellites = 25;
        msp_raw_gps.latitude = 1208767262u;
        msp_raw_gps.longitude = 257555572u;
        msp_raw_gps.altitude = 5767;
        msp_raw_gps.speed = 12;
        msp_raw_gps.course = 5.9;
        msp_raw_gps.hdop = 0;

        reply_len = sizeof(msp_raw_gps_t);
        memcpy(reply_buf, &msp_raw_gps, reply_len);
        break;

    case MSP_COMP_GPS:
        msp_comp_gps_t msp_comp_gps;
        msp_comp_gps.distanceToHome = 123;
        msp_comp_gps.directionToHome = 10;
        msp_comp_gps.update = 1;

        reply_len = sizeof(msp_comp_gps_t);
        memcpy(reply_buf, &msp_comp_gps, reply_len);
        break;

    case MSP_GPSSVINFO:
        gps_svinfo_t gps_svinfo;
        gps_svinfo.numChannel = 1;
        for (uint8_t i = 0; i < 1; i++)
        {
            gps_svinfo.svinfo[i].channel = i;
            gps_svinfo.svinfo[i].svid = 9;
            gps_svinfo.svinfo[i].quality = 10;
            gps_svinfo.svinfo[i].cno = 7;
        }

        reply_len = 1 + gps_svinfo.numChannel * sizeof(svinfo_t);
        memcpy(reply_buf, &gps_svinfo, reply_len);
        break;

    case MSP_RC:
        uint8_t channelCount = 1;
        mspRcData_t mspRcData[8];
        for (int i = 0; i < channelCount; i++) {
            mspRcData[i].value = 11;
        }

        reply_len = channelCount * sizeof(mspRcData_t);
        memcpy(reply_buf, mspRcData, reply_len);
        break;


    case MSP_ATTITUDE:
        msp_attitude_t msp_attitude;
        msp_attitude.roll = roll;
        msp_attitude.pitch = pitch;
        msp_attitude.yaw = yaw;

        reply_len = sizeof(msp_attitude_t);
        memcpy(reply_buf, &msp_attitude, reply_len);
        break;

    case MSP_ALTITUDE:
        msp_altitude_t msp_altitude;
        msp_altitude.estimatedAltitudeCm = 5767;
        msp_altitude.estimatedVario = 124;

        reply_len = sizeof(msp_altitude_t);
        memcpy(reply_buf, &msp_altitude, reply_len);
        break;

    case MSP_ANALOG:
        msp_analog_t msp_analog;
        msp_analog.legacyBatteryVoltage = 250u;
        msp_analog.batteryVoltage = 3230u;
        msp_analog.mAhDrawn = 25;
        msp_analog.rssi = 254;
        msp_analog.amperage = 123;

        reply_len = sizeof(msp_analog_t);
        memcpy(reply_buf, &msp_analog, reply_len);
        break;

    case MSP_BOXNAMES:
        const int page = payloadLen > 0 ? readU8(payload, 0) : 0;

        char boxs_name[] = "www;eee;fff;";
        reply_len = strlen(boxs_name);
        memcpy(reply_buf, boxs_name, reply_len);
        break;



    case MSP_LED_STRIP_MODECOLOR:
#define LED_AUX_CHANNEL 11
        msp_ledStripConfig_t msp_ledStripConfig;

        break;
    case MSP_VOLTAGE_METERS:
#define supportedVoltageMeterCount  2
        msp_voltageMeter_t msp_voltageMeter[supportedVoltageMeterCount];
        for (uint8_t i = 0; i < supportedVoltageMeterCount; i++)
        {
            msp_voltageMeter[i].id = i;
            msp_voltageMeter[i].displayFiltered = 1234 + i;
        }

        reply_len = supportedVoltageMeterCount * sizeof(msp_voltageMeter_t);
        memcpy(reply_buf, msp_voltageMeter, reply_len);
        break;

    case MSP_CURRENT_METERS:
#define supportedCurrentMeterCount 2
        msp_currentMeter_t msp_currentMeter[supportedCurrentMeterCount];
        for (uint8_t i = 0; i < supportedCurrentMeterCount; i++)
        {
            msp_currentMeter[i].id = i;
            msp_currentMeter[i].mAhDrawn = 2538 + i;
            msp_currentMeter[i].amperage = 1237;
        }

        reply_len = supportedCurrentMeterCount * sizeof(msp_currentMeter_t);
        memcpy(reply_buf, msp_currentMeter, reply_len);
        break;

    case MSP_BATTERY_STATE:
        msp_batteryState_t msp_batteryState;
        msp_batteryState.cellCount = 2;
        msp_batteryState.batteryCapacity = 5500;
        msp_batteryState.batteryVoltageLegacy = 250u;
        msp_batteryState.mAhDrawn = 2500;
        msp_batteryState.amperage = 200;
        msp_batteryState.batteryState = 2;
        msp_batteryState.batteryVoltage = 1200;

        reply_len = sizeof(msp_batteryState_t);
        memcpy(reply_buf, &msp_batteryState, reply_len);
        break;








    case MSP_UID:
        msp_uid_t msp_uid;
        msp_uid.id_1 = 0;
        msp_uid.id_2 = 1;
        msp_uid.id_3 = 2;

        reply_len = sizeof(msp_uid_t);
        memcpy(reply_buf, &msp_uid, reply_len);
        break;

#define MAX_SUPPORTED_SERVOS 8
    case MSP_SERVO:
        uint16_t servo[8] = { 0 };

        reply_len = MAX_SUPPORTED_SERVOS * 2;
        memcpy(reply_buf, servo, reply_len);
        break;

    case MSP_SERVO_CONFIGURATIONS:
        msp_servo_configurations_t msp_Servo_configurations[MAX_SUPPORTED_SERVOS];
        for (int i = 0; i < MAX_SUPPORTED_SERVOS; i++) {
            msp_Servo_configurations[i].min = 12;
            msp_Servo_configurations[i].max = 12;
            msp_Servo_configurations[i].middle = 12;
            msp_Servo_configurations[i].rate = 12;
            msp_Servo_configurations[i].forwardFromChannel = 12;
            msp_Servo_configurations[i].reversedSources = 12;
        }

        reply_len = MAX_SUPPORTED_SERVOS * sizeof(msp_servo_configurations_t);
        memcpy(reply_buf, msp_Servo_configurations, reply_len);
        break;

    case MSP_SET_SERVO_CONFIGURATION:
        // todo 配置伺服电机
        break;

    case MSP_SERVO_MIX_RULES:
#define MAX_SERVO_RULES (2 * 8)
        mspCustomServoMixer_t mspCustomServoMixer[MAX_SERVO_RULES];
        for (int i = 0; i < MAX_SERVO_RULES; i++) {
            mspCustomServoMixer[i].targetChannel = 13;
            mspCustomServoMixer[i].inputSource = 13;
            mspCustomServoMixer[i].rate = 13;
            mspCustomServoMixer[i].speed = 13;
            mspCustomServoMixer[i].min = 13;
            mspCustomServoMixer[i].max = 13;
            mspCustomServoMixer[i].box = 13;
        }
        reply_len = MAX_SERVO_RULES * sizeof(mspCustomServoMixer_t);
        memcpy(reply_buf, mspCustomServoMixer, reply_len);

        break;

    case MSP_EEPROM_WRITE:
        break;

    case MSP_DEBUG:
        debugValue_t debugValue;
        debugValue.value[0] = 9;
        debugValue.value[1] = 2;
        debugValue.value[2] = 6;
        debugValue.value[3] = 5;

        reply_len = sizeof(debugValue_t);
        memcpy(reply_buf, (uint8_t*)&debugValue, reply_len);
        break;

    default:
        handle = false;
        if (registerMspEventCallack[command])
            handle = registerMspEventCallack[command](payload, payloadLen, reply_buf, &reply_len);
    }

    if (handle) {
        memcpy(msp_message.payload, reply_buf, reply_len);
        msp_message.payload_size = reply_len;
        dst_len = packMessage(&msp_message, dst, sizeof(dst));
        mspSerialWrite(dst, dst_len);
        ESP_LOGI(TAG, "handle ver=%u, com=%u, dst_len=%d", version, command, dst_len);
    }
    else {
        ESP_LOGI(TAG, "ver=%u, com=%u", version, command);
    }
}

void registerMspEvent(uint16_t eventType, RegisterMspEventCallack callback) {
    if (!registerMspEventCallack[eventType])registerMspEventCallack[eventType] = callback;
    else printf("duplicate registration msp event");
}

void unregisterMspEvent(uint16_t eventType, RegisterMspEventCallack callback) {
    if (registerMspEventCallack[eventType] == callback) registerMspEventCallack[eventType] = NULL;
}


bool betaflight_bind_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}

bool motor_output_reordering_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}
bool set_motor_output_reordering_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}
bool send_dshot_command_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}
bool get_vtx_device_status_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}
bool get_osd_warnings_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return false;
}
bool get_text_event(const uint8_t* payload, const uint16_t payload_len, uint8_t* reply, uint16_t* reply_len) {
    return true;
}

void initMspEvent() {
    mspRegisterFn(mspCommonProcess);

    // registerMspEvent(MSP2_BETAFLIGHT_BIND, betaflight_bind_event);
    // registerMspEvent(MSP2_MOTOR_OUTPUT_REORDERING, motor_output_reordering_event);
    // registerMspEvent(MSP2_SET_MOTOR_OUTPUT_REORDERING, set_motor_output_reordering_event);
    // registerMspEvent(MSP2_SEND_DSHOT_COMMAND, send_dshot_command_event);
    // registerMspEvent(MSP2_GET_VTX_DEVICE_STATUS, get_vtx_device_status_event);
    // registerMspEvent(MSP2_GET_OSD_WARNINGS, get_osd_warnings_event);
    registerMspEvent(MSP2_GET_TEXT, get_text_event);

}

void init(void) {
    systemInit();


    // ret = serialInit();

    // voltageMeterADCInit();

    // mpu6050
    imuInit();

    // Initialize MSP
    // mspInit();
    // mspSerialInit();

    // initMspEvent();

    // voltageMeter_t voltageMeter;

    http_server_init("/spiffs");
    // while (1)
    // {
        // voltageMeterADCRefresh();
        // voltageMeterADCRead(VOLTAGE_SENSOR_ADC_12V, &voltageMeter);
        // ESP_LOGI(TAG,"get chan=%d, voltage=%d", VOLTAGE_SENSOR_ADC_12V, voltageMeter.unfiltered);

        // float degrees = 0;
        // int ret = mpu6050_basic_read_temperature(&degrees);
        // float g[3] = { 0 }, dps[3] = { 0 };
        // ret = mpu6050_basic_read(g, dps);
        // for (int i = 0; ret==0 && i < 3; i++)
        // {
        //     printf("g[%d]=%f, dps[%d]=%f\n", i, g[i], i, dps[i]);
        // }
        // printf("ret=%d\n", ret);

        // vTaskDelay(2000 / portTICK_PERIOD_MS);

    // }

}

static tcp_client_ctx_t* client = NULL;
static void imu_data_handler(
    mpu6050_raw_acce_value_t* mpu6050_raw_acce_value,
    mpu6050_raw_gyro_value_t* mpu6050_raw_gyro_value,
    int16_t mpu6050_temp_value
) {
    static char data[512];
    if (client) {
        snprintf(data, sizeof(data), "Accel: X=%6d, Y=%6d, Z=%6d\nGyro: X=%6d, Y=%6d, Z=%6d\nTemp: %6d\n",
            mpu6050_raw_acce_value->raw_acce_x, mpu6050_raw_acce_value->raw_acce_y, mpu6050_raw_acce_value->raw_acce_z,
            mpu6050_raw_gyro_value->raw_gyro_x, mpu6050_raw_gyro_value->raw_gyro_y, mpu6050_raw_gyro_value->raw_gyro_z,
            mpu6050_temp_value
            );
        server_send(client, data, strlen(data));
    }
}

void run(void) {
    http_server_start();
    client = start_server();

    imuStart(imu_data_handler);

    led_launch();
}
