#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "msp.h"
#include "msp_serial.h"
#include "msp_protocol.h"
#include "system.h"
#include "bluetooth.h"
#include "stdbool.h"
#include "msp_data.h"
#include "init.h"
#include "util.h"
#include "led.h"

#define FC_VARIANT "BTFL"

static void mspCommonProcess(uint8_t command, uint8_t* payload, uint8_t payloadLen) {
    bool handle = true;
    static uint8_t dst[300] = { 0 };
    uint8_t payload_buf[256] = { 0 };
    uint16_t dst_len = 0;
    int payload_len = 0;
    msp_message_t msp_message;
    msp_message.command = command;
    msp_message.header.protocol_version = 0;
    msp_message.header.direction_flag = '>';

    switch (command) {
        // INFO
    case MSP_API_VERSION:
        msp_api_version_t api_version;
        api_version.protocol_version = 0;
        api_version.api_version_major = 1;
        api_version.api_version_minor = 46;

        payload_len = sizeof(msp_api_version_t);
        memcpy(payload_buf, &api_version, payload_len);
        break;

    case MSP_FC_VARIANT:
        payload_len = snprintf((char*)payload_buf, 256, "%s", FC_VARIANT);

        break;


    case MSP_FC_VERSION:
        msp_fc_version_t fc_version;
        fc_version.fc_version_major = 4;
        fc_version.fc_version_minor = 5;
        fc_version.fc_version_patch_level = 0;

        payload_len = sizeof(msp_fc_version_t);
        memcpy(payload_buf, &fc_version, payload_len);

        break;

    case MSP_BOARD_INFO:
        int boardIdentifier = 123;
        uint16_t hardwareRevision = 1;
        payload_len = snprintf((char*)payload_buf, 256, "%d%hu%u", boardIdentifier, hardwareRevision, 0);

        break;

    case MSP_BUILD_INFO:
        char* buildDate = "Feb 25 2024";
        char* buildTime = "12:25:28";
        char* shortGitRevision = "0.02.01";
        payload_len = snprintf((char*)payload_buf, 256, "%s%s%s", buildDate, buildTime, shortGitRevision);
        break;

    case MSP_NAME:
#define CRAFT_NAME "YAMATO-FLIGHT"
        payload_len = strlen(CRAFT_NAME);
        memcpy(payload_buf, (char*)CRAFT_NAME, payload_len);
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

        payload_len = sizeof(mspBatteryConfig_t);
        memcpy(payload_buf, &mspBatteryConfig, payload_len);
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

        payload_len = sizeof(msp_current_meter_config_t);
        memcpy(payload_buf, &msp_current_meter_config, payload_len);
        break;
    case MSP_SET_CURRENT_METER_CONFIG:
        // todo 设置电流表配置
        break;

    case MSP_LED_COLORS:
#define LED_CONFIGURABLE_COLOR_COUNT 16
        msp_led_colors_t msp_led_colors[LED_CONFIGURABLE_COLOR_COUNT];
        for (uint8_t i = 0; i < LED_CONFIGURABLE_COLOR_COUNT; i++)
        {
            msp_led_colors[i].h = 233;
            msp_led_colors[i].s = 2;
            msp_led_colors[i].v = 45;
        }

        payload_len = LED_CONFIGURABLE_COLOR_COUNT * sizeof(msp_led_colors_t);
        memcpy(payload_buf, msp_led_colors, payload_len);
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

        payload_len = 1 + voltage_sensor_adc.maxVoltageSensorADC + sizeof(msp_voltage_meter_config_t);
        memcpy(payload_buf, &voltage_sensor_adc, payload_len);
        break;

    case MSP_SET_VOLTAGE_METER_CONFIG:
        // todo 保存电压传感器数据
        break;

    case MSP_SET_ARMING_DISABLED:
        // todo 禁飞处理
        break;

    case MSP_ACC_TRIM:
        msp_acc_trim_t msp_acc_trim;
        msp_acc_trim.pitch = 22;
        msp_acc_trim.roll = 33;

        payload_len = sizeof(msp_acc_trim_t);
        memcpy(payload_buf, &msp_acc_trim, payload_len);
        break;

    case MSP_MIXER_CONFIG:
        msp_mixer_config_t msp_mixer_config;
        msp_mixer_config.mixer_mode = 1;
        msp_mixer_config.yaw_motors_reversed = 1;

        payload_len = sizeof(msp_mixer_config_t);
        memcpy(payload_buf, &msp_mixer_config, payload_len);
        break;


    case MSP_SONAR_ALTITUDE:
        uint32_t sonarAltitude = 230;
        payload_len = sizeof(uint32_t);
        memcpy(payload_buf, &sonarAltitude, payload_len);
        break;

    case MSP_SET_RTC:
        int32_t secs = (int32_t)readU32(payload, 0);
        uint16_t millis = readU16(payload, 4);
        break;

    case MSP_STATUS:
        msp_status_t msp_status = { 0 };
        msp_status.taskDeltaTimeUs = 28;
        msp_status.i2cErrorCount = 0;
        msp_status.sensors.acc = 1;
        msp_status.sensors.baro = 1;
        msp_status.sensors.mag = 1;
        msp_status.sensors.gps = 1;
        msp_status.sensors.rangefinder = 1;
        msp_status.sensors.gyro = 1;
        msp_status.flightModeFlags = -1;
        msp_status.currentPidProfileIndex = 2;
        msp_status.averageSystemLoad = 24;
        msp_status.pidProfileCount = 13;
        msp_status.coreTemperature = 65;
        msp_status.pidProfileCount = 4;
        msp_status.currentControlRateProfileIndex = 3;

        payload_len = sizeof(msp_status_t);
        memcpy(payload_buf, &msp_status, payload_len);
        break;

    case MSP_RAW_IMU:
        msp_raw_imu_t msp_raw_imu;
        for (uint8_t i = 0; i < 3; i++)
        {
            msp_raw_imu.accel[i] = 125;
            msp_raw_imu.gyro[i] = 160;
            msp_raw_imu.mag[i] = 225;
        }

        payload_len = sizeof(msp_raw_imu_t);
        memcpy(payload_buf, &msp_raw_imu, payload_len);

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

        payload_len = sizeof(msp_raw_gps_t);
        memcpy(payload_buf, &msp_raw_gps, payload_len);
        break;

    case MSP_COMP_GPS:
        msp_comp_gps_t msp_comp_gps;
        msp_comp_gps.distanceToHome = 123;
        msp_comp_gps.directionToHome = 10;
        msp_comp_gps.update = 1;

        payload_len = sizeof(msp_comp_gps_t);
        memcpy(payload_buf, &msp_comp_gps, payload_len);
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

        payload_len = 1 + gps_svinfo.numChannel * sizeof(svinfo_t);
        memcpy(payload_buf, &gps_svinfo, payload_len);
        break;

    case MSP_RC:
        uint8_t channelCount = 1;
        mspRcData_t mspRcData[8];
        for (int i = 0; i < channelCount; i++) {
            mspRcData[i].value = 11;
        }

        payload_len = channelCount * sizeof(mspRcData_t);
        memcpy(payload_buf, mspRcData, payload_len);
        break;


    case MSP_ATTITUDE:
        msp_attitude_t msp_attitude;
        msp_attitude.roll = 230;
        msp_attitude.pitch = 560;
        msp_attitude.yaw = 123;

        payload_len = sizeof(msp_attitude_t);
        memcpy(payload_buf, &msp_attitude, payload_len);
        break;

    case MSP_ALTITUDE:
        msp_altitude_t msp_altitude;
        msp_altitude.estimatedAltitudeCm = 5767;
        msp_altitude.estimatedVario = 124;

        payload_len = sizeof(msp_altitude_t);
        memcpy(payload_buf, &msp_altitude, payload_len);
        break;

    case MSP_ANALOG:
        msp_analog_t msp_analog;
        msp_analog.legacyBatteryVoltage = 250u;
        msp_analog.batteryVoltage = 3230u;
        msp_analog.mAhDrawn = 25;
        msp_analog.rssi = 254;
        msp_analog.amperage = 123;

        payload_len = sizeof(msp_analog_t);
        memcpy(payload_buf, &msp_analog, payload_len);
        break;

    case MSP_BOXNAMES:
        const int page = payloadLen > 0 ? readU8(payload, 0) : 0;

        char boxs_name[] = "www;eee;fff;";
        payload_len = strlen(boxs_name);
        memcpy(payload_buf, boxs_name, payload_len);
        break;



    case MSP_VOLTAGE_METERS:
#define supportedVoltageMeterCount  2
        msp_voltageMeter_t msp_voltageMeter[supportedVoltageMeterCount];
        for (uint8_t i = 0; i < supportedVoltageMeterCount; i++)
        {
            msp_voltageMeter[i].id = i;
            msp_voltageMeter[i].displayFiltered = 1234 + i;
        }

        payload_len = supportedVoltageMeterCount * sizeof(msp_voltageMeter_t);
        memcpy(payload_buf, msp_voltageMeter, payload_len);
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

        payload_len = supportedCurrentMeterCount * sizeof(msp_currentMeter_t);
        memcpy(payload_buf, msp_currentMeter, payload_len);
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

        payload_len = sizeof(msp_batteryState_t);
        memcpy(payload_buf, &msp_batteryState, payload_len);
        break;








    case MSP_UID:
        msp_uid_t msp_uid;
        msp_uid.id_1 = 0;
        msp_uid.id_2 = 1;
        msp_uid.id_3 = 2;

        payload_len = sizeof(msp_uid_t);
        memcpy(payload_buf, &msp_uid, payload_len);
        break;

#define MAX_SUPPORTED_SERVOS 8
    case MSP_SERVO:
        uint16_t servo[8] = { 0 };

        payload_len = MAX_SUPPORTED_SERVOS * 2;
        memcpy(payload_buf, servo, payload_len);
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

        payload_len = MAX_SUPPORTED_SERVOS * sizeof(msp_servo_configurations_t);
        memcpy(payload_buf, msp_Servo_configurations, payload_len);
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
        payload_len = MAX_SERVO_RULES * sizeof(mspCustomServoMixer_t);
        memcpy(payload_buf, mspCustomServoMixer, payload_len);

        break;

    case MSP_EEPROM_WRITE:
        break;

    case MSP_DEBUG:
        debugValue_t debugValue;
        debugValue.value[0] = 9;
        debugValue.value[1] = 2;
        debugValue.value[2] = 6;
        debugValue.value[3] = 5;

        payload_len = sizeof(debugValue_t);
        memcpy(payload_buf, (uint8_t*)&debugValue, payload_len);
        break;

    default:
        handle = false;
        break;
    }

    if (handle) {
        msp_message.payload = payload_buf;
        msp_message.payload_size = payload_len;
        dst_len = packMessage(&msp_message, dst, sizeof(dst));
        serialWrite(dst, dst_len);
    }
    else
        sendByBt((char*)&command, 1);

}

void registerMspEvent() {
    mspRegisterFn(mspCommonProcess);
}

void init(void) {
    systemInit();
    initLED();

    // Initialize Ble
    initBluetooth();

    // Initialize MSP
    mspInit();
    mspSerialInit();

    registerMspEvent();

}