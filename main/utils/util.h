#pragma once

#include <stdint.h>

#define DelayMs(ms)     vTaskDelay(ms / portTICK_PERIOD_MS)

uint8_t readU8(uint8_t* src, uint16_t offset);
uint16_t readU16(uint8_t* src, uint16_t offset);
uint32_t readU32(uint8_t* src, uint16_t offset);
