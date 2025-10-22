/**
 * @file temp_control.h
 * @brief Temperature control helper functions
 *
 * This header provides helper functions for temperature control operations
 * extracted from the main temperature control task.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include <stdbool.h>

// =============================================================================
// Auto-tune Handling
// =============================================================================

/**
 * @brief Handle auto-tune update and completion
 *
 * Processes one iteration of PID auto-tuning, applies relay feedback output,
 * and handles completion by updating PID parameters and saving settings.
 *
 * @return true if auto-tune is still running, false if completed or failed
 */
bool handle_autotune_update(void);

// =============================================================================
// UI State Detection
// =============================================================================

/**
 * @brief Check if UI is in heat-up mode
 *
 * @return true if in heat-up mode, false otherwise
 */
bool is_in_heat_up_mode(void);

/**
 * @brief Check if UI is in any press workflow state
 *
 * Press workflow includes: START_PRESSING, FREE_PRESS, PRESSING_ACTIVE,
 * STAGE1_DONE, STAGE2_READY, STAGE2_DONE, CYCLE_COMPLETE.
 *
 * @return true if in press workflow, false otherwise
 */
bool is_in_press_workflow(void);

// =============================================================================
// Heating Control Logic
// =============================================================================

/**
 * @brief Determine if heating should be enabled
 *
 * Checks current UI state and safety conditions to determine if heating
 * should be enabled.
 *
 * @param in_heat_up_mode True if in heat-up mode
 * @param in_press_workflow True if in press workflow
 * @return true if heating should be enabled, false otherwise
 */
bool should_enable_heating(bool in_heat_up_mode, bool in_press_workflow);

/**
 * @brief Apply PID control output to heating element
 *
 * Applies PID output directly or with hysteresis depending on current state.
 * Uses direct control during heat-up or when not actively pressing.
 * Uses hysteresis during active pressing for stability.
 *
 * @param pid_output PID controller output (0-100%)
 * @param in_heat_up_mode True if in heat-up mode
 * @param in_press_workflow True if in press workflow
 */
void apply_heating_control(float pid_output, bool in_heat_up_mode, bool in_press_workflow);

/**
 * @brief Handle normal temperature control operation
 *
 * Performs normal PID-based temperature control including:
 * - Pressing cycle timing updates
 * - UI state detection
 * - Heating enable/disable logic
 * - PID control output application
 */
void handle_normal_temp_control(void);

// =============================================================================
// Temperature Safety Checks
// =============================================================================

/**
 * @brief Validate temperature is within safe limits
 *
 * Checks if current temperature exceeds maximum safe limit and triggers
 * emergency shutdown if necessary.
 *
 * @return true if temperature is safe, false if limit exceeded
 */
bool validate_temperature_safety(void);

// =============================================================================
// Sensor Error Handling
// =============================================================================

/**
 * @brief Handle temperature sensor read failure
 *
 * Implements retry and escalation strategy for sensor failures:
 * - Increments error counter
 * - Updates statistics
 * - Triggers emergency shutdown after max retries
 * - Uses last valid temperature for control during errors
 */
void handle_sensor_failure(void);

/**
 * @brief Handle successful temperature sensor read
 *
 * Updates temperature tracking variables and resets error counters.
 *
 * @param new_temperature New temperature reading from sensor
 */
void handle_sensor_success(float new_temperature);

#endif // TEMP_CONTROL_H
