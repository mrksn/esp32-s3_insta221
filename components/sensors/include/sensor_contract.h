// sensor_contract.h - Temperature Sensor Interface
#ifndef SENSOR_CONTRACT_H
#define SENSOR_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Initialize sensor
esp_err_t sensor_init(void);

// Read temperature in Celsius
// Returns true on success, false on failure
bool sensor_read_temperature(float *temperature);

// Check if sensor is operational
bool sensor_is_operational(void);

// Simulation mode functions
bool sensor_is_simulation_mode(void);
void sensor_sim_set_heating_power(float power_percent);

#endif // SENSOR_CONTRACT_H
