#pragma once

typedef void (*callback_t)(uint16_t);

void batteryInit(void);
void batteryDeinit(void);
void battery_register(callback_t callback);