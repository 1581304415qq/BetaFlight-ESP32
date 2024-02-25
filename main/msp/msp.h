#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MSP_V1          = 0,
    MSP_V2_OVER_V1  = 1,
    MSP_V2_NATIVE   = 2,
    MSP_VERSION_COUNT
} mspVersion_e;

// MSP 消息头结构体
typedef struct {
    uint8_t start_byte;       // 固定为 '$'
    uint8_t message_type;     // 固定为 'M'
    uint8_t direction_flag;   // '<' 表示从飞控发送到地面站, '>' 表示从地面站发送到飞控
    mspVersion_e protocol_version; // 协议版本号
} __attribute__((packed)) msp_header_t;

// MSP 消息结构体
typedef struct {
    msp_header_t header;
    uint8_t command;          // MSP 命令码
    uint8_t payload_size;    // 有效载荷大小
    uint8_t *payload;         // 指向有效载荷数据的指针
    uint8_t checksum;         // 校验和
} __attribute__((packed)) msp_message_t;

// MSPV2 消息结构体
typedef struct {
    msp_header_t header;
    uint16_t command;          // MSP 命令码
    uint16_t payload_size;    // 有效载荷大小
    uint8_t *payload;         // 指向有效载荷数据的指针
    uint8_t checksum;         // 校验和
} __attribute__((packed)) mspv2_message_t;


void mspInit(void);
int parse_msp_packet(const uint8_t* packet, uint8_t packet_len, msp_header_t *header, uint16_t* command, uint16_t* payload_len, uint8_t* payload);
uint16_t packMessage(msp_message_t* message, uint8_t* buffer, uint16_t buffer_size);