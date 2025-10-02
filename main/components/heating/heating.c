#include "heating.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "heating";

// Pin definitions
#define SSR_PIN 17
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_FREQUENCY 1000 // 1kHz for PWM

// PID state
static pid_config_t pid_config;
static float pid_integral = 0.0f;
static float pid_prev_error = 0.0f;
static uint32_t pid_last_time = 0;

esp_err_t heating_init(void)
{
    esp_err_t ret;

    // Configure GPIO for SSR (output)
    gpio_config_t ssr_conf = {
        .pin_bit_mask = (1ULL << SSR_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&ssr_conf);
    if (ret != ESP_OK)
        return ret;

    // Configure LEDC for PWM control
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT, // 0-255
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK)
        return ret;

    ledc_channel_config_t channel_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SSR_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK)
        return ret;

    // Initialize to off
    heating_set_power(0);

    ESP_LOGI(TAG, "Heating system initialized");
    return ESP_OK;
}

void heating_set_power(uint8_t power_percent)
{
    if (power_percent > 100)
        power_percent = 100;

    uint32_t duty = (power_percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

void heating_emergency_shutoff(void)
{
    heating_set_power(0);
    ESP_LOGW(TAG, "Emergency shutoff activated");
}

bool heating_is_active(void)
{
    uint32_t duty;
    ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, &duty);
    return duty > 0;
}

void pid_init(pid_config_t config)
{
    pid_config = config;
    pid_integral = 0.0f;
    pid_prev_error = 0.0f;
    pid_last_time = 0;
}

float pid_update(float current_temp)
{
    uint32_t now = esp_timer_get_time() / 1000; // ms to us, but we'll treat as time units
    if (pid_last_time == 0)
    {
        pid_last_time = now;
        return 0.0f;
    }

    float dt = (now - pid_last_time) / 1000000.0f; // Convert to seconds
    pid_last_time = now;

    float error = pid_config.setpoint - current_temp;

    // Proportional
    float p_term = pid_config.kp * error;

    // Integral
    pid_integral += error * dt;
    // Anti-windup
    if (pid_integral > pid_config.output_max)
        pid_integral = pid_config.output_max;
    if (pid_integral < pid_config.output_min)
        pid_integral = pid_config.output_min;
    float i_term = pid_config.ki * pid_integral;

    // Derivative
    float derivative = (error - pid_prev_error) / dt;
    float d_term = pid_config.kd * derivative;
    pid_prev_error = error;

    // Calculate output
    float output = p_term + i_term + d_term;

    // Clamp output
    if (output > pid_config.output_max)
        output = pid_config.output_max;
    if (output < pid_config.output_min)
        output = pid_config.output_min;

    return output;
}
