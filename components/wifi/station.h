#pragma once
#include <stdint.h>

void pullux_wifi_init(void);
void pullux_wifi_deinit(void);
void pullux_wifi_set_max_tx_power(int8_t power);
void pullux_wifi_set_ssid_passwd(const char* ssid, const char* password);
void pullux_wifi_restart(void);

