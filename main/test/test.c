#include <stdio.h>
#include <string.h>
#include "msp.h"
#include "queue.h"


int test_msp() {
#define MAX_QUEUE_SIZE 200
    Queue* queue = createQueue(MAX_QUEUE_SIZE);

    // 示例 MSP 数据包
    uint8_t msp_packet_v1[] = { 0x24, 0x4D, 0x3C, 0x00, 0x01, 0x01, 0x24, 0x4D };
    for (int i = 0; i < 8; i++)
    {
        enqueue(queue, msp_packet_v1[i]);
    }

    uint8_t packet[256], payload[256];
    int size = getSize(queue);
    printf("len=%d\n", size);
    msp_message_t msp_message = { 0 };
    uint16_t command = 0, payload_len = 0;
    if (size >= 6) {
        for (int i = 0; i < size; i++)
        {
            packet[i] = dequeue(queue);
        }

        int ret = parse_msp_packet(packet, size, &msp_message.header, &command, &payload_len, payload);
        printf("parse ret=%d\n", ret);
        if (ret == 0) {
            popqueue(queue, payload_len + 6);
        }

    }

    size = getSize(queue);
    printf("len=%d\n", size);

    // uint8_t item;
    // for (int i = 0; i < 3; i++)
    //     dequeue(queue);
    // size = getSize(queue);
    // printf("len=%d\n", size);

    return 0;
}

int main() {
    test_msp();

    return 0;
}