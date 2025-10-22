/**
 * @file temp_control.c
 * @brief Temperature control helper functions
 *
 * This module contains helper functions extracted from the main temperature
 * control task to improve code organization and readability.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "temp_control.h"
#include "main.h"
#include "ui_state.h"
#include "heating_contract.h"
#include "sensor_contract.h"
#include "storage_contract.h"
#include "pid_autotune.h"
#include "system_config.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "temp_control";

// External variables (defined in main.c)
extern settings_t settings;
extern statistics_t statistics;
extern float current_temperature;
extern bool emergency_shutdown;
extern uint8_t sensor_error_count;
extern uint32_t last_temp_reading;
extern float last_valid_temperature;
extern bool pressing_active;
extern bool pause_mode;
extern bool press_safety_locked;
extern bool is_autotuning;
extern autotune_context_t g_autotune_ctx;

// External mutex (defined in main.c)
extern SemaphoreHandle_t statistics_mutex;

// External functions (defined in main.c)
extern void update_pressing_cycle(void);
extern void control_heating_with_hysteresis(float pid_output);

// Thread-safe statistics access helpers
static inline void stats_lock(void) { xSemaphoreTake(statistics_mutex, portMAX_DELAY); }
static inline void stats_unlock(void) { xSemaphoreGive(statistics_mutex); }

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
bool handle_autotune_update(void)
{
    // Run auto-tune update
    float autotune_output = pid_autotune_update(&g_autotune_ctx, current_temperature);

    // Check if auto-tune is complete
    if (pid_autotune_is_complete(&g_autotune_ctx))
    {
        autotune_result_t result;
        if (pid_autotune_get_result(&g_autotune_ctx, &result))
        {
            // Apply new PID parameters
            settings.pid_kp = result.kp;
            settings.pid_ki = result.ki;
            settings.pid_kd = result.kd;

            // Update PID controller with new parameters
            pid_config_t new_pid_config = {
                .kp = result.kp,
                .ki = result.ki,
                .kd = result.kd,
                .setpoint = settings.target_temp,
                .output_min = 0.0f,
                .output_max = 100.0f
            };
            pid_init(new_pid_config);

            // Save new settings
            save_persistent_data();

            ESP_LOGI(TAG, "Auto-tune complete! New PID parameters:");
            ESP_LOGI(TAG, "  Kp = %.3f", result.kp);
            ESP_LOGI(TAG, "  Ki = %.3f", result.ki);
            ESP_LOGI(TAG, "  Kd = %.3f", result.kd);
            ESP_LOGI(TAG, "  Ultimate Gain (Ku) = %.3f", result.ultimate_gain);
            ESP_LOGI(TAG, "  Ultimate Period (Tu) = %.1f seconds", result.ultimate_period);

            heating_set_power(0);  // Turn off heating

            // Transition UI to results screen
            ui_set_state(UI_STATE_AUTOTUNE_COMPLETE);

            return false; // Auto-tune completed
        }
        else
        {
            ESP_LOGE(TAG, "Auto-tune failed to produce valid results");
            heating_set_power(0);
            return false; // Auto-tune failed
        }
    }
    else
    {
        // Apply auto-tune output (relay feedback)
        heating_set_power((uint8_t)autotune_output);
        return true; // Auto-tune still running
    }
}

// =============================================================================
// UI State Detection
// =============================================================================

/**
 * @brief Check if UI is in heat-up mode
 *
 * @return true if in heat-up mode, false otherwise
 */
bool is_in_heat_up_mode(void)
{
    ui_state_t current_ui_state = ui_get_current_state();
    return (current_ui_state == UI_STATE_HEAT_UP);
}

/**
 * @brief Check if UI is in any press workflow state
 *
 * Press workflow includes: START_PRESSING, FREE_PRESS, PRESSING_ACTIVE,
 * STAGE1_DONE, STAGE2_READY, STAGE2_DONE, CYCLE_COMPLETE.
 *
 * @return true if in press workflow, false otherwise
 */
bool is_in_press_workflow(void)
{
    ui_state_t current_ui_state = ui_get_current_state();
    return (current_ui_state == UI_STATE_START_PRESSING ||
            current_ui_state == UI_STATE_FREE_PRESS ||
            current_ui_state == UI_STATE_PRESSING_ACTIVE ||
            current_ui_state == UI_STATE_STAGE1_DONE ||
            current_ui_state == UI_STATE_STAGE2_READY ||
            current_ui_state == UI_STATE_STAGE2_DONE ||
            current_ui_state == UI_STATE_CYCLE_COMPLETE);
}

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
bool should_enable_heating(bool in_heat_up_mode, bool in_press_workflow)
{
    // Control heating when:
    // 1. In Heat Up mode and safety checks pass, OR
    // 2. In any Press workflow state and safety checks pass and not paused
    return (in_heat_up_mode && check_system_safety()) ||
           (in_press_workflow && check_system_safety() && !pause_mode);
}

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
void apply_heating_control(float pid_output, bool in_heat_up_mode, bool in_press_workflow)
{
    ESP_LOGI(TAG, "Heating: PID output=%.1f%%, pressing=%d, heat_up=%d, press_workflow=%d",
             pid_output, pressing_active, in_heat_up_mode, in_press_workflow);

    // In Heat Up mode or Press workflow (not actively pressing), apply PID directly without hysteresis
    // During active pressing, use hysteresis for stability
    if (in_heat_up_mode || (in_press_workflow && !pressing_active))
    {
        heating_set_power((uint8_t)pid_output);
    }
    else
    {
        control_heating_with_hysteresis(pid_output);
    }
}

/**
 * @brief Handle normal temperature control operation
 *
 * Performs normal PID-based temperature control including:
 * - Pressing cycle timing updates
 * - UI state detection
 * - Heating enable/disable logic
 * - PID control output application
 */
void handle_normal_temp_control(void)
{
    // Update pressing cycle timing
    update_pressing_cycle();

    // Detect current UI state
    bool in_heat_up_mode = is_in_heat_up_mode();
    bool in_press_workflow = is_in_press_workflow();

    // Check if heating should be enabled
    if (should_enable_heating(in_heat_up_mode, in_press_workflow))
    {
        // Update PID controller with current temperature
        float output = pid_update(current_temperature);

        // Apply heating control
        apply_heating_control(output, in_heat_up_mode, in_press_workflow);
    }
    else
    {
        // No heating when not in heating modes, paused, or when safety systems are engaged
        ESP_LOGD(TAG, "Heating off: pressing=%d, locked=%d, safety=%d, pause=%d, heat_up=%d, press_workflow=%d",
                 pressing_active, press_safety_locked, check_system_safety(), pause_mode,
                 in_heat_up_mode, in_press_workflow);
        heating_set_power(0);
    }
}

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
bool validate_temperature_safety(void)
{
    if (current_temperature > MAX_TEMPERATURE)
    {
        ESP_LOGE(TAG, "Temperature safety limit exceeded: %.2f°C > %.2f°C",
                 current_temperature, MAX_TEMPERATURE);
        return false;
    }
    return true;
}

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
void handle_sensor_failure(void)
{
    sensor_error_count++;

    // Update failure statistics (thread-safe)
    stats_lock();
    statistics.sensor_failures++;
    stats_unlock();

    ESP_LOGW(TAG, "Temperature sensor read failed (attempt %d/%d)",
             sensor_error_count, SENSOR_RETRY_COUNT);

    // Emergency shutdown after maximum retry attempts
    if (sensor_error_count >= SENSOR_RETRY_COUNT)
    {
        emergency_shutdown_system("Temperature sensor failure - too many consecutive errors");
    }
    else
    {
        // Use last valid temperature for control during sensor errors
        current_temperature = last_valid_temperature;

        // Safety: turn off heating during sensor errors
        heating_set_power(0);
    }
}

/**
 * @brief Handle successful temperature sensor read
 *
 * Updates temperature tracking variables and resets error counters.
 *
 * @param new_temperature New temperature reading from sensor
 */
void handle_sensor_success(float new_temperature)
{
    current_temperature = new_temperature;
    last_temp_reading = esp_timer_get_time() / 1000000;
    sensor_error_count = 0; // Reset error count on successful read

    // Log temperature to console
    ESP_LOGI(TAG, "Temperature: %.2f°C", current_temperature);
}
