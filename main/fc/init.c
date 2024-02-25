#include "msp.h"
#include "msp_serial.h"
#include "msp_protocol.h"
#include "system.h"
#include "bluetooth.h"
#include "stdbool.h"
#include "msp_data.h"
#include <string.h>
#include "init.h"
#include "util.h"

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
    case MSP_SET_ARMING_DISABLED:


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

    case MSP_SET_RTC:
        int32_t secs = (int32_t)readU32(payload, 0);
        uint16_t millis = readU16(payload, 4);
        break;

    case MSP_STATUS:
        msp_status_t msp_status = { 0 };
        msp_status.taskDeltaTimeUs = 28;
        msp_status.i2cErrorCount = 0;
        msp_status.sensorFlags = -1;
        msp_status.flightModeFlags = -1;
        msp_status.currentPidProfileIndex = 2;
        msp_status.averageSystemLoad = 24;

        payload_len = sizeof(msp_status_t);
        memcpy(payload_buf, &msp_status, payload_len);
        break;

    case MSP_RAW_IMU:
        msp_raw_imu_t msp_raw_imu;
        // todo

        payload_len = sizeof(msp_raw_imu_t);
        memcpy(payload_buf, &msp_raw_imu, payload_len);

        break;

    case MSP_UID:
        msp_uid_t msp_uid;
        msp_uid.id_1 = 0;
        msp_uid.id_2 = 1;
        msp_uid.id_3 = 2;

        payload_len = sizeof(msp_uid_t);
        memcpy(payload_buf, &msp_uid, payload_len);
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
}

void registerMspEvent() {
    mspRegisterFn(mspCommonProcess);
}

void init(void) {
    systemInit();

    // Initialize Ble
    initBluetooth();

    // Initialize MSP
    mspInit();
    mspSerialInit();


    registerMspEvent();
}