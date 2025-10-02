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

// =============================================================================
// Safety Limits (Exposed for Testing)
// =============================================================================

extern const float MAX_TEMPERATURE;
extern const uint32_t MAX_CYCLE_TIME;
extern const uint8_t SENSOR_RETRY_COUNT;
extern const uint32_t SENSOR_RETRY_DELAY_MS;
extern const float TEMP_HYSTERESIS;
extern const uint32_t HEAP_MINIMUM;

// Timing constants
extern const uint32_t UI_TASK_TIMEOUT_SEC;
extern const uint32_t TEMP_TASK_TIMEOUT_SEC;
extern const uint32_t SENSOR_TIMEOUT_SEC;
extern const uint32_t SENSOR_VALIDATION_TIMEOUT_SEC;

// Temperature validation constants
extern const float TEMP_PRESSING_MAX_OFFSET;
extern const float TEMP_CYCLE_START_MAX_OFFSET;
extern const float TEMP_CYCLE_START_MIN;
extern const float TEMP_RECOVERY_OFFSET;

#endif // MAIN_H
