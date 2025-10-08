/**
 * @file system_config.c
 * @brief System configuration implementation
 *
 * This file contains the actual system configuration instance.
 * The header file only declares it as extern.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "system_config.h"

/**
 * @brief Global system configuration instance
 *
 * This constant structure contains all system configuration values.
 * Modify these values to tune system behavior.
 */
const system_config_t SYSTEM_CONFIG = {
    .safety = {
        .max_temperature_celsius = 220.0f,
        .max_cycle_time_seconds = 300,
        .heap_minimum_bytes = 8192,
    },
    .timing = {
        .ui_task_timeout_sec = 1,
        .temp_task_timeout_sec = 3,
        .sensor_timeout_sec = 30,
        .sensor_validation_timeout_sec = 10,
    },
    .temperature = {
        .hysteresis_celsius = 5.0f,
        .pressing_max_offset_celsius = 20.0f,
        .cycle_start_max_offset_celsius = 30.0f,
        .cycle_start_min_celsius = 20.0f,
        .recovery_offset_celsius = 10.0f,
        .calibration_offset_celsius = 0.0f,
    },
    .sensor = {
        .retry_count = 3,
        .retry_delay_ms = 500,
    },
    .simulation = {
        .enabled = false, ///< Set to true to enable simulation mode for SSR testing
    },
};
