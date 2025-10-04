/**
 * @file heating.c
 * @brief Heating element control and PID temperature regulation
 *
 * This module provides heating control functionality including:
 * - PWM control of solid state relay (SSR) for heating element
 * - PID temperature controller with configurable parameters
 * - Emergency shutoff capabilities
 * - Power level control and status monitoring
 *
 * The heating system uses LEDC PWM to control the SSR, allowing precise
 * power control from 0-100%. The PID controller maintains target temperature
 * with configurable proportional, integral, and derivative terms.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "heating_contract.h"
#include "pid_controller.h"
#include "system_config.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_timer.h"

static const char *TAG = "heating";

// LEDC PWM Configuration
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT ///< 10-bit resolution (0-1023)
#define LEDC_FREQUENCY 1000             ///< 1 kHz PWM frequency
#define SSR_PIN GPIO_NUM_2              ///< GPIO pin connected to SSR

// PID Controller Instance
static pid_controller_t g_pid_controller;

/**
 * @brief Initialize the heating control system
 *
 * Configures LEDC PWM timer and channel for SSR control, and initializes
 * PID controller state. Must be called before any heating operations.
 *
 * @return ESP_OK on success, ESP_FAIL on LEDC configuration failure
 */
esp_err_t heating_init(void)
{
    ESP_LOGI(TAG, "Initializing heating control system");

    // Configure LEDC timer for PWM generation
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES, // 10-bit resolution
        .freq_hz = LEDC_FREQUENCY,        // 1 kHz PWM frequency
        .clk_cfg = LEDC_AUTO_CLK,         // Auto-select clock source
    };

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "LEDC timer configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel for SSR control
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE, // No interrupts needed
        .gpio_num = SSR_PIN,            // GPIO connected to SSR
        .duty = 0,                      // Start with heating off
        .hpoint = 0,                    // No phase shift
    };

    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "LEDC channel configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Heating control system initialized successfully");
    return ESP_OK;
}

/**
 * @brief Set heating power level
 *
 * Controls the SSR duty cycle to set heating power from 0-100%.
 * Values above 100% are clamped to 100% with a warning.
 *
 * @param power_percent Power level as percentage (0-100)
 */
void heating_set_power(uint8_t power_percent)
{
    if (power_percent > HEATING_POWER_MAX_PERCENT)
    {
        ESP_LOGW(TAG, "Power clamped from %d%% to %d%%",
                 power_percent, HEATING_POWER_MAX_PERCENT);
        power_percent = HEATING_POWER_MAX_PERCENT;
    }

    // Convert percentage to LEDC duty cycle (0-1023 for 10-bit resolution)
    uint32_t duty = (power_percent * ((1 << LEDC_DUTY_RES) - 1)) / 100;

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    ESP_LOGD(TAG, "Heating power set to %d%% (duty: %d)", power_percent, duty);
}

/**
 * @brief Emergency shutoff of heating system
 *
 * Immediately disables all heating output. Used for critical safety conditions.
 * Logs the emergency action for system monitoring.
 */
void heating_emergency_shutoff(void)
{
    ESP_LOGW(TAG, "Emergency heating shutoff activated");
    heating_set_power(HEATING_POWER_MIN_PERCENT);
}

/**
 * @brief Check if heating is currently active
 *
 * @return true if heating power is above 0%, false if off
 */
bool heating_is_active(void)
{
    uint32_t duty = ledc_get_duty(LEDC_MODE, LEDC_CHANNEL);
    return duty > 0;
}

/**
 * @brief Initialize PID controller with configuration
 *
 * Sets up the PID controller with the specified parameters and resets
 * internal state (integral accumulator, previous error, timing).
 * This is a wrapper for backward compatibility with the heating contract.
 *
 * @param config PID configuration structure with Kp, Ki, Kd, setpoint, and limits
 */
void pid_init(pid_config_t config)
{
    pid_controller_init(&g_pid_controller, config);
}

/**
 * @brief Update PID controller and calculate output
 *
 * Wrapper function for backward compatibility with heating contract.
 * Delegates to the PID controller module.
 *
 * @param current_temp Current temperature reading (°C)
 * @return PID output value (typically 0-100 for heating power)
 */
float pid_update(float current_temp)
{
    return pid_controller_update(&g_pid_controller, current_temp);
}
