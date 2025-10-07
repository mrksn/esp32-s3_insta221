/**
 * @file pid_controller.h
 * @brief PID temperature controller module
 *
 * This module provides a self-contained PID controller implementation
 * for temperature regulation. It maintains its own state and provides
 * a clean interface for initialization, updates, and resets.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "heating_contract.h"  // components/heating/include/ - For pid_config_t

// =============================================================================
// Type Definitions
// =============================================================================

/**
 * @brief PID controller state
 *
 * Encapsulates all PID controller state for better testability
 * and multiple controller support.
 */
typedef struct
{
    pid_config_t config;     ///< Configuration parameters
    float integral;          ///< Integral accumulator
    float prev_error;        ///< Previous error for derivative
    uint64_t last_update_us; ///< Last update timestamp (microseconds)
    float last_output;       ///< Last calculated output
} pid_controller_t;

// =============================================================================
// Function Prototypes
// =============================================================================

/**
 * @brief Initialize PID controller
 *
 * Sets up the PID controller with the specified configuration and
 * resets all internal state (integral, previous error, timing).
 *
 * @param pid Pointer to PID controller structure
 * @param config PID configuration parameters
 */
void pid_controller_init(pid_controller_t *pid, pid_config_t config);

/**
 * @brief Update PID controller and calculate output
 *
 * Implements the PID control algorithm with anti-windup protection
 * and output clamping. Should be called periodically at a consistent
 * rate for optimal performance.
 *
 * @param pid Pointer to PID controller structure
 * @param measurement Current process variable (temperature)
 * @return Calculated control output (clamped to configured limits)
 */
float pid_controller_update(pid_controller_t *pid, float measurement);

/**
 * @brief Reset PID controller state
 *
 * Clears integral accumulator and previous error. Use when changing
 * setpoint or after significant process disturbances.
 *
 * @param pid Pointer to PID controller structure
 */
void pid_controller_reset(pid_controller_t *pid);

/**
 * @brief Update PID setpoint
 *
 * Changes the target setpoint and optionally resets integral term
 * to prevent windup when making large setpoint changes.
 *
 * @param pid Pointer to PID controller structure
 * @param new_setpoint New target setpoint
 * @param reset_integral If true, clears integral accumulator
 */
void pid_controller_set_setpoint(pid_controller_t *pid, float new_setpoint, bool reset_integral);

/**
 * @brief Get current PID output
 *
 * Returns the last calculated output without performing an update.
 * Useful for monitoring or diagnostics.
 *
 * @param pid Pointer to PID controller structure
 * @return Last calculated output value
 */
float pid_controller_get_output(const pid_controller_t *pid);

#endif // PID_CONTROLLER_H
