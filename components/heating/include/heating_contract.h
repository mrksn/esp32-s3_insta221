// heating_contract.h - Heating Control Interface
#ifndef HEATING_CONTRACT_H
#define HEATING_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Initialize heating system
esp_err_t heating_init(void);

// Set heater power (0-100%)
void heating_set_power(uint8_t power_percent);

// Emergency shutoff
void heating_emergency_shutoff(void);

// Check if heating is active
bool heating_is_active(void);

// PID control interface
typedef struct
{
    float kp;
    float ki;
    float kd;
    float setpoint;
    float output_min;
    float output_max;
} pid_config_t;

// Initialize PID controller
void pid_init(pid_config_t config);

// Update PID with current temperature
float pid_update(float current_temp);

#endif // HEATING_CONTRACT_H
