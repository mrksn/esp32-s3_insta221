#include "storage_contract.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "storage";

// NVS namespace
#define NVS_NAMESPACE "insta_retrofit"

// NVS keys
#define NVS_KEY_SETTINGS "settings"
#define NVS_KEY_PRINT_RUN "print_run"

static nvs_handle_t my_nvs_handle;

esp_err_t storage_init(void)
{
    ESP_LOGI(TAG, "Initializing storage system");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    // Open NVS handle
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Storage system initialized successfully");
    return ESP_OK;
}

esp_err_t storage_save_settings(const settings_t *settings)
{
    if (!settings)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_set_blob(my_nvs_handle, NVS_KEY_SETTINGS, settings, sizeof(settings_t));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save settings: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(my_nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit settings: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Settings saved successfully");
    return ESP_OK;
}

esp_err_t storage_load_settings(settings_t *settings)
{
    if (!settings)
        return ESP_ERR_INVALID_ARG;

    size_t required_size = sizeof(settings_t);
    esp_err_t ret = nvs_get_blob(my_nvs_handle, NVS_KEY_SETTINGS, settings, &required_size);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to load settings: %s", esp_err_to_name(ret));
        return ret;
    }

    if (required_size != sizeof(settings_t))
    {
        ESP_LOGW(TAG, "Settings size mismatch, using defaults");
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    ESP_LOGI(TAG, "Settings loaded successfully");
    return ESP_OK;
}

esp_err_t storage_save_print_run(const print_run_t *run)
{
    if (!run)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_set_blob(my_nvs_handle, NVS_KEY_PRINT_RUN, run, sizeof(print_run_t));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save print run: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(my_nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit print run: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Print run saved successfully");
    return ESP_OK;
}

esp_err_t storage_load_print_run(print_run_t *run)
{
    if (!run)
        return ESP_ERR_INVALID_ARG;

    size_t required_size = sizeof(print_run_t);
    esp_err_t ret = nvs_get_blob(my_nvs_handle, NVS_KEY_PRINT_RUN, run, &required_size);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to load print run: %s", esp_err_to_name(ret));
        return ret;
    }

    if (required_size != sizeof(print_run_t))
    {
        ESP_LOGW(TAG, "Print run size mismatch, using defaults");
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    ESP_LOGI(TAG, "Print run loaded successfully");
    return ESP_OK;
}

bool storage_has_saved_data(void)
{
    size_t required_size;
    esp_err_t ret = nvs_get_blob(my_nvs_handle, NVS_KEY_SETTINGS, NULL, &required_size);
    return ret == ESP_OK && required_size == sizeof(settings_t);
}
