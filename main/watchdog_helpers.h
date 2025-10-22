/**
 * @file watchdog_helpers.h
 * @brief Watchdog task helper functions
 *
 * This header provides helper functions for system health monitoring
 * extracted from the main watchdog task.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef WATCHDOG_HELPERS_H
#define WATCHDOG_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// Task Health Monitoring
// =============================================================================

/**
 * @brief Check if UI task is responsive
 *
 * Verifies that the UI task has updated within the timeout period.
 *
 * @param current_time Current system time in seconds
 * @return true if UI task is healthy, false if unresponsive
 */
bool check_ui_task_health(uint32_t current_time);

/**
 * @brief Check if temperature control task is responsive
 *
 * Verifies that the temperature control task has updated within the timeout period.
 * Triggers emergency shutdown if task appears to have failed.
 *
 * @param current_time Current system time in seconds
 * @return true if temp control task is healthy, false if unresponsive
 */
bool check_temp_control_task_health(uint32_t current_time);

// =============================================================================
// Memory Health Monitoring
// =============================================================================

/**
 * @brief Check heap memory availability
 *
 * Verifies sufficient heap memory is available. Triggers emergency shutdown
 * if critically low. Logs warning if approaching limits.
 *
 * @return true if memory is sufficient, false if critically low
 */
bool check_memory_health(void);

// =============================================================================
// Sensor Health Monitoring
// =============================================================================

/**
 * @brief Check sensor communication health
 *
 * Verifies that valid sensor readings have been received recently.
 * Triggers emergency shutdown if sensor communication is lost.
 *
 * @param current_time Current system time in seconds
 * @return true if sensor is healthy, false if communication lost
 */
bool check_sensor_health(uint32_t current_time);

// =============================================================================
// System Recovery
// =============================================================================

/**
 * @brief Attempt system recovery from error state
 *
 * Checks if conditions are safe to recover from error state and
 * attempts recovery if possible.
 *
 * @return true if recovery successful, false if unsafe to recover
 */
bool attempt_system_recovery(void);

// =============================================================================
// Status Logging
// =============================================================================

/**
 * @brief Log system health status
 *
 * Logs current system health status including memory and temperature.
 *
 * @param free_heap Current free heap size
 */
void log_system_health_status(uint32_t free_heap);

#endif // WATCHDOG_HELPERS_H
