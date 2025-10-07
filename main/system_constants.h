/**
 * @file system_constants.h
 * @brief System-wide constants and configuration parameters
 *
 * This header centralizes all magic numbers and configuration values
 * used throughout the Insta Retrofit heat press system.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef SYSTEM_CONSTANTS_H
#define SYSTEM_CONSTANTS_H

#include <stdint.h>

// =============================================================================
// Task Configuration
// =============================================================================

#define UI_TASK_STACK_SIZE 4096
#define TEMP_CONTROL_TASK_STACK_SIZE 4096
#define WATCHDOG_TASK_STACK_SIZE 2048
#define UI_TASK_PRIORITY 5
#define TEMP_CONTROL_TASK_PRIORITY 4
#define WATCHDOG_TASK_PRIORITY 3

// =============================================================================
// Safety Limits - Critical safety parameters
// =============================================================================

/** Maximum safe temperature (°C) - heating disabled above this */
#define MAX_TEMPERATURE 220.0f

/** Maximum cycle time (seconds) - 5 minutes safety limit */
#define MAX_CYCLE_TIME 300

/** Number of sensor read retry attempts */
#define SENSOR_RETRY_COUNT 3

/** Delay between sensor retry attempts (ms) */
#define SENSOR_RETRY_DELAY_MS 500

/** Temperature hysteresis for PID control (°C) */
#define TEMP_HYSTERESIS 5.0f

/** Minimum free heap memory (bytes) - 8KB */
#define HEAP_MINIMUM 8192

// =============================================================================
// Timing Constants
// =============================================================================

/** UI task watchdog timeout (seconds) */
#define UI_TASK_TIMEOUT_SEC 1

/** Temperature task watchdog timeout (seconds) */
#define TEMP_TASK_TIMEOUT_SEC 3

/** Maximum time without sensor reading (seconds) */
#define SENSOR_TIMEOUT_SEC 30

/** Sensor validation timeout for cycle start (seconds) */
#define SENSOR_VALIDATION_TIMEOUT_SEC 10

// =============================================================================
// Temperature Validation Constants
// =============================================================================

/** Maximum temperature offset during pressing (°C) */
#define TEMP_PRESSING_MAX_OFFSET 20.0f

/** Maximum temperature offset for cycle start (°C) */
#define TEMP_CYCLE_START_MAX_OFFSET 30.0f

/** Minimum temperature for cycle start (°C) */
#define TEMP_CYCLE_START_MIN 20.0f

/** Temperature offset for error recovery (°C) */
#define TEMP_RECOVERY_OFFSET 10.0f

// =============================================================================
// Default Values
// =============================================================================

/** Default temperature reading when sensor fails (°C) */
#define DEFAULT_TEMPERATURE 25.0f

#endif // SYSTEM_CONSTANTS_H
