#ifndef HEATING_H
#define HEATING_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "heating_contract.h" // For structs

// Heating interface functions (matching contract)
esp_err_t heating_init(void);
void heating_set_power(uint8_t power_percent);
void heating_emergency_shutoff(void);
bool heating_is_active(void);
void pid_init(pid_config_t config);
float pid_update(float current_temp);

// Internal functions
void ssr_set_power(uint8_t power);
void pid_reset(void);

#endif // HEATING_H
