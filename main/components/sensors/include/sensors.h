#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Sensor interface functions (matching contract)
esp_err_t sensor_init(void);
bool sensor_read_temperature(float *temperature);
bool sensor_is_operational(void);

// Internal functions
esp_err_t max31855_init(void);
esp_err_t max31855_read_raw(uint32_t *data);
float max31855_convert_temperature(uint32_t raw_data);
bool max31855_check_faults(uint32_t raw_data);

#endif // SENSORS_H
