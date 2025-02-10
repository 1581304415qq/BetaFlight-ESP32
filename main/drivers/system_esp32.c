#include "system.h"
#include "storage.h"
#include "wifilink.h"
#include "serial.h"
#include "sdkconfig.h"

#include "wifilink.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2 
#include "ble.h"
#endif

void systemInit(void) {
    int ret;

    ret = storage_init();

#ifndef CONFIG_IDF_TARGET_ESP32S2 
    // Initialize Ble
    initBluetooth();
#endif

    ret = wifilink_init();

    ret = serialInit();
}