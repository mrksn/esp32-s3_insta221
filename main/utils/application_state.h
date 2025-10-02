/**
 * @file application_state.h
 * @brief Application state management
 *
 * This module encapsulates the global application state into a
 * structured format, providing better organization and reducing
 * the number of scattered global variables.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef APPLICATION_STATE_H
#define APPLICATION_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "data_model.h"

// =============================================================================
// Application State Structure
// =============================================================================

/**
 * @brief Main application state
 *
 * Encapsulates all runtime state into a single structure for better
 * organization, testability, and maintenance.
 */
typedef struct
{
    // Configuration and data models
    settings_t settings;              ///< System settings
    print_run_t print_run;            ///< Current print run data
    pressing_cycle_t current_cycle;   ///< Active pressing cycle

    // Temperature state
    struct
    {
        float current_celsius;        ///< Current temperature reading (°C)
        float last_valid_celsius;     ///< Last valid temperature (°C)
        uint32_t last_reading_time_sec; ///< Time of last successful reading
    } temperature;

    // Pressing cycle state
    struct
    {
        bool is_active;               ///< Is a pressing cycle active?
        uint32_t start_time_sec;      ///< Cycle start timestamp
        uint32_t stage_start_time_sec; ///< Current stage start timestamp
        cycle_status_t current_stage; ///< Current cycle stage
    } pressing;

    // Safety and error state
    struct
    {
        bool is_emergency_shutdown;   ///< Emergency shutdown flag
        bool is_press_locked;         ///< Safety interlock engaged?
        bool is_system_paused;        ///< System in pause mode?
        uint8_t sensor_consecutive_errors; ///< Consecutive sensor failures
    } safety;

    // UI tracking
    struct
    {
        bool was_press_closed;        ///< Previous press state
        bool was_heating_active;      ///< Previous heating state (for hysteresis)
    } ui_tracking;

    // Task monitoring
    struct
    {
        uint32_t ui_task_last_run_sec;        ///< Last UI task execution
        uint32_t temp_task_last_run_sec;      ///< Last temp task execution
        bool is_system_healthy;               ///< Overall system health
    } monitoring;

} application_state_t;

// =============================================================================
// Global State Access
// =============================================================================

/**
 * @brief Get pointer to application state
 *
 * Provides controlled access to the global application state.
 * Use this instead of direct global variable access.
 *
 * @return Pointer to application state
 */
application_state_t *app_state_get(void);

/**
 * @brief Initialize application state
 *
 * Resets all state to safe defaults. Must be called at startup.
 */
void app_state_init(void);

// =============================================================================
// Convenience Accessors (Optional - use for better encapsulation)
// =============================================================================

// Temperature accessors
static inline float app_state_get_current_temp(void)
{
    return app_state_get()->temperature.current_celsius;
}

static inline void app_state_set_current_temp(float temp)
{
    app_state_get()->temperature.current_celsius = temp;
}

// Safety state accessors
static inline bool app_state_is_emergency_shutdown(void)
{
    return app_state_get()->safety.is_emergency_shutdown;
}

static inline bool app_state_is_paused(void)
{
    return app_state_get()->safety.is_system_paused;
}

static inline bool app_state_is_pressing_active(void)
{
    return app_state_get()->pressing.is_active;
}

#endif // APPLICATION_STATE_H
