#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool ota_ready(uint32_t image_size);
void ota_end(bool isRestart);
void ota(const char* ota_write_data, size_t data_size);
