#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "msp.h"

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

    // <size>, <command> and each data byte into a zero'ed sum
    uint8_t checksum = 0;
    for (uint8_t i = 3; i < packet_len - 1; i++) {
        checksum ^= packet[i];
    }

    if (checksum != packet[packet_len - 1]) {
        // 校验和错误
        printf("check_sum=%x\n", checksum);
        return 4;
    }

    // 检查V1 or V2_Over_V1
#define MSP_V2_FRAME_ID 255
    if (packet[3] >= 6 && packet[4] == MSP_V2_FRAME_ID) {
        packet_version = MSP_V2_OVER_V1;
    }
    header->protocol_version = packet_version;

    if (packet_version == MSP_V1)
    {
        *payload_len = packet[3];
        *command = packet[4];

        if (*payload_len + 6 != packet_len) {
            // 数据包长度错误
            printf("payload %d %d\n", *payload_len, packet_len);
            return 5;
        }

        for (uint8_t i = 0; i < *payload_len; i++) {
            payload[i] = packet[5 + i];
        }
    }
    else if (packet_version == MSP_V2_NATIVE) {
        uint8_t flag = packet[3];
        *command = packet[4] << 8 | packet[5];
        *payload_len = packet[6] << 8 | packet[7];

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


    return 0;
}

// 计算校验和
uint8_t calculateChecksum(msp_message_t* message) {
    uint16_t i;
    uint8_t checksum = 0;
    // uint8_t* ptr = (uint8_t*)&message->header.direction_flag;

    // for (i = 0; i < sizeof(msp_header_t) + sizeof(message->command) + sizeof(message->payload_size); i++) {
    //     checksum ^= *ptr++;
    // }
    checksum ^= message->payload_size;
    checksum ^= message->command;

    if (message->payload) {
        for (i = 0; i < message->payload_size; i++) {
            checksum ^= message->payload[i];
        }
    }

    return checksum;
}

// MSP 消息打包
uint16_t packMessage(msp_message_t* message, uint8_t* buffer, uint16_t buffer_size) {
    uint16_t offset = 0;

    if (buffer_size < sizeof(msp_header_t) + sizeof(message->command) + sizeof(message->payload_size) + message->payload_size + 1) {
        // 缓冲区太小
        return 0;
    }

    // 写入消息头
    buffer[offset++] = '$';
    buffer[offset++] = 'M';
    buffer[offset++] = message->header.direction_flag;

    // 写入命令和有效载荷大小
    if (message->header.protocol_version == 0) {
        buffer[offset++] = message->payload_size;
        buffer[offset++] = message->command;
    }
    else if (message->header.protocol_version == 1) {
        buffer[offset++] = (message->payload_size >> 8) & 0xFF;
        buffer[offset++] = message->payload_size & 0xFF;

        buffer[offset++] = (message->command >> 8) & 0xFF;
        buffer[offset++] = message->command & 0xFF;
    }
    // 写入有效载荷
    if (message->payload) {
        for (uint16_t i = 0; i < message->payload_size; i++) {
            buffer[offset++] = message->payload[i];
        }
    }

    // 写入校验和
    buffer[offset++] = calculateChecksum(message);

    return offset;
}

void mspInit(void) {

}