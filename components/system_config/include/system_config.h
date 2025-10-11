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
#include <stdbool.h>

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
        float max_temperature_celsius;   ///< Maximum safe temperature (°C)
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
        float hysteresis_celsius;             ///< Temperature hysteresis for control (°C)
        float pressing_max_offset_celsius;    ///< Max temp offset during pressing (°C)
        float cycle_start_max_offset_celsius; ///< Max temp offset for cycle start (°C)
        float cycle_start_min_celsius;        ///< Minimum temperature for cycle start (°C)
        float recovery_offset_celsius;        ///< Temperature offset for error recovery (°C)
        float min_for_pressing_celsius;       ///< Minimum temperature to allow pressing (°C)
        float ready_threshold_celsius;        ///< Temperature threshold to consider heat-up complete (°C)
        float calibration_offset_celsius;     ///< Temperature calibration offset for thermocouple (°C)
    } temperature;

    // Sensor retry configuration
    struct
    {
        uint8_t retry_count;     ///< Number of sensor read retry attempts
        uint32_t retry_delay_ms; ///< Delay between retry attempts (ms)
    } sensor;

    // Heat-up display configuration
    struct
    {
        float min_temp_change_celsius;  ///< Minimum temp change before calculating ETA (°C)
        uint32_t min_elapsed_time_sec;  ///< Minimum elapsed time before calculating ETA (s)
        float min_heating_rate;         ///< Minimum heating rate for valid ETA (°C/s)
    } heat_up;

    // Simulation mode configuration
    struct
    {
        bool enabled; ///< Enable simulation mode (true = simulated, false = real hardware)
    } simulation;
} system_config_t;

// =============================================================================
// Global Configuration Instance
// =============================================================================

/**
 * @brief Global system configuration
 *
 * This constant structure contains all system configuration values.
 * Modify the values in system_config.c to tune system behavior.
 *
 * IMPORTANT: To enable/disable simulation mode, edit system_config.c
 */
extern const system_config_t SYSTEM_CONFIG;

// =============================================================================
// Legacy Compatibility Macros (for backward compatibility)
// =============================================================================

// Task Configuration
#define UI_TASK_STACK_SIZE 4096
#define TEMP_CONTROL_TASK_STACK_SIZE 4096
#define WATCHDOG_TASK_STACK_SIZE 2048
#define UI_TASK_PRIORITY 5
#define TEMP_CONTROL_TASK_PRIORITY 4
#define WATCHDOG_TASK_PRIORITY 3

// Safety Limits
#define MAX_TEMPERATURE (SYSTEM_CONFIG.safety.max_temperature_celsius)
#define MAX_CYCLE_TIME (SYSTEM_CONFIG.safety.max_cycle_time_seconds)
#define HEAP_MINIMUM (SYSTEM_CONFIG.safety.heap_minimum_bytes)

// Timing Constants
#define UI_TASK_TIMEOUT_SEC (SYSTEM_CONFIG.timing.ui_task_timeout_sec)
#define TEMP_TASK_TIMEOUT_SEC (SYSTEM_CONFIG.timing.temp_task_timeout_sec)
#define SENSOR_TIMEOUT_SEC (SYSTEM_CONFIG.timing.sensor_timeout_sec)
#define SENSOR_VALIDATION_TIMEOUT_SEC (SYSTEM_CONFIG.timing.sensor_validation_timeout_sec)

// Temperature Constants
#define TEMP_HYSTERESIS (SYSTEM_CONFIG.temperature.hysteresis_celsius)
#define TEMP_PRESSING_MAX_OFFSET (SYSTEM_CONFIG.temperature.pressing_max_offset_celsius)
#define TEMP_CYCLE_START_MAX_OFFSET (SYSTEM_CONFIG.temperature.cycle_start_max_offset_celsius)
#define TEMP_CYCLE_START_MIN (SYSTEM_CONFIG.temperature.cycle_start_min_celsius)
#define TEMP_RECOVERY_OFFSET (SYSTEM_CONFIG.temperature.recovery_offset_celsius)
#define TEMP_MIN_FOR_PRESSING (SYSTEM_CONFIG.temperature.min_for_pressing_celsius)
#define HEAT_UP_TEMP_READY_THRESHOLD (SYSTEM_CONFIG.temperature.ready_threshold_celsius)

// Sensor Constants
#define SENSOR_RETRY_COUNT (SYSTEM_CONFIG.sensor.retry_count)
#define SENSOR_RETRY_DELAY_MS (SYSTEM_CONFIG.sensor.retry_delay_ms)

// Heat-up Display Constants
#define HEAT_UP_MIN_TEMP_CHANGE (SYSTEM_CONFIG.heat_up.min_temp_change_celsius)
#define HEAT_UP_MIN_ELAPSED_TIME (SYSTEM_CONFIG.heat_up.min_elapsed_time_sec)
#define HEAT_UP_MIN_HEATING_RATE (SYSTEM_CONFIG.heat_up.min_heating_rate)

// Default Values
#define DEFAULT_TEMPERATURE 25.0f

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

// =============================================================================
// Configuration Validation Functions
// =============================================================================

/**
 * @brief Validate system configuration
 *
 * Checks that all configuration values are within acceptable ranges.
 * This should be called at system startup to ensure configuration integrity.
 *
 * @return true if configuration is valid, false otherwise
 */
bool system_config_validate(void);

/**
 * @brief Get configuration validation error message
 *
 * Returns a human-readable description of any configuration errors found
 * during validation.
 *
 * @return Error message string, or NULL if no errors
 */
const char* system_config_get_error(void);

/**
 * @brief Print current configuration to log
 *
 * Outputs all configuration parameters to the ESP log for debugging.
 */
void system_config_print(void);

#endif // SYSTEM_CONFIG_H
