#include "storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "storage";

// NVS namespace
#define STORAGE_NAMESPACE "insta_press"

// Keys
#define SETTINGS_KEY "settings"
#define PRINT_RUN_KEY "print_run"
#define DATA_FLAG_KEY "has_data"

static nvs_handle_t nvs_handle;

esp_err_t storage_init(void)
{
    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_erase();
        if (ret != ESP_OK)
            return ret;
        ret = nvs_flash_init();
        if (ret != ESP_OK)
            return ret;
    }

    // Open NVS handle
    ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Storage initialized");
    return ESP_OK;
}

esp_err_t storage_save_settings(const settings_t *settings)
{
    if (!settings)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_save_blob(SETTINGS_KEY, settings, sizeof(settings_t));
    if (ret != ESP_OK)
        return ret;

    // Mark that we have data
    uint8_t flag = 1;
    ret = nvs_set_u8(nvs_handle, DATA_FLAG_KEY, flag);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Settings saved");
    return ESP_OK;
}

esp_err_t storage_load_settings(settings_t *settings)
{
    if (!settings)
        return ESP_ERR_INVALID_ARG;

    size_t size = sizeof(settings_t);
    esp_err_t ret = nvs_load_blob(SETTINGS_KEY, settings, &size);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Settings loaded");
    return ESP_OK;
}

esp_err_t storage_save_print_run(const print_run_t *run)
{
    if (!run)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_save_blob(PRINT_RUN_KEY, run, sizeof(print_run_t));
    if (ret != ESP_OK)
        return ret;

    // Mark that we have data
    uint8_t flag = 1;
    ret = nvs_set_u8(nvs_handle, DATA_FLAG_KEY, flag);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Print run saved");
    return ESP_OK;
}

esp_err_t storage_load_print_run(print_run_t *run)
{
    if (!run)
        return ESP_ERR_INVALID_ARG;

    size_t size = sizeof(print_run_t);
    esp_err_t ret = nvs_load_blob(PRINT_RUN_KEY, run, &size);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Print run loaded");
    return ESP_OK;
}

bool storage_has_saved_data(void)
{
    uint8_t flag = 0;
    esp_err_t ret = nvs_get_u8(nvs_handle, DATA_FLAG_KEY, &flag);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
        return false;
    if (ret != ESP_OK)
        return false;
    return flag == 1;
}

esp_err_t nvs_save_blob(const char *key, const void *data, size_t size)
{
    return nvs_set_blob(nvs_handle, key, data, size);
}

esp_err_t nvs_load_blob(const char *key, void *data, size_t size)
{
    size_t actual_size = size;
    return nvs_get_blob(nvs_handle, key, data, &actual_size);
}
