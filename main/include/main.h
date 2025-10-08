/**
 * @file main.h
 * @brief Main application header for Insta Retrofit heat press system
 *
 * This header provides access to global state variables and safety functions
 * for testing purposes. It exposes the internal state management and safety
 * mechanisms used by the main application.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "data_model.h"  // components/storage/include/ - For settings_t, print_run_t, etc.

// =============================================================================
// Global State Variables (Exposed for Testing)
// =============================================================================

// Configuration and data models
extern settings_t settings;
extern print_run_t print_run;
extern pressing_cycle_t current_cycle;
extern float current_temperature;
extern float last_valid_temperature;

// Pressing cycle state management
extern bool pressing_active;
extern uint32_t cycle_start_time;
extern uint32_t stage_start_time;
extern cycle_status_t current_stage;

// Error state and safety management
extern bool emergency_shutdown;
extern uint8_t sensor_error_count;
extern uint32_t last_temp_reading;
extern bool press_safety_locked;
extern bool pause_mode;

// System health
extern bool system_healthy;

// =============================================================================
// Safety and Error Handling Functions
// =============================================================================

/**
 * @brief Emergency shutdown system with safety actions
 *
 * Immediately shuts down all heating, locks safety interlocks, and logs
 * the emergency condition.
 *
 * @param reason Human-readable reason for emergency shutdown
 */
void emergency_shutdown_system(const char *reason);

/**
 * @brief Check overall system safety status
 *
 * Validates temperature limits, memory usage, sensor health, and emergency
 * shutdown state.
 *
 * @return true if system is safe, false if safety violation detected
 */
bool check_system_safety(void);

/**
 * @brief Read temperature with retry logic and error handling
 *
 * Attempts to read temperature with configurable retry attempts and delays.
 * Updates error counters and last valid temperature on success.
 *
 * @param[out] temperature Pointer to store temperature reading
 * @return true on successful read, false on failure
 */
bool read_temperature_safe(float *temperature);

/**
 * @brief Reset error state when safe conditions are met
 *
 * Attempts to recover from error state if temperature and other conditions
 * are within safe limits.
 */
void reset_error_state(void);

/**
 * @brief Validate conditions for starting a pressing cycle
 *
 * Checks system health, temperature ranges, memory usage, and sensor
 * responsiveness before allowing cycle start.
 *
 * @return true if cycle can safely start, false otherwise
 */
bool validate_cycle_safety(void);

/**
 * @brief Check if target temperature has been reached at least once since boot
 *
 * This is a persistent state flag that indicates the heat press has been
 * properly warmed up at least once during this session.
 *
 * @return true if target temperature was reached at least once since boot
 */
bool has_reached_target_temp_once(void);

/**
 * @brief Check if heat press is in full ready state for pressing
 *
 * Heat press is ready when:
 * 1. Target temperature has been reached at least once since boot, AND
 * 2. Heating switch is connected (heating is active), AND
 * 3. Current temperature is within ±5°C of target temperature
 *
 * @return true if heat press is ready for pressing operations
 */
bool is_heat_press_ready(void);

/**
 * @brief Cleanup and shutdown all system components
 *
 * Safely deinitializes all hardware components in the correct order.
 * Ensures heating is off and all resources are properly freed.
 * Safe to call even if initialization failed.
 */
void system_cleanup(void);

/**
 * @brief Reset all statistics counters
 *
 * Resets all statistics tracking data. Thread-safe operation.
 */
void reset_all_statistics(void);

/**
 * @brief Attempt to recover from emergency shutdown state
 *
 * Checks if conditions are safe to resume normal operation and
 * releases emergency shutdown if recovery is possible.
 *
 * @return true if recovery successful, false if still unsafe
 */
bool attempt_emergency_recovery(void);

// =============================================================================
// System Constants
// =============================================================================
// All system constants have been moved to system_constants.h
// Include that header to access safety limits, timing constants, and
// temperature validation parameters.

#endif // MAIN_H
