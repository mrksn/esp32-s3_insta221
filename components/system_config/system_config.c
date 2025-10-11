/**
 * @file system_config.c
 * @brief System configuration implementation
 *
 * This file contains the actual system configuration instance and
 * validation functions.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "system_config.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "system_config";
static const char *validation_error = NULL;

/**
 * @brief Global system configuration instance
 *
 * This constant structure contains all system configuration values.
 * Modify these values to tune system behavior.
 *
 * SAFETY CRITICAL: Changes to these values directly affect system safety.
 * Review all modifications carefully.
 */
const system_config_t SYSTEM_CONFIG = {
    .safety = {
        .max_temperature_celsius = 220.0f,    // Maximum safe operating temperature
        .max_cycle_time_seconds = 300,        // 5 minute maximum cycle time
        .heap_minimum_bytes = 8192,           // 8KB minimum free heap
    },
    .timing = {
        .ui_task_timeout_sec = 1,             // UI task must update within 1 second
        .temp_task_timeout_sec = 3,           // Temp control task must update within 3 seconds
        .sensor_timeout_sec = 30,             // Maximum time without sensor reading
        .sensor_validation_timeout_sec = 10,  // Sensor timeout for cycle validation
    },
    .temperature = {
        .hysteresis_celsius = 5.0f,           // ±5°C hysteresis for PID control
        .pressing_max_offset_celsius = 20.0f, // Max ±20°C offset during pressing
        .cycle_start_max_offset_celsius = 30.0f, // Max +30°C offset for cycle start
        .cycle_start_min_celsius = 20.0f,     // Minimum 20°C to start cycle
        .recovery_offset_celsius = 10.0f,     // 10°C offset for error recovery
        .min_for_pressing_celsius = 100.0f,   // Minimum 100°C to allow pressing
        .ready_threshold_celsius = 1.0f,      // ±1°C to consider heat-up complete
        .calibration_offset_celsius = 0.0f,   // 0°C calibration offset (no calibration)
    },
    .sensor = {
        .retry_count = 3,                     // 3 retry attempts for sensor reads
        .retry_delay_ms = 500,                // 500ms delay between retries
    },
    .heat_up = {
        .min_temp_change_celsius = 0.5f,      // 0.5°C minimum change for ETA calculation
        .min_elapsed_time_sec = 10,           // 10 second minimum for ETA calculation
        .min_heating_rate = 0.01f,            // 0.01°C/s minimum heating rate
    },
    .simulation = {
        .enabled = false,                     // Set to true to enable simulation mode
    },
};

// =============================================================================
// Configuration Validation
// =============================================================================

/**
 * @brief Validate system configuration
 *
 * Checks that all configuration values are within acceptable ranges.
 *
 * @return true if configuration is valid, false otherwise
 */
bool system_config_validate(void)
{
    validation_error = NULL;

    // Validate safety limits
    if (SYSTEM_CONFIG.safety.max_temperature_celsius <= 0.0f ||
        SYSTEM_CONFIG.safety.max_temperature_celsius > 300.0f)
    {
        validation_error = "Invalid max_temperature_celsius (must be 0-300)";
        return false;
    }

    if (SYSTEM_CONFIG.safety.max_cycle_time_seconds == 0 ||
        SYSTEM_CONFIG.safety.max_cycle_time_seconds > 3600)
    {
        validation_error = "Invalid max_cycle_time_seconds (must be 1-3600)";
        return false;
    }

    if (SYSTEM_CONFIG.safety.heap_minimum_bytes < 4096)
    {
        validation_error = "Invalid heap_minimum_bytes (must be >= 4096)";
        return false;
    }

    // Validate timing parameters
    if (SYSTEM_CONFIG.timing.ui_task_timeout_sec == 0 ||
        SYSTEM_CONFIG.timing.ui_task_timeout_sec > 60)
    {
        validation_error = "Invalid ui_task_timeout_sec (must be 1-60)";
        return false;
    }

    if (SYSTEM_CONFIG.timing.temp_task_timeout_sec == 0 ||
        SYSTEM_CONFIG.timing.temp_task_timeout_sec > 60)
    {
        validation_error = "Invalid temp_task_timeout_sec (must be 1-60)";
        return false;
    }

    if (SYSTEM_CONFIG.timing.sensor_timeout_sec == 0 ||
        SYSTEM_CONFIG.timing.sensor_timeout_sec > 300)
    {
        validation_error = "Invalid sensor_timeout_sec (must be 1-300)";
        return false;
    }

    if (SYSTEM_CONFIG.timing.sensor_validation_timeout_sec == 0 ||
        SYSTEM_CONFIG.timing.sensor_validation_timeout_sec > 60)
    {
        validation_error = "Invalid sensor_validation_timeout_sec (must be 1-60)";
        return false;
    }

    // Validate temperature thresholds
    if (SYSTEM_CONFIG.temperature.hysteresis_celsius <= 0.0f ||
        SYSTEM_CONFIG.temperature.hysteresis_celsius > 50.0f)
    {
        validation_error = "Invalid hysteresis_celsius (must be 0-50)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.pressing_max_offset_celsius <= 0.0f ||
        SYSTEM_CONFIG.temperature.pressing_max_offset_celsius > 100.0f)
    {
        validation_error = "Invalid pressing_max_offset_celsius (must be 0-100)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.cycle_start_max_offset_celsius <= 0.0f ||
        SYSTEM_CONFIG.temperature.cycle_start_max_offset_celsius > 100.0f)
    {
        validation_error = "Invalid cycle_start_max_offset_celsius (must be 0-100)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.cycle_start_min_celsius < -50.0f ||
        SYSTEM_CONFIG.temperature.cycle_start_min_celsius > 100.0f)
    {
        validation_error = "Invalid cycle_start_min_celsius (must be -50 to 100)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.recovery_offset_celsius <= 0.0f ||
        SYSTEM_CONFIG.temperature.recovery_offset_celsius > 100.0f)
    {
        validation_error = "Invalid recovery_offset_celsius (must be 0-100)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.min_for_pressing_celsius < 0.0f ||
        SYSTEM_CONFIG.temperature.min_for_pressing_celsius >
        SYSTEM_CONFIG.safety.max_temperature_celsius)
    {
        validation_error = "Invalid min_for_pressing_celsius (must be 0 to max_temperature)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.ready_threshold_celsius <= 0.0f ||
        SYSTEM_CONFIG.temperature.ready_threshold_celsius > 50.0f)
    {
        validation_error = "Invalid ready_threshold_celsius (must be 0-50)";
        return false;
    }

    if (SYSTEM_CONFIG.temperature.calibration_offset_celsius < -50.0f ||
        SYSTEM_CONFIG.temperature.calibration_offset_celsius > 50.0f)
    {
        validation_error = "Invalid calibration_offset_celsius (must be -50 to 50)";
        return false;
    }

    // Validate sensor configuration
    if (SYSTEM_CONFIG.sensor.retry_count == 0 ||
        SYSTEM_CONFIG.sensor.retry_count > 10)
    {
        validation_error = "Invalid sensor retry_count (must be 1-10)";
        return false;
    }

    if (SYSTEM_CONFIG.sensor.retry_delay_ms == 0 ||
        SYSTEM_CONFIG.sensor.retry_delay_ms > 5000)
    {
        validation_error = "Invalid sensor retry_delay_ms (must be 1-5000)";
        return false;
    }

    // Validate heat-up configuration
    if (SYSTEM_CONFIG.heat_up.min_temp_change_celsius <= 0.0f ||
        SYSTEM_CONFIG.heat_up.min_temp_change_celsius > 10.0f)
    {
        validation_error = "Invalid heat_up min_temp_change_celsius (must be 0-10)";
        return false;
    }

    if (SYSTEM_CONFIG.heat_up.min_elapsed_time_sec == 0 ||
        SYSTEM_CONFIG.heat_up.min_elapsed_time_sec > 300)
    {
        validation_error = "Invalid heat_up min_elapsed_time_sec (must be 1-300)";
        return false;
    }

    if (SYSTEM_CONFIG.heat_up.min_heating_rate <= 0.0f ||
        SYSTEM_CONFIG.heat_up.min_heating_rate > 10.0f)
    {
        validation_error = "Invalid heat_up min_heating_rate (must be 0-10)";
        return false;
    }

    return true;
}

/**
 * @brief Get configuration validation error message
 *
 * Returns a human-readable description of any configuration errors.
 *
 * @return Error message string, or NULL if no errors
 */
const char* system_config_get_error(void)
{
    return validation_error;
}

/**
 * @brief Print current configuration to log
 *
 * Outputs all configuration parameters to the ESP log for debugging.
 */
void system_config_print(void)
{
    ESP_LOGI(TAG, "=== System Configuration ===");

    ESP_LOGI(TAG, "Safety Limits:");
    ESP_LOGI(TAG, "  max_temperature_celsius: %.1f",
             SYSTEM_CONFIG.safety.max_temperature_celsius);
    ESP_LOGI(TAG, "  max_cycle_time_seconds: %lu",
             SYSTEM_CONFIG.safety.max_cycle_time_seconds);
    ESP_LOGI(TAG, "  heap_minimum_bytes: %lu",
             SYSTEM_CONFIG.safety.heap_minimum_bytes);

    ESP_LOGI(TAG, "Timing Parameters:");
    ESP_LOGI(TAG, "  ui_task_timeout_sec: %lu",
             SYSTEM_CONFIG.timing.ui_task_timeout_sec);
    ESP_LOGI(TAG, "  temp_task_timeout_sec: %lu",
             SYSTEM_CONFIG.timing.temp_task_timeout_sec);
    ESP_LOGI(TAG, "  sensor_timeout_sec: %lu",
             SYSTEM_CONFIG.timing.sensor_timeout_sec);
    ESP_LOGI(TAG, "  sensor_validation_timeout_sec: %lu",
             SYSTEM_CONFIG.timing.sensor_validation_timeout_sec);

    ESP_LOGI(TAG, "Temperature Thresholds:");
    ESP_LOGI(TAG, "  hysteresis_celsius: %.1f",
             SYSTEM_CONFIG.temperature.hysteresis_celsius);
    ESP_LOGI(TAG, "  pressing_max_offset_celsius: %.1f",
             SYSTEM_CONFIG.temperature.pressing_max_offset_celsius);
    ESP_LOGI(TAG, "  cycle_start_max_offset_celsius: %.1f",
             SYSTEM_CONFIG.temperature.cycle_start_max_offset_celsius);
    ESP_LOGI(TAG, "  cycle_start_min_celsius: %.1f",
             SYSTEM_CONFIG.temperature.cycle_start_min_celsius);
    ESP_LOGI(TAG, "  recovery_offset_celsius: %.1f",
             SYSTEM_CONFIG.temperature.recovery_offset_celsius);
    ESP_LOGI(TAG, "  min_for_pressing_celsius: %.1f",
             SYSTEM_CONFIG.temperature.min_for_pressing_celsius);
    ESP_LOGI(TAG, "  ready_threshold_celsius: %.1f",
             SYSTEM_CONFIG.temperature.ready_threshold_celsius);
    ESP_LOGI(TAG, "  calibration_offset_celsius: %.1f",
             SYSTEM_CONFIG.temperature.calibration_offset_celsius);

    ESP_LOGI(TAG, "Sensor Configuration:");
    ESP_LOGI(TAG, "  retry_count: %u",
             SYSTEM_CONFIG.sensor.retry_count);
    ESP_LOGI(TAG, "  retry_delay_ms: %lu",
             SYSTEM_CONFIG.sensor.retry_delay_ms);

    ESP_LOGI(TAG, "Heat-up Display:");
    ESP_LOGI(TAG, "  min_temp_change_celsius: %.2f",
             SYSTEM_CONFIG.heat_up.min_temp_change_celsius);
    ESP_LOGI(TAG, "  min_elapsed_time_sec: %lu",
             SYSTEM_CONFIG.heat_up.min_elapsed_time_sec);
    ESP_LOGI(TAG, "  min_heating_rate: %.3f",
             SYSTEM_CONFIG.heat_up.min_heating_rate);

    ESP_LOGI(TAG, "Simulation Mode: %s",
             SYSTEM_CONFIG.simulation.enabled ? "ENABLED" : "DISABLED");

    ESP_LOGI(TAG, "=== End Configuration ===");
}
