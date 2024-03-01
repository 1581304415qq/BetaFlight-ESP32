#include <stdio.h>
#include <string.h>
#include "msp.h"
#include "queue.h"


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
    test_msp();

    return 0;
}