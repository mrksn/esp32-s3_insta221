/**
 * @file pid_autotune.c
 * @brief PID auto-tuning implementation
 *
 * Implements the Åström-Hägglund relay feedback method for automatic
 * PID parameter tuning.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "pid_autotune.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <string.h>

static const char *TAG = "pid_autotune";

// =============================================================================
// Tuning Rule Coefficients
// =============================================================================

typedef struct {
    float kp_factor;
    float ti_factor; // Integral time factor (Ti = tu / ti_factor)
    float td_factor; // Derivative time factor (Td = tu * td_factor)
} tuning_coefficients_t;

static const tuning_coefficients_t tuning_rules[] = {
    // Ziegler-Nichols Classic PID: Ku * 0.6, Tu / 2, Tu / 8
    [TUNING_RULE_ZIEGLER_NICHOLS_CLASSIC] = {0.6f, 2.0f, 0.125f},

    // Pessen Integral Rule: More aggressive
    [TUNING_RULE_ZIEGLER_NICHOLS_PESSEN] = {0.7f, 2.5f, 0.15f},

    // Some Overshoot
    [TUNING_RULE_ZIEGLER_NICHOLS_SOME_OVERSHOOT] = {0.33f, 2.0f, 0.33f},

    // No Overshoot
    [TUNING_RULE_ZIEGLER_NICHOLS_NO_OVERSHOOT] = {0.2f, 2.0f, 0.33f},

    // Tyreus-Luyben: Conservative, good for temperature
    [TUNING_RULE_TYREUS_LUYBEN] = {0.45f, 2.2f, 0.15f},
};

// =============================================================================
// Helper Functions
// =============================================================================

static uint32_t get_time_sec(void)
{
    return esp_timer_get_time() / 1000000;
}

static float calculate_amplitude(const float *peaks, uint8_t count)
{
    if (count < 2) return 0.0f;

    float sum = 0.0f;
    for (uint8_t i = 0; i < count - 1; i++)
    {
        sum += fabsf(peaks[i + 1] - peaks[i]);
    }
    return sum / (count - 1);
}

static float calculate_period(const uint32_t *timestamps, uint8_t count)
{
    if (count < 2) return 0.0f;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < count - 1; i++)
    {
        sum += (timestamps[i + 1] - timestamps[i]);
    }
    return (float)sum / (count - 1);
}

// =============================================================================
// Public API Implementation
// =============================================================================

autotune_config_t pid_autotune_default_config(float setpoint)
{
    autotune_config_t config = {
        .setpoint = setpoint,
        .output_step = 50.0f,         // 50% relay step
        .noise_band = 0.5f,            // 0.5°C noise tolerance
        .max_cycles = 10,              // Observe 10 oscillations
        .timeout_seconds = 600,        // 10 minute timeout
        .initial_output = 20.0f,       // Start at 20% output
    };
    return config;
}

void pid_autotune_init(autotune_context_t *ctx,
                       autotune_config_t config,
                       tuning_rule_t rule)
{
    if (!ctx) return;

    memset(ctx, 0, sizeof(autotune_context_t));

    ctx->config = config;
    ctx->rule = rule;
    ctx->state = AUTOTUNE_STATE_IDLE;

    ESP_LOGI(TAG, "Auto-tune initialized: setpoint=%.1f°C, rule=%d",
             config.setpoint, rule);
}

bool pid_autotune_start(autotune_context_t *ctx)
{
    if (!ctx) return false;

    if (ctx->state != AUTOTUNE_STATE_IDLE)
    {
        ESP_LOGW(TAG, "Auto-tune already running");
        return false;
    }

    // Reset state
    ctx->state = AUTOTUNE_STATE_RELAY_STEP_UP;
    ctx->start_time_sec = get_time_sec();
    ctx->relay_output_high = true;
    ctx->peak_count = 0;
    ctx->peak_index = 0;
    ctx->just_changed = false;
    ctx->is_peak_detected = false;
    ctx->running_output = ctx->config.initial_output;

    ESP_LOGI(TAG, "Auto-tune started");
    return true;
}

float pid_autotune_update(autotune_context_t *ctx, float input)
{
    if (!ctx) return 0.0f;

    // Check timeout
    uint32_t elapsed = get_time_sec() - ctx->start_time_sec;
    if (elapsed > ctx->config.timeout_seconds)
    {
        ESP_LOGE(TAG, "Auto-tune timeout after %lu seconds", elapsed);
        ctx->state = AUTOTUNE_STATE_FAILED;
        return 0.0f;
    }

    // State machine
    switch (ctx->state)
    {
    case AUTOTUNE_STATE_IDLE:
    case AUTOTUNE_STATE_COMPLETE:
    case AUTOTUNE_STATE_FAILED:
        return 0.0f;

    case AUTOTUNE_STATE_RELAY_STEP_UP:
    case AUTOTUNE_STATE_RELAY_STEP_DOWN:
    case AUTOTUNE_STATE_MEASURE_PERIOD:
    {
        // Relay feedback control with hysteresis
        float setpoint = ctx->config.setpoint;
        float noise_band = ctx->config.noise_band;

        // Detect crossing with hysteresis
        bool should_switch = false;

        if (ctx->relay_output_high)
        {
            // Output is high, wait for input to exceed setpoint + noise_band
            if (input > (setpoint + noise_band))
            {
                should_switch = true;
            }
        }
        else
        {
            // Output is low, wait for input to fall below setpoint - noise_band
            if (input < (setpoint - noise_band))
            {
                should_switch = true;
            }
        }

        // Switch relay and detect peaks
        if (should_switch)
        {
            ctx->relay_output_high = !ctx->relay_output_high;
            ctx->just_changed = true;

            uint32_t now = get_time_sec();

            // Store peak value and timestamp
            if (ctx->peak_count < 10)
            {
                if (ctx->relay_output_high)
                {
                    // Just switched to high, so we found a low peak
                    ctx->peak_low[ctx->peak_count] = input;
                }
                else
                {
                    // Just switched to low, so we found a high peak
                    ctx->peak_high[ctx->peak_count] = input;
                }

                ctx->peak_timestamps[ctx->peak_count] = now;
                ctx->peak_count++;

                ESP_LOGD(TAG, "Peak %d detected: %.2f°C at %lu sec",
                         ctx->peak_count, input, now);
            }

            // Check if we have enough data
            if (ctx->peak_count >= ctx->config.max_cycles)
            {
                ctx->state = AUTOTUNE_STATE_CALCULATING;
                ESP_LOGI(TAG, "Enough peaks collected, calculating parameters");
            }
        }

        // Calculate output
        if (ctx->relay_output_high)
        {
            ctx->running_output = ctx->config.initial_output + ctx->config.output_step;
        }
        else
        {
            ctx->running_output = ctx->config.initial_output - ctx->config.output_step;
        }

        // Clamp output
        if (ctx->running_output > 100.0f) ctx->running_output = 100.0f;
        if (ctx->running_output < 0.0f) ctx->running_output = 0.0f;

        ctx->last_input = input;
        return ctx->running_output;
    }

    case AUTOTUNE_STATE_CALCULATING:
    {
        // Calculate ultimate gain and period from collected data

        // Calculate amplitude (average peak-to-peak)
        float amplitude_high = calculate_amplitude(ctx->peak_high, ctx->peak_count);
        float amplitude_low = calculate_amplitude(ctx->peak_low, ctx->peak_count);
        float amplitude = (amplitude_high + amplitude_low) / 2.0f;

        // Calculate period (average time between peaks)
        float period = calculate_period(ctx->peak_timestamps, ctx->peak_count);

        if (amplitude < 0.1f || period < 1.0f)
        {
            ESP_LOGE(TAG, "Invalid oscillation detected: amp=%.2f, period=%.1f",
                     amplitude, period);
            ctx->state = AUTOTUNE_STATE_FAILED;
            return 0.0f;
        }

        // Calculate ultimate gain: Ku = 4 * d / (π * a)
        // where d is relay amplitude and a is oscillation amplitude
        float relay_amplitude = ctx->config.output_step;
        float ku = (4.0f * relay_amplitude) / (M_PI * amplitude);

        // Get tuning rule coefficients
        tuning_coefficients_t coeffs = tuning_rules[ctx->rule];

        // Calculate PID parameters
        float kp = ku * coeffs.kp_factor;
        float ti = period / coeffs.ti_factor;  // Integral time
        float td = period * coeffs.td_factor;  // Derivative time

        // Convert to PID gains
        float ki = kp / ti;
        float kd = kp * td;

        // Store results
        ctx->result.kp = kp;
        ctx->result.ki = ki;
        ctx->result.kd = kd;
        ctx->result.ultimate_gain = ku;
        ctx->result.ultimate_period = period;
        ctx->result.cycles_observed = ctx->peak_count;
        ctx->result.final_state = AUTOTUNE_STATE_COMPLETE;

        ctx->state = AUTOTUNE_STATE_COMPLETE;

        ESP_LOGI(TAG, "Auto-tune complete!");
        ESP_LOGI(TAG, "  Ultimate gain (Ku): %.3f", ku);
        ESP_LOGI(TAG, "  Ultimate period (Tu): %.1f seconds", period);
        ESP_LOGI(TAG, "  Calculated Kp: %.3f", kp);
        ESP_LOGI(TAG, "  Calculated Ki: %.3f", ki);
        ESP_LOGI(TAG, "  Calculated Kd: %.3f", kd);

        return 0.0f;
    }
    }

    return 0.0f;
}

bool pid_autotune_is_complete(const autotune_context_t *ctx)
{
    if (!ctx) return false;
    return (ctx->state == AUTOTUNE_STATE_COMPLETE ||
            ctx->state == AUTOTUNE_STATE_FAILED);
}

bool pid_autotune_get_result(const autotune_context_t *ctx,
                             autotune_result_t *result)
{
    if (!ctx || !result) return false;

    if (ctx->state != AUTOTUNE_STATE_COMPLETE)
    {
        return false;
    }

    *result = ctx->result;
    return true;
}

bool pid_autotune_apply_result(const autotune_context_t *ctx,
                               pid_controller_t *pid)
{
    if (!ctx || !pid) return false;

    if (ctx->state != AUTOTUNE_STATE_COMPLETE)
    {
        ESP_LOGW(TAG, "Cannot apply result: auto-tune not complete");
        return false;
    }

    pid_config_t config = {
        .kp = ctx->result.kp,
        .ki = ctx->result.ki,
        .kd = ctx->result.kd,
        .setpoint = ctx->config.setpoint,
        .output_min = 0.0f,
        .output_max = 100.0f,
    };

    pid_controller_init(pid, config);

    ESP_LOGI(TAG, "Applied auto-tune results to PID controller");
    return true;
}

void pid_autotune_cancel(autotune_context_t *ctx)
{
    if (!ctx) return;

    ESP_LOGI(TAG, "Auto-tune cancelled");
    ctx->state = AUTOTUNE_STATE_IDLE;
}

autotune_state_t pid_autotune_get_state(const autotune_context_t *ctx)
{
    if (!ctx) return AUTOTUNE_STATE_IDLE;
    return ctx->state;
}

uint8_t pid_autotune_get_progress(const autotune_context_t *ctx)
{
    if (!ctx) return 0;

    switch (ctx->state)
    {
    case AUTOTUNE_STATE_IDLE:
        return 0;

    case AUTOTUNE_STATE_RELAY_STEP_UP:
    case AUTOTUNE_STATE_RELAY_STEP_DOWN:
    case AUTOTUNE_STATE_MEASURE_PERIOD:
        // Progress based on peaks collected
        if (ctx->config.max_cycles == 0) return 50;
        return (ctx->peak_count * 90) / ctx->config.max_cycles;

    case AUTOTUNE_STATE_CALCULATING:
        return 95;

    case AUTOTUNE_STATE_COMPLETE:
        return 100;

    case AUTOTUNE_STATE_FAILED:
        return 0;
    }

    return 0;
}
