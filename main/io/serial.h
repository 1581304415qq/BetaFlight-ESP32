#pragma once 

void serialInit();
esp_err_t serialWrite(const char* buffer, uint32_t len);