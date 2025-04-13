#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "msp.h"
#include "queue.h"

#define crc8_dvb_s2(crc, a)        crc8_calc(crc, a, 0xD5)

uint8_t crc8_calc(uint8_t crc, unsigned char a, uint8_t poly)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ poly;
        }
        else {
            crc = crc << 1;
        }
    }
    return crc;
}

// <preamble>,<direction>,<size>,<command>,,<crc>
// $, M, <, size(1), command, payload(0-255), checksum 
int parse_msp_packet(const uint8_t* packet, uint8_t packet_len, msp_header_t* header, uint16_t* command, uint16_t* payload_len, uint8_t* payload) {
    if (packet_len < 6 || packet[0] != '$') {
        // 无效的数据包头
        return 1;
    }
    header->start_byte = packet[0];

    if (packet[1] != 'M' && packet[1] != 'X') {
        return 2;
    }
    header->message_type = packet[1];

    uint8_t packet_version = 0;
    if (packet[1] == 'M')
    {
        packet_version = MSP_V1;
    }
    else if (packet[1] == 'X') {
        packet_version = MSP_V2_NATIVE;
    }

    if (packet[2] != '<' && packet[2] != '>') {
        return 3;
    }
    header->direction_flag = packet[2];

    // 检查V1 or V2_Over_V1
#define MSP_V2_FRAME_ID 255
    if (packet[3] >= 6 && packet[4] == MSP_V2_FRAME_ID) {
        packet_version = MSP_V2_OVER_V1;
    }
    header->protocol_version = packet_version;
    printf("MSP VERSION = %d\n", packet_version);

    if (packet_version == MSP_V1)
    {
        *payload_len = packet[3];
        *command = packet[4];

        if (*payload_len + 6 > packet_len) {
            // 数据包长度错误
            printf("payload_len=%d packet_len=%d\n", *payload_len, packet_len);
            return 5;
        }

        for (uint8_t i = 0; i < *payload_len; i++) {
            payload[i] = packet[5 + i];
        }
    }
    else if (packet_version == MSP_V2_NATIVE) {
        uint8_t flag = packet[3];
        *command = packet[5] << 8 | packet[4];
        *payload_len = packet[7] << 8 | packet[6];

        if (*payload_len + 9 != packet_len) {
            // 数据包长度错误
            printf("v2 payload %d %d\n", *payload_len, packet_len);
            return 6;
        }

        for (uint8_t i = 0; i < *payload_len; i++) {
            payload[i] = packet[8 + i];
        }
    }
    else if (packet_version == MSP_V2_OVER_V1) {
        uint8_t flag = packet[5];
        *command = packet[6] << 8 | packet[7];
        *payload_len = packet[8] << 8 | packet[9];
    }

    // <size>, <command> and each data byte into a zero'ed sum
    uint8_t checksum = 0;
    for (uint8_t i = 3; i < (*payload_len + 5); i++) {
        if (packet_version == MSP_V1)
            checksum ^= packet[i];

        else if (packet_version == MSP_V2_NATIVE)
            checksum = crc8_dvb_s2(checksum, packet[i]);
    }

    if (packet_version == MSP_V1 && checksum != packet[*payload_len + 5]) {
        // 校验和错误
        printf("CHECK_SUM = %x\n", checksum);
        return 4;
    }


    return 0;
}

int parse_msp_mechine(Queue* queue, msp_message_t* msp_message) {
    if (getSize(queue) < 1) return MSP_LESS;

    uint8_t byte;
    int ret = dequeue(queue, &byte), result = 0;
    printf("byte=%02x, %c\n", byte, byte);
    switch (msp_message->header.status)
    {
    case MSP_IDLE:
        if ('$' == byte) {
            msp_message->header.start_byte = byte;
            msp_message->header.status = MSP_START_FRAME;
        }
        else {
            msp_message->header.status = MSP_IDLE;
            result = MSP_START_FRAME;
        }

        break;
    case MSP_START_FRAME:
        if ('M' == byte || 'X' == byte) {
            msp_message->header.message_type = byte;
            msp_message->header.status = MSP_VERSION;
            if (byte == 'M')
            {
                msp_message->header.protocol_version = MSP_V1;
                // 检查V1 or V2_Over_V1
    // #define MSP_V2_FRAME_ID 255
    //             if (packet[3] >= 6 && packet[4] == MSP_V2_FRAME_ID) {
    //                 msp_message->header.protocol_version = MSP_V2_OVER_V1;
    //             }
            }
            else if (byte == 'X') {
                msp_message->header.protocol_version = MSP_V2_NATIVE;
            }
        }
        else {
            msp_message->header.status = MSP_IDLE;
            result = MSP_VERSION;
        }

        break;
    case MSP_VERSION:
        if ('<' == byte || '>' == byte) {
            msp_message->header.direction_flag = byte;
            msp_message->header.status = MSP_DIRECTION;
        }
        else {
            msp_message->header.status = MSP_IDLE;
            result = MSP_DIRECTION;
        }

        break;
    case MSP_DIRECTION:
        if (msp_message->header.protocol_version == MSP_V1)
        {
            msp_message->payload_size = byte;
            msp_message->header.status = MSP_SIZE;
            msp_message->checksum ^= byte;
        }
        else if (msp_message->header.protocol_version == MSP_V2_NATIVE) {
            msp_message->header.status = MSP_SIZE;
            msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte);
        }
        else if (msp_message->header.protocol_version == MSP_V2_OVER_V1) {
            // uint8_t flag = packet[5];
            // *command = packet[6] << 8 | packet[7];
            // *payload_len = packet[8] << 8 | packet[9];
        }

        break;
    case MSP_SIZE:
        if (msp_message->header.protocol_version == MSP_V1)
        {
            msp_message->command = byte;
            msp_message->checksum ^= byte;
            msp_message->header.status = MSP_COMMAND;
        }
        else if (msp_message->header.protocol_version == MSP_V2_NATIVE) {
            if (getSize(queue) < 1)return MSP_LESS;
            uint8_t byte2;
            ret = dequeue(queue, &byte2);
            msp_message->command = byte2 << 8 | byte;
            msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte);
            msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte2);
            msp_message->header.status = MSP_COMMAND_V2;
        }

        break;
    case MSP_COMMAND_V2: {
        if (getSize(queue) < 1)return MSP_LESS;
        uint8_t byte2;
        ret = dequeue(queue, &byte2);
        msp_message->payload_size = byte2 << 8 | byte;
        msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte);
        msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte2);
        msp_message->header.status = MSP_COMMAND;
        break;
    }
    case MSP_COMMAND: {
        static uint32_t payload_offset = 0;
        // printf("payload_size=%d, payload_offset=%d\n", msp_message->payload_size, payload_offset);
        if (payload_offset == msp_message->payload_size) {
            msp_message->header.status = MSP_PAYLOAD;
            payload_offset = 0;
            //如果接收payload完成继续检验值检查
        }
        else {
            if (msp_message->header.protocol_version == MSP_V1)
            {
                msp_message->checksum ^= byte;
            }
            else if (msp_message->header.protocol_version == MSP_V2_NATIVE) {
                msp_message->checksum = crc8_dvb_s2(msp_message->checksum, byte);
            }
            msp_message->payload[payload_offset++] = byte;
            break;
        }
    }
    case MSP_PAYLOAD:
        if (msp_message->checksum == byte) {
            msp_message->header.status = MSP_IDLE;
            result = MSP_SUCCESS;
        }
        else {
            // 校验和错误
            printf("CHECK_SUM = %02x, byte=%02x\n", msp_message->checksum, byte);
            result = MSP_CHECK_SUM;
        }

        break;
    case MSP_CHECK_SUM:

        break;

    default:
        result = MSP_UNKONW;
        break;
    }

    return result;
}


// 计算校验和
uint8_t calculateChecksum(msp_message_t* message) {
    uint16_t i;
    uint8_t checksum = 0;
    if (message->header.protocol_version == 0) {
        checksum ^= message->payload_size;
        checksum ^= message->command;

        for (i = 0; i < message->payload_size; i++) {
            checksum ^= message->payload[i];
        }
    }
    else if (message->header.protocol_version == 2) {
        // printf("x=%02x, %02x\n", (uint8_t)(message->command >> 8), (uint8_t)message->command);
        checksum = crc8_dvb_s2((uint8_t)message->command, checksum);
        checksum = crc8_dvb_s2((uint8_t)(message->command >> 8), checksum);
        checksum = crc8_dvb_s2((uint8_t)message->payload_size, checksum);
        checksum = crc8_dvb_s2((uint8_t)(message->payload_size >> 8), checksum);

        for (i = 0; i < message->payload_size; i++) {
            checksum = crc8_dvb_s2(message->payload[i], checksum);
        }
        printf("sum=%02x\n", checksum);
    }

    return checksum;
}

// MSP 消息打包
uint16_t packMessage(msp_message_t* message, uint8_t* buffer, uint16_t buffer_size) {
    uint16_t offset = 0;
    printf("ver=%d, len=%u\n", message->header.protocol_version, message->payload_size);
    if (message->header.protocol_version == 0 &&
        buffer_size < 6 + message->payload_size) {
        // 缓冲区太小
        return 0;
    }
    else if (message->header.protocol_version == 1 &&
        buffer_size < 12 + message->payload_size) {
        // 缓冲区太小
        return 0;
    }
    else if (message->header.protocol_version == 2 &&
        buffer_size < 9 + message->payload_size) {
        // 缓冲区太小
        return 0;
    }

    // 写入消息头
    buffer[offset++] = '$';
    if (message->header.protocol_version == MSP_V1)
        buffer[offset++] = 'M';
    if (message->header.protocol_version == MSP_V2_OVER_V1)
        buffer[offset++] = 'M';
    if (message->header.protocol_version == MSP_V2_NATIVE)
        buffer[offset++] = 'X';
    buffer[offset++] = message->header.direction_flag;

    // 写入命令和有效载荷大小
    if (message->header.protocol_version == MSP_V1) {
        buffer[offset++] = message->payload_size;
        buffer[offset++] = message->command;
    }
    else if (message->header.protocol_version == MSP_V2_NATIVE) {
        buffer[offset++] = 0x00; // flag

        buffer[offset++] = message->command;
        buffer[offset++] = (message->command >> 8);

        buffer[offset++] = message->payload_size;
        buffer[offset++] = (message->payload_size >> 8);
    }

    // 写入有效载荷
    for (uint16_t i = 0; i < message->payload_size; i++) {
        buffer[offset++] = message->payload[i];
    }

    // 写入校验和
    buffer[offset++] = calculateChecksum(message);

    return offset;
}




void mspInit(void) {

}