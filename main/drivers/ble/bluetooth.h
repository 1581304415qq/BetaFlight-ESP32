#pragma once
#include <stdint.h>

typedef enum {
    CHANNEL_1,
    CHANNEL_2,
    CHANNEL_3,
} btServiceChannel_e;

typedef void BtReceviveFn(btServiceChannel_e channel, const uint8_t data, uint32_t len);

void initBluetooth();
void receiveByBt(BtReceviveFn callback);
void sendByBt(const char* data, uint32_t len);
void BT_LOG(const char* fmt, ...);