#include "util.h"

uint8_t readU8(uint8_t* src, uint16_t offset)
{
    return *(src + offset);
}

uint16_t readU16(uint8_t* src, uint16_t offset)
{
    uint16_t ret;
    ret = readU8(src, offset);
    ret |= readU8(src, offset) << 8;
    return ret;
}

uint32_t readU32(uint8_t* src, uint16_t offset)
{
    uint32_t ret;
    ret = readU8(src, offset);
    ret |= readU8(src, offset) << 8;
    ret |= readU8(src, offset) << 16;
    ret |= readU8(src, offset) << 24;
    return ret;
}