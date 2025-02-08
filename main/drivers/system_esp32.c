#include "system.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "inttypes.h"
#include "serial.h"

void systemInit(void) {
    esp_err_t ret;

    //-------------STORAGE---------------//
    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    serialInit();
}