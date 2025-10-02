/**
 * @file pid_autotune.h
 * @brief PID auto-tuning using relay feedback method
 *
 * This module implements automatic PID tuning using the Åström-Hägglund
 * relay feedback method. This method is well-suited for temperature control
 * systems and provides reliable tuning results.
 *
 * The auto-tuning process:
 * 1. Applies relay control (on/off with hysteresis)
 * 2. Observes system oscillation
 * 3. Measures ultimate gain (Ku) and period (Tu)
 * 4. Calculates PID parameters using Ziegler-Nichols rules
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef PID_AUTOTUNE_H
#define PID_AUTOTUNE_H

#include <stdint.h>
#include <stdbool.h>
#include "pid_controller.h"

// =============================================================================
// Type Definitions
// =============================================================================

/**
 * @brief Auto-tune state machine states
 */
typedef enum
{
    AUTOTUNE_STATE_IDLE,           ///< Not running
    AUTOTUNE_STATE_RELAY_STEP_UP,  ///< Relay output high
    AUTOTUNE_STATE_RELAY_STEP_DOWN,///< Relay output low
    AUTOTUNE_STATE_MEASURE_PERIOD, ///< Measuring oscillation period
    AUTOTUNE_STATE_CALCULATING,    ///< Computing PID parameters
    AUTOTUNE_STATE_COMPLETE,       ///< Auto-tune successful
    AUTOTUNE_STATE_FAILED          ///< Auto-tune failed
} autotune_state_t;

/**
 * @brief Auto-tune configuration
 */
typedef struct
{
    float setpoint;              ///< Target temperature for tuning (°C)
    float output_step;           ///< Relay output step size (0-100%)
    float noise_band;            ///< Noise band to ignore (°C)
    uint32_t max_cycles;         ///< Maximum oscillation cycles to observe
    uint32_t timeout_seconds;    ///< Maximum tuning time (safety)
    float initial_output;        ///< Starting output level (%)
} autotune_config_t;

/**
 * @brief Auto-tune results
 */
typedef struct
{
    float kp;                    ///< Calculated proportional gain
    float ki;                    ///< Calculated integral gain
    float kd;                    ///< Calculated derivative gain
    float ultimate_gain;         ///< Measured ultimate gain (Ku)
    float ultimate_period;       ///< Measured ultimate period (Tu) in seconds
    uint32_t cycles_observed;    ///< Number of oscillation cycles observed
    autotune_state_t final_state;///< Final state of auto-tune
} autotune_result_t;

/**
 * @brief Tuning rule selection
 */
typedef enum
{
    TUNING_RULE_ZIEGLER_NICHOLS_CLASSIC, ///< Classic Z-N: Some overshoot
    TUNING_RULE_ZIEGLER_NICHOLS_PESSEN,  ///< Pessen: Less overshoot
    TUNING_RULE_ZIEGLER_NICHOLS_SOME_OVERSHOOT, ///< Some overshoot variant
    TUNING_RULE_ZIEGLER_NICHOLS_NO_OVERSHOOT,   ///< No overshoot variant
    TUNING_RULE_TYREUS_LUYBEN,           ///< Tyreus-Luyben: Conservative
} tuning_rule_t;

/**
 * @brief Auto-tune context structure
 *
 * Contains all state and data for the auto-tuning process.
 * User should not access members directly.
 */
typedef struct
{
    // Configuration
    autotune_config_t config;
    tuning_rule_t rule;

    // State
    autotune_state_t state;
    uint32_t start_time_sec;
    bool relay_output_high;

    // Oscillation detection
    float peak_high[10];         ///< Buffer for peak highs
    float peak_low[10];          ///< Buffer for peak lows
    uint32_t peak_timestamps[10];///< Timestamps of peaks
    uint8_t peak_count;
    uint8_t peak_index;

    // Measurement state
    float last_input;
    bool just_changed;
    uint32_t last_change_time_sec;
    bool is_peak_detected;
    float running_output;

    // Results
    autotune_result_t result;

} autotune_context_t;

// =============================================================================
// Function Prototypes
// =============================================================================

/**
 * @brief Initialize auto-tuning context
 *
 * Prepares the auto-tuner for a new tuning run. Must be called before
 * starting auto-tune.
 *
 * @param ctx Pointer to auto-tune context
 * @param config Auto-tune configuration parameters
 * @param rule Tuning rule to use for calculating PID parameters
 */
void pid_autotune_init(autotune_context_t *ctx,
                       autotune_config_t config,
                       tuning_rule_t rule);

/**
 * @brief Start the auto-tuning process
 *
 * Begins the relay feedback test. The process will run until completion,
 * failure, or timeout.
 *
 * @param ctx Pointer to auto-tune context
 * @return true if started successfully, false if already running
 */
bool pid_autotune_start(autotune_context_t *ctx);

/**
 * @brief Update auto-tuning state machine
 *
 * This function must be called periodically (e.g., every 1 second) with
 * the current process variable (temperature). It performs the relay test
 * and detects oscillations.
 *
 * @param ctx Pointer to auto-tune context
 * @param input Current process variable (temperature in °C)
 * @return Control output (0-100%) to apply to the system
 */
float pid_autotune_update(autotune_context_t *ctx, float input);

/**
 * @brief Check if auto-tuning is complete
 *
 * @param ctx Pointer to auto-tune context
 * @return true if complete (success or failure), false if still running
 */
bool pid_autotune_is_complete(const autotune_context_t *ctx);

/**
 * @brief Get auto-tuning results
 *
 * Retrieves the calculated PID parameters. Only valid after auto-tune
 * completes successfully.
 *
 * @param ctx Pointer to auto-tune context
 * @param result Pointer to structure to receive results
 * @return true if results are valid, false if auto-tune not complete
 */
bool pid_autotune_get_result(const autotune_context_t *ctx,
                             autotune_result_t *result);

/**
 * @brief Apply auto-tune results to PID controller
 *
 * Convenience function to configure a PID controller with auto-tuned
 * parameters.
 *
 * @param ctx Pointer to auto-tune context
 * @param pid Pointer to PID controller to configure
 * @return true if applied successfully, false if results invalid
 */
bool pid_autotune_apply_result(const autotune_context_t *ctx,
                               pid_controller_t *pid);

/**
 * @brief Cancel auto-tuning
 *
 * Stops the auto-tuning process and returns to idle state.
 *
 * @param ctx Pointer to auto-tune context
 */
void pid_autotune_cancel(autotune_context_t *ctx);

/**
 * @brief Get current auto-tune state
 *
 * @param ctx Pointer to auto-tune context
 * @return Current state of auto-tuner
 */
autotune_state_t pid_autotune_get_state(const autotune_context_t *ctx);

/**
 * @brief Get progress percentage
 *
 * Returns approximate progress through the auto-tune process.
 *
 * @param ctx Pointer to auto-tune context
 * @return Progress percentage (0-100)
 */
uint8_t pid_autotune_get_progress(const autotune_context_t *ctx);

/**
 * @brief Create default auto-tune configuration
 *
 * Provides reasonable defaults for heat press temperature control.
 *
 * @param setpoint Target temperature for tuning (°C)
 * @return Default configuration structure
 */
autotune_config_t pid_autotune_default_config(float setpoint);

#endif // PID_AUTOTUNE_H
