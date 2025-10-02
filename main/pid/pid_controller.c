/**
 * @file pid_controller.c
 * @brief PID temperature controller implementation
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "pid_controller.h"
#include "system_config.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "pid_controller";

void pid_controller_init(pid_controller_t *pid, pid_config_t config)
{
    if (!pid)
    {
        ESP_LOGE(TAG, "NULL PID controller pointer");
        return;
    }

    pid->config = config;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->last_update_us = esp_timer_get_time();
    pid->last_output = 0.0f;

    ESP_LOGI(TAG, "PID controller initialized: Kp=%.2f, Ki=%.2f, Kd=%.2f, setpoint=%.1f°C",
             config.kp, config.ki, config.kd, config.setpoint);
}

float pid_controller_update(pid_controller_t *pid, float measurement)
{
    if (!pid)
    {
        ESP_LOGE(TAG, "NULL PID controller pointer");
        return 0.0f;
    }

    uint64_t now = esp_timer_get_time();
    float dt = (now - pid->last_update_us) / 1000000.0f; // Convert to seconds

    // Check minimum update interval to reduce jitter
    if (dt < (PID_MIN_UPDATE_INTERVAL_MS / 1000.0f))
    {
        return pid->last_output; // Return cached output
    }

    pid->last_update_us = now;

    // Prevent division by zero or negative time
    if (dt <= 0.0f)
    {
        dt = 0.001f; // Minimum time step
    }

    // Calculate error (setpoint - measurement)
    float error = pid->config.setpoint - measurement;

    // Proportional term
    float p_term = pid->config.kp * error;

    // Integral term with anti-windup protection
    pid->integral += error * dt;

    // Clamp integral to prevent windup
    float integral_limit = pid->config.output_max;
    if (pid->integral > integral_limit)
    {
        pid->integral = integral_limit;
    }
    else if (pid->integral < -integral_limit)
    {
        pid->integral = -integral_limit;
    }

    float i_term = pid->config.ki * pid->integral;

    // Derivative term (with derivative on measurement to avoid kick)
    float derivative = (error - pid->prev_error) / dt;
    float d_term = pid->config.kd * derivative;
    pid->prev_error = error;

    // Calculate total output
    float output = p_term + i_term + d_term;

    // Clamp output to configured limits
    output = CLAMP(output, pid->config.output_min, pid->config.output_max);

    pid->last_output = output;

    ESP_LOGD(TAG, "PID update: temp=%.2f°C, error=%.2f, P=%.2f, I=%.2f, D=%.2f, output=%.2f",
             measurement, error, p_term, i_term, d_term, output);

    return output;
}

void pid_controller_reset(pid_controller_t *pid)
{
    if (!pid)
    {
        ESP_LOGE(TAG, "NULL PID controller pointer");
        return;
    }

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->last_update_us = esp_timer_get_time();
    pid->last_output = 0.0f;

    ESP_LOGI(TAG, "PID controller reset");
}

void pid_controller_set_setpoint(pid_controller_t *pid, float new_setpoint, bool reset_integral)
{
    if (!pid)
    {
        ESP_LOGE(TAG, "NULL PID controller pointer");
        return;
    }

    ESP_LOGI(TAG, "PID setpoint changed: %.1f°C → %.1f°C", pid->config.setpoint, new_setpoint);

    pid->config.setpoint = new_setpoint;

    if (reset_integral)
    {
        pid->integral = 0.0f;
        ESP_LOGD(TAG, "PID integral reset on setpoint change");
    }
}

float pid_controller_get_output(const pid_controller_t *pid)
{
    if (!pid)
    {
        ESP_LOGE(TAG, "NULL PID controller pointer");
        return 0.0f;
    }

    return pid->last_output;
}
