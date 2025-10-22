/**
 * @file watchdog_helpers.c
 * @brief Watchdog task helper functions implementation
 *
 * This module contains helper functions extracted from the main watchdog
 * task to improve code organization and readability.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "watchdog_helpers.h"
#include "main.h"
#include "system_config.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "watchdog";

// External variables (defined in main.c)
extern settings_t settings;
extern float current_temperature;
extern bool emergency_shutdown;
extern bool pressing_active;
extern bool system_healthy;
extern uint32_t ui_task_last_run;
extern uint32_t temp_control_task_last_run;
extern uint32_t last_temp_reading;

// External functions (defined in main.c)
extern void emergency_shutdown_system(const char *reason);
extern void reset_error_state(void);

// =============================================================================
// Task Health Monitoring
// =============================================================================

bool check_ui_task_health(uint32_t current_time)
{
    // Check if UI task is still running (should update every 100ms)
    if ((current_time - ui_task_last_run) > UI_TASK_TIMEOUT_SEC)
    {
        ESP_LOGE(TAG, "UI task appears unresponsive!");
        system_healthy = false;
        return false;
    }
    return true;
}

bool check_temp_control_task_health(uint32_t current_time)
{
    // Check if temperature control task is still running (should update every 1s)
    if ((current_time - temp_control_task_last_run) > TEMP_TASK_TIMEOUT_SEC)
    {
        ESP_LOGE(TAG, "Temperature control task appears unresponsive!");
        system_healthy = false;
        emergency_shutdown_system("Temperature control task failure");
        return false;
    }
    return true;
}

// =============================================================================
// Memory Health Monitoring
// =============================================================================

bool check_memory_health(void)
{
    // Check heap memory (critical for embedded systems)
    uint32_t free_heap = esp_get_free_heap_size();

    if (free_heap < HEAP_MINIMUM)
    {
        ESP_LOGE(TAG, "Critical heap memory low: %lu bytes free (minimum: %lu)",
                 (unsigned long)free_heap, (unsigned long)HEAP_MINIMUM);
        emergency_shutdown_system("Critical memory shortage detected");
        return false;
    }
    else if (free_heap < (HEAP_MINIMUM * 2))
    {
        ESP_LOGW(TAG, "Low heap memory warning: %lu bytes free", (unsigned long)free_heap);
    }

    return true;
}

// =============================================================================
// Sensor Health Monitoring
// =============================================================================

bool check_sensor_health(uint32_t current_time)
{
    // Check for prolonged sensor read failures
    if ((current_time - last_temp_reading) > SENSOR_TIMEOUT_SEC)
    {
        ESP_LOGE(TAG, "No valid temperature reading for %lu+ seconds",
                 (unsigned long)SENSOR_TIMEOUT_SEC);
        emergency_shutdown_system("Temperature sensor communication lost");
        return false;
    }
    return true;
}

// =============================================================================
// System Recovery
// =============================================================================

bool attempt_system_recovery(void)
{
    // Check system health and attempt recovery if possible
    if (!system_healthy && !emergency_shutdown)
    {
        ESP_LOGW(TAG, "System health compromised - attempting recovery");

        // Try to reset error state if conditions are safe
        if (current_temperature < (settings.target_temp - TEMP_RECOVERY_OFFSET) && !pressing_active)
        {
            ESP_LOGI(TAG, "Safe conditions detected - attempting error recovery");
            reset_error_state();
            return true;
        }
        return false;
    }
    return false;
}

// =============================================================================
// Status Logging
// =============================================================================

void log_system_health_status(uint32_t free_heap)
{
    // Log system health status
    if (system_healthy && !emergency_shutdown)
    {
        ESP_LOGD(TAG, "System health check passed - heap: %lu bytes, temp: %.1f°C",
                 (unsigned long)free_heap, current_temperature);
    }
    else
    {
        ESP_LOGE(TAG, "System health check failed - emergency shutdown active");
    }
}
