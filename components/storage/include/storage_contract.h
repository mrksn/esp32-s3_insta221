// storage_contract.h - Persistent Storage Interface
#ifndef STORAGE_CONTRACT_H
#define STORAGE_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "data_model.h" // Include data structures

// Initialize storage
esp_err_t storage_init(void);

// Save settings
esp_err_t storage_save_settings(const settings_t *settings);

// Load settings
esp_err_t storage_load_settings(settings_t *settings);

// Save print run
esp_err_t storage_save_print_run(const print_run_t *run);

// Load print run
esp_err_t storage_load_print_run(print_run_t *run);

// Check if data exists
bool storage_has_saved_data(void);

#endif // STORAGE_CONTRACT_H
