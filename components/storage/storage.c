#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_spiffs.h"
#include "esp_partition.h"

#define TAG "Storage"
static void partition_init(void);


static bool isStart = false;
static nvs_handle_t nvs_handle_id;
void storage_init(void)
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
    }
    else
    {
        printf("Storage is Ready\n");
    }


    partition_init();

    isStart = true;
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


static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = "html",
    .max_files = 10,
    .format_if_mount_failed = true
};


static void partition_init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    }
    else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        }
        else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }
    isStart = true;
}


static long getFileSize(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        return -1;  // 获取文件信息失败
    }
    return st.st_size;
}


/**
 *@brief 文件读取
 *
 * @param filename 文件名
 * @param offset   读取位置
 * @param str      存储位置
 * @param str_size 存储大小
 * @return uint32_t 读取长度
 */
static int32_t readFile(const char* filename, uint32_t offset, char* str, uint32_t str_size)
{
    long fsize = getFileSize(filename);
    if (fsize < 0) {
        printf("%s文件不存在\n", filename);
        return -1;
    }
    ESP_LOGI(TAG, "读取文件大小:%lu\n", fsize);
    if ((fsize - offset) > str_size) {
        printf("缓存太小不小于%lu\n", fsize - offset);
        return 0;
    }

    FILE* fp = fopen(filename, "r");
    // 判断文件是否打开成功
    if (fp == NULL && fseek(fp, offset, SEEK_SET) != 0) {
        printf("打开文件失败!\n");
        return 0;
    }

    int32_t ret = fread(str, sizeof(char), fsize - offset, fp);
    str[ret] = '\0';
    // 关闭文件
    fclose(fp);
    return ret;
}

char* read_file(const char* filename, uint32_t offset, uint32_t* length, int try_count)
{
    ESP_LOGI(TAG, "read file:%s",filename);
    char* buffer = NULL;
    while (try_count > 0)
    {
        long fsize = getFileSize(filename) + 1;
        buffer = malloc(fsize);
        if (buffer == NULL) {
            ESP_LOGE(TAG, "Failed to apply for memory %ld", fsize);
            return NULL;
        }
        int32_t ret = readFile(filename, 0, buffer, fsize); // 读取文件
        if (ret > 0) {
            *length = fsize;
            return buffer;
        }
        else
            try_count++;
    }
    free(buffer);
    return NULL;
}

int writeFile(const char* filename, const char* str, uint32_t length)
{
    return 0;
}
