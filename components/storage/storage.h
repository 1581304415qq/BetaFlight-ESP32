#pragma once
#include <stdint.h>

void storage_init(void);
int storage_read(const char* key, uint8_t* value, uint32_t value_length);
int storage_write(const char* key, uint8_t* value, uint8_t value_length);
int storage_clean_key(const char* key);

char* read_file(const char* filename, uint32_t offset, uint32_t* length, int try_count);
int writeFile(const char* filename, const char* str, uint32_t length);
