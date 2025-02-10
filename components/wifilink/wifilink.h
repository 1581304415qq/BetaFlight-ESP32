#pragma once

#include <stdint.h>
#include <string.h>

int wifilink_init(void);
void wifilink_deinit(void);
void wifilink_reset(void);
int wifilink_write(char* data, uint32_t data_len);
uint32_t wifilink_read(char* data, uint32_t data_size);