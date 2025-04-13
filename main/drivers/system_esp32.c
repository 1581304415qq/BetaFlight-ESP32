#include "system.h"
#include "storage.h"
#include "station.h"
#include "serial.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "tcp_server.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2 
#include "ble.h"
#endif

void systemInit(void) {
    int ret;
    
    ret = esp_timer_init();

    storage_init();

#ifndef CONFIG_IDF_TARGET_ESP32S2 
    // Initialize Ble
    initBluetooth();
#endif

    pullux_wifi_init();
    
    // ret = serialInit();
}

void systemDeinit(void) {
    
}