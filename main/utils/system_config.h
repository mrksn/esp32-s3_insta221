/**
 * @file system_config.h
 * @brief Unified system configuration and constants
 *
 * This header centralizes all system configuration parameters including
 * safety limits, timing parameters, and temperature thresholds. This
 * provides a single source of truth for all system constraints.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>

// =============================================================================
// Configuration Structure
// =============================================================================

/**
 * @brief System configuration structure
 *
 * Contains all configurable system parameters organized by category.
 * These values are initialized as constants and exposed for testing.
 */
typedef struct
{
    // Safety limits
    struct
    {
        float max_temperature_celsius;  ///< Maximum safe temperature (°C)
        uint32_t max_cycle_time_seconds; ///< Maximum pressing cycle time (s)
        uint32_t heap_minimum_bytes;     ///< Minimum free heap memory (bytes)
    } safety;

    // Timing parameters
    struct
    {
        uint32_t ui_task_timeout_sec;           ///< UI task watchdog timeout (s)
        uint32_t temp_task_timeout_sec;         ///< Temperature task watchdog timeout (s)
        uint32_t sensor_timeout_sec;            ///< Maximum time without sensor reading (s)
        uint32_t sensor_validation_timeout_sec; ///< Sensor timeout for cycle validation (s)
    } timing;

    // Temperature thresholds
    struct
    {
        float hysteresis_celsius;          ///< Temperature hysteresis for control (°C)
        float pressing_max_offset_celsius; ///< Max temp offset during pressing (°C)
        float cycle_start_max_offset_celsius; ///< Max temp offset for cycle start (°C)
        float cycle_start_min_celsius;     ///< Minimum temperature for cycle start (°C)
        float recovery_offset_celsius;     ///< Temperature offset for error recovery (°C)
    } temperature;

    // Sensor retry configuration
    struct
    {
        uint8_t retry_count;           ///< Number of sensor read retry attempts
        uint32_t retry_delay_ms;       ///< Delay between retry attempts (ms)
    } sensor;

    // Simulation mode configuration
    struct
    {
        bool enabled;                  ///< Enable simulation mode (true = simulated, false = real hardware)
    } simulation;
} system_config_t;

// =============================================================================
// Global Configuration Instance
// =============================================================================

/**
 * @brief Global system configuration
 *
 * This constant structure contains all system configuration values.
 * Modify these values to tune system behavior.
 */
static const system_config_t SYSTEM_CONFIG = {
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
    },
    .sensor = {
        .retry_count = 3,
        .retry_delay_ms = 500,
    },
    .simulation = {
        .enabled = false,  ///< Set to true to enable simulation mode for SSR testing
    },
};

// =============================================================================
// UI Configuration Constants
// =============================================================================

#define NUM_SHIRTS_MIN 1
#define NUM_SHIRTS_MAX 999
#define TEMPERATURE_MIN_CELSIUS 0
#define TEMPERATURE_MAX_CELSIUS 250
#define PID_PARAMETER_MIN 0
#define PID_PARAMETER_MAX 1000
#define PID_SCALE_FACTOR 100
#define STAGE_DURATION_MIN_SECONDS 1
#define STAGE_DURATION_MAX_SECONDS 300

#define JOB_SETUP_ITEM_COUNT 2
#define JOB_ITEM_NUM_SHIRTS 0
#define JOB_ITEM_PRINT_TYPE 1

#define HEATING_POWER_MAX_PERCENT 100
#define HEATING_POWER_MIN_PERCENT 0

#define PID_MIN_UPDATE_INTERVAL_MS 100

// =============================================================================
// Helper Macros
// =============================================================================

#define CLAMP(value, min, max) \
    ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

#define MENU_WRAP(value, count) \
    (((value) % (count) + (count)) % (count))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif // SYSTEM_CONFIG_H
