#ifndef _STORAGE_H
#define _STORAGE_H

#include <inttypes.h>

#define SSID        "ssid"
#define PASSWORD    "password"

int storage_init();
void storage_destory();
int storage_read(const char* key, uint8_t* value, uint32_t value_length);
int storage_write(const char* key, uint8_t* value, uint8_t value_length);
int storage_clean_key(const char* key);

#endif