#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "storage.h"


static nvs_handle_t nvs_handle_id;


int storage_init()
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open("storage", NVS_READWRITE, &nvs_handle_id);
    if (ret != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        return -1;
    }
    else
    {
        printf("Storage is Ready\n");
    }
    return 0;
}

void storage_destory()
{
    // Close
    nvs_close(nvs_handle_id);
}

int storage_read(const char* key, uint8_t* value, uint32_t value_length)
{
    if (value_length == 4)
    {
        int32_t temp = 0;
        if (nvs_get_i32(nvs_handle_id, key, &temp) == ESP_OK)
            memcpy(value, &temp, value_length);
        else
        {
            return -1;
        }
    }
    else
    {
        size_t length = value_length;
        esp_err_t err = nvs_get_str(nvs_handle_id, key, (char*)value, &length);

        if (err == ESP_FAIL)
        {
            printf("ESP_FAIL\n");
        }
        else if (err == ESP_ERR_NVS_INVALID_HANDLE)
        {
            printf("ESP_ERR_NVS_INVALID_HANDLE\n");
        }
        else if (err == ESP_ERR_NVS_INVALID_LENGTH)
        {
            printf("ESP_ERR_NVS_INVALID_LENGTH\n");
        }
        else if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            printf("ESP_ERR_NVS_NOT_FOUND\n");
            return 2;
        }
        value[length] = 0;
        printf("storage get key=%s length=%d value=%s\n", key, length, value);
    }
    return 0;
}

int storage_write(const char* key, uint8_t* value, uint8_t value_length)
{
    if (strlen(key) > (NVS_KEY_NAME_MAX_SIZE - 1))
        return 1;
    esp_err_t ret;
    if (value_length == 4)
    {
        int32_t temp = 0;
        memcpy(&temp, value, 4);
        if (nvs_set_i32(nvs_handle_id, key, temp) != ESP_OK)
        {
            return -1;
        }
    }
    else
    {
        if (nvs_set_str(nvs_handle_id, key, (char*)value) != ESP_OK)
        {
            return -1;
        }
    }
    ret = nvs_commit(nvs_handle_id);
    printf("Storage wrie=%s", (ret != ESP_OK) ? "Failed!\n" : "Done\n");
    return 0;
}

int storage_clean_key(const char* key)
{
    if (nvs_erase_key(nvs_handle_id, key) != ESP_OK)
        return -1;
    return 0;
}
