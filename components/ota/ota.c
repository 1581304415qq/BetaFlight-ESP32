#include "ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "OTA"

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    esp_restart();
    while (1) {
        ;
    }
}

esp_ota_handle_t update_handle = 0;
esp_partition_t* update_partition = NULL;
uint32_t offset = 0;

/*
* OTA前准备,不知道升级文件大小可设置0,将擦除ota分区
*/
bool ota_ready(uint32_t image_size)
{
    esp_err_t err;

    // 输出基本信息
    uint8_t app_partition_count = esp_ota_get_app_partition_count();
    ESP_LOGI(TAG, "App partition count %d", app_partition_count);
    esp_partition_t* running_partition = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition subtype %d at offset 0x%"PRIx32,
        running_partition->subtype, running_partition->address);

    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition %s subtype %d size %lu at offset 0x%"PRIx32,
        update_partition->label,
        update_partition->subtype, update_partition->size, update_partition->address
    );

    err = esp_ota_begin(update_partition, image_size, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));

        esp_ota_abort(update_handle);
        task_fatal_error();
        return false;
    }

    offset = 0;
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
    return true;
}

void ota_end(bool isRestart)
{
    esp_err_t err;

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    if (isRestart) esp_restart();
}

void ota(const char* ota_write_data, size_t data_size)
{
    esp_err_t err = esp_ota_write_with_offset(update_handle, (const void*)ota_write_data, data_size, offset);
    if (err != ESP_OK) {
        esp_ota_abort(update_handle);
        task_fatal_error();
    }
    offset += data_size;
    ESP_LOGD(TAG, "ota write %d, offset=%lu", data_size, offset);
}