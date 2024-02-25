#include <stdio.h>
#include <string.h>
#include "msp.h"
#define MSP_PROTOCOL_VERSION 1
#define MSP_COMMAND_STATUS 101

int test_v2() {
    uint8_t msp_packet_v2[] = { 0x24, 'X', 0x3C, 0x10, 0x00, MSP_COMMAND_STATUS, 0x00, 0x09, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x76 };

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
}
int test_v1() {
    // 示例 MSP 数据包
    // uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x00, 0x01, 0x01};
    // uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x09, MSP_COMMAND_STATUS, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x66 };
    uint8_t msp_packet_v1[] = { 0x24, 0x4D, '>', 0x03, 0x01, 0x00, 0x01, 46, 0x2D };

    uint8_t payload[255];
    uint16_t command, payload_len;
    msp_message_t msp_message;
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

}

int main() {
    // test_v1();
    // test_v2();

    printf("\n");

    static uint8_t dst[300] = { 0 };
    uint16_t dst_len = 0;
    msp_message_t msp_message;
    msp_message.header.protocol_version = 0;
    msp_message.header.direction_flag = '>';
    msp_message.command = 1;
    // char data[]={ 0x01, 0x02, 0x03 };
    // msp_message.payload = data;
    msp_message.payload = (uint8_t[]){ 0x00, 0x01, 46 };
    msp_message.payload_size = 3;
    for (size_t i = 0; i < msp_message.payload_size; i++)
    {
        printf("%02x ", msp_message.payload[i]);
    }
    printf("\n");
    dst_len = packMessage(&msp_message, dst, sizeof(dst));
    printf("dst_len=%d\n", dst_len);
    for (uint8_t i = 0; i < dst_len; i++)
    {
        printf("%02x ", dst[i]);
    }
    char ff[80]={0};
    // int rt = sprintf(ff, "%s%d", "ss", 5);
    int rt = snprintf(ff, 80, "%u%u%u", (uint8_t)5, (uint8_t)5, (uint8_t)0);
    printf("\n%d %x\n", rt, ff[0]);

    uint32_t payload_len = 0;
    uint8_t payload[100]={0};
#define CONFIG_MSP_NAME "YAMATO-FLIGHT"
        payload_len = strlen(CONFIG_MSP_NAME);
        memcpy(payload, (char *)CONFIG_MSP_NAME, payload_len);
        printf("%s %d\n", payload, payload_len);
        return 0;
}
