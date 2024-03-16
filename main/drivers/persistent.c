#include "persistent.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "inttypes.h"

static nvs_handle_t nvs_handler;
void persistentObjectInit(void) {
    int ret = nvs_open("storage", NVS_READWRITE, &nvs_handler);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    }
    else {
        printf("Done\n");
    }

}
void persistentObjectDeinit() {
    // Close
    nvs_close(nvs_handler);
}
uint32_t persistentObjectRead(persistentObjectId_e id) {
    char key[25] = { 0 };
    sprintf(key, "nvs_%d", id);
    // Read
    printf("Reading restart counter from NVS ... ");
    int32_t value = 0; // value will default to 0, if not set yet in NVS
    int ret = nvs_get_i32(nvs_handler, key, &value);
    switch (ret) {
    case ESP_OK:
        printf("Done\n");
        printf("Restart counter = %" PRIu32 "\n", value);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        printf("The value is not initialized yet!\n");
        break;
    default:
        printf("Error (%s) reading!\n", esp_err_to_name(ret));
    }
    return value;
}
void persistentObjectWrite(persistentObjectId_e id, uint32_t value)
{
    char key[25] = { 0 };
    sprintf(key, "nvs_%d", id);
    // Write
    printf("Updating restart counter in NVS ... ");
    int ret = nvs_set_i32(nvs_handler, key, value);
    printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS ... ");
    ret = nvs_commit(nvs_handler);
    printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");
}