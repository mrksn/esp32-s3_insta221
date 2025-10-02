/**
 * @file application_state.c
 * @brief Application state management implementation
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "application_state.h"
#include "esp_timer.h"
#include <string.h>

// Global application state instance
static application_state_t g_app_state;

application_state_t *app_state_get(void)
{
    return &g_app_state;
}

void app_state_init(void)
{
    // Zero out entire structure
    memset(&g_app_state, 0, sizeof(application_state_t));

    // Initialize temperature with reasonable defaults
    g_app_state.temperature.current_celsius = 25.0f;
    g_app_state.temperature.last_valid_celsius = 25.0f;
    g_app_state.temperature.last_reading_time_sec = esp_timer_get_time() / 1000000;

    // Initialize safety state
    g_app_state.safety.is_emergency_shutdown = false;
    g_app_state.safety.is_press_locked = true; // Start with safety lock engaged
    g_app_state.safety.is_system_paused = false;
    g_app_state.safety.sensor_consecutive_errors = 0;

    // Initialize pressing state
    g_app_state.pressing.is_active = false;
    g_app_state.pressing.current_stage = IDLE;

    // Initialize monitoring state
    uint32_t now = esp_timer_get_time() / 1000000;
    g_app_state.monitoring.ui_task_last_run_sec = now;
    g_app_state.monitoring.temp_task_last_run_sec = now;
    g_app_state.monitoring.is_system_healthy = true;

    // Initialize UI tracking
    g_app_state.ui_tracking.was_press_closed = false;
    g_app_state.ui_tracking.was_heating_active = false;
}
