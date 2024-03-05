#include <stdio.h>
#include <string.h>
#include "msp.h"

#define MSP_PROTOCOL_VERSION 1
#define MSP_COMMAND_STATUS 101

int test_v2() {
    // uint8_t msp_packet_v2[] = { 0x24, 'X', 0x3C, 0x10, 0x00, MSP_COMMAND_STATUS, 0x00, 0x09, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x76 };
    uint8_t msp_packet_v2[] = { 0x24, 0x58, 0x3C, 0x00, 0x06, 0x30, 0x01, 0x00, 0x02, 0xc5};

    uint8_t payload[255];
    uint16_t command, payload_len;
    msp_message_t msp_message;
    int ret = parse_msp_packet(msp_packet_v2, sizeof(msp_packet_v2), &msp_message.header, &command, &payload_len, payload);
    printf("parse v2 ret=%d\n", ret);
    if (ret == 0) {
        printf("MSP 版本: 0x%02X\n", msp_message.header.protocol_version);
        printf("MSP 命令: 0x%02X\n", command);
        printf("有效载荷长度: %d\n", payload_len);
        printf("有效载荷: ");
        for (uint8_t i = 0; i < payload_len; i++) {
            printf("0x%02X ", payload[i]);
        }
        printf("\n");
    }
    else {
        printf("解析 MSP 数据包失败\n");
    }

    uint8_t reply[256] = { 0 };
    msp_message.payload_size = payload_len;
    memcpy(msp_message.payload , payload, payload_len);
    msp_message.command = command;
    ret = packMessage(&msp_message, reply, 255);
    printf("pack ret=%d\n", ret);
    for (int i = 0; i < ret; i++)
    {
        printf("%02X ", reply[i]);
    }
    
}

int test_v1() {
    // 示例 MSP 数据包
    uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x00, 0x01, 0x01};
    // uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x09, MSP_COMMAND_STATUS, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x66 };
    // uint8_t msp_packet_v1[] = { 0x24, 0x4D, '>', 0x03, 0x01, 0x00, 0x01, 46, 0x2D };

    uint8_t payload[255];
    uint16_t command, payload_len;
    msp_message_t msp_message={0};
    int ret = parse_msp_packet(msp_packet_v1, sizeof(msp_packet_v1), &msp_message.header, &command, &payload_len, payload);
    printf("parse ret=%d\n", ret);
    if (ret == 0) {
        printf("MSP 版本: 0x%02X\n", msp_message.header.protocol_version);
        printf("MSP 命令: 0x%02X\n", command);
        printf("有效载荷长度: %d\n", payload_len);
        printf("有效载荷: ");
        for (uint8_t i = 0; i < payload_len; i++) {
            printf("0x%02X ", payload[i]);
        }
        printf("\n");
    }
    else {
        printf("解析 MSP 数据包失败\n");
    }

    uint8_t reply[256] = { 0 };
    msp_message.payload_size = payload_len;
    memcpy(msp_message.payload , payload, payload_len);
    msp_message.command = command;
    ret = packMessage(&msp_message, reply, 255);
    printf("pack ret=%d\n", ret);
    for (int i = 0; i < ret; i++)
    {
        printf("%02X ", reply[i]);
    }
}


int test_msp() {
#define MAX_QUEUE_SIZE 200
    Queue* queue = createQueue(MAX_QUEUE_SIZE);

    // 示例 MSP 数据包
    uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x00, 0x01, 0x01, 0x24, 0x4D, 0x3C, 0x00, 0x01, 0x01, 0x24, 0x4D };
    uint8_t msp_packet_v2[] = { 0x24, 0x58, 0x3C, 0x00, 0x06, 0x30, 0x01, 0x00, 0x02, 0xc5 };

    for (int i = 0; i < sizeof(msp_packet_v1); i++)
    {
        enqueue(queue, msp_packet_v1[i]);
    }

    int size = getSize(queue);
    printf("len=%d\n", size);

    msp_message_t msp_message = { 0 };

    while (size > 0)
    {
        int ret = parse_msp_mechine(queue, &msp_message);

        size = getSize(queue);
        printf("parse ret=%d, len=%d\n", ret, size);

        if (MSP_SUCCESS == ret) {
            printf("解析成功\n");
            printf("%c %c %c %d %x %d\n",
                msp_message.header.start_byte,
                msp_message.header.message_type,
                msp_message.header.direction_flag,
                msp_message.header.protocol_version,
                msp_message.command,
                msp_message.payload_size
            );

            uint8_t reply[256] = { 0 };
            ret = packMessage(&msp_message, reply, 255);
            printf("pack ret=%d\n", ret);
            for (int i = 0; i < ret; i++)
            {
                printf("%02X ", reply[i]);
            }

            memset(&msp_message, 0, sizeof(msp_message_t));
        }
    }
    return 0;
}


int main() {
    test_v1();
    printf("\n");
    test_v2();
    printf("\n");
    int test_msp();
    return 0;
}
