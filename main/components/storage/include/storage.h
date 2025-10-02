#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "data_model.h" // For structs

// Storage interface functions (matching contract)
esp_err_t storage_init(void);
esp_err_t storage_save_settings(const settings_t *settings);
esp_err_t storage_load_settings(settings_t *settings);
esp_err_t storage_save_print_run(const print_run_t *run);
esp_err_t storage_load_print_run(print_run_t *run);
bool storage_has_saved_data(void);

// Internal functions
esp_err_t nvs_save_blob(const char *key, const void *data, size_t size);
esp_err_t nvs_load_blob(const char *key, void *data, size_t size);

#endif // STORAGE_H
