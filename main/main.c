/**
 * @file main.c
 * @brief Main application for Insta Retrofit heat press automation system
 *
 * This file contains the main application logic for the ESP32-S3 based heat press
 * automation system. It implements:
 * - FreeRTOS task management for UI, temperature control, and system monitoring
 * - Comprehensive safety mechanisms and emergency shutdown systems
 * - Pressing cycle management with timing and progress tracking
 * - Component initialization and error handling
 * - Persistent storage management for settings and statistics
 *
 * The system provides automated temperature control via PID, user interface via
 * OLED display and rotary encoder, and industrial-grade safety features for
 * high-temperature heat press operations.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"

// Component contracts
#include "sensor_contract.h"
#include "display_contract.h"
#include "controls_contract.h"
#include "heating_contract.h"
#include "storage_contract.h"

// Internal modules
#include "data_model.h"
#include "ui_state.h"
#include "main.h"
#include "pid_autotune.h"  // NEW: Auto-tune support

// Task definitions
#define UI_TASK_STACK_SIZE 4096
#define TEMP_CONTROL_TASK_STACK_SIZE 4096
#define WATCHDOG_TASK_STACK_SIZE 2048
#define UI_TASK_PRIORITY 5
#define TEMP_CONTROL_TASK_PRIORITY 4
#define WATCHDOG_TASK_PRIORITY 3

// Safety limits - Critical safety parameters
const float MAX_TEMPERATURE = 220.0f;       ///< Maximum safe temperature (°C) - heating disabled above this
const uint32_t MAX_CYCLE_TIME = 300;        ///< Maximum cycle time (seconds) - 5 minutes safety limit
const uint8_t SENSOR_RETRY_COUNT = 3;       ///< Number of sensor read retry attempts
const uint32_t SENSOR_RETRY_DELAY_MS = 500; ///< Delay between sensor retry attempts (ms)
const float TEMP_HYSTERESIS = 5.0f;         ///< Temperature hysteresis for PID control (°C)
const uint32_t HEAP_MINIMUM = 8192;         ///< Minimum free heap memory (bytes) - 8KB

// Timing constants
const uint32_t UI_TASK_TIMEOUT_SEC = 1;           ///< UI task watchdog timeout (seconds)
const uint32_t TEMP_TASK_TIMEOUT_SEC = 3;         ///< Temperature task watchdog timeout (seconds)
const uint32_t SENSOR_TIMEOUT_SEC = 30;           ///< Maximum time without sensor reading (seconds)
const uint32_t SENSOR_VALIDATION_TIMEOUT_SEC = 10; ///< Sensor validation timeout for cycle start (seconds)

// Temperature validation constants
const float TEMP_PRESSING_MAX_OFFSET = 20.0f;   ///< Maximum temperature offset during pressing (°C)
const float TEMP_CYCLE_START_MAX_OFFSET = 30.0f; ///< Maximum temperature offset for cycle start (°C)
const float TEMP_CYCLE_START_MIN = 20.0f;       ///< Minimum temperature for cycle start (°C)
const float TEMP_RECOVERY_OFFSET = 10.0f;       ///< Temperature offset for error recovery (°C)

static const char *TAG = "main";

// =============================================================================
// Global State Variables
// =============================================================================

// Configuration and data models
settings_t settings;              ///< Global settings configuration
print_run_t print_run;            ///< Current print run data
pressing_cycle_t current_cycle;   ///< Current pressing cycle state
statistics_t statistics;          ///< Comprehensive statistics tracking
float current_temperature = 0.0f; ///< Current temperature reading (°C)

// Pressing cycle state management
bool pressing_active = false;        ///< Whether a pressing cycle is currently active
uint32_t run_start_time = 0;         ///< Timestamp when the first cycle of the run started
uint32_t cycle_start_time = 0;       ///< Timestamp when current cycle started
uint32_t stage_start_time = 0;       ///< Timestamp when current stage started
cycle_status_t current_stage = IDLE; ///< Current cycle stage

// Temperature tracking for debugging
uint32_t system_start_time = 0;      ///< Timestamp when system started (for heat-up tracking)
uint32_t time_to_target_temp = 0;    ///< Time in seconds to reach target temperature (0 = not yet reached)
bool target_temp_reached = false;    ///< Whether target temperature has been reached for the first time

// Error state and safety management
bool emergency_shutdown = false;      ///< Emergency shutdown flag
uint8_t sensor_error_count = 0;       ///< Count of consecutive sensor read failures
uint32_t last_temp_reading = 0;       ///< Timestamp of last successful temperature reading
float last_valid_temperature = 25.0f; ///< Last valid temperature reading (°C)
bool press_safety_locked = true;      ///< Safety interlock for press operations

// Task management
static TaskHandle_t ui_task_handle;           ///< UI task handle
static TaskHandle_t temp_control_task_handle; ///< Temperature control task handle
static TaskHandle_t watchdog_task_handle;     ///< Watchdog task handle

// Task monitoring and health
static uint32_t ui_task_last_run = 0;           ///< Last execution time of UI task
static uint32_t temp_control_task_last_run = 0; ///< Last execution time of temp control task
bool system_healthy = true;                     ///< Overall system health status

// UI state tracking (moved from ui_task for testability)
static bool last_press_state = false;   ///< Previous reed switch state
static bool heating_was_on = false;     ///< Previous heating state for hysteresis
bool pause_mode = false;                ///< System pause mode flag (extern in main.h)
static uint32_t state_transition_time = 0; ///< Time when UI state transitioned (for timed messages)

// Auto-tune state (NEW)
static autotune_context_t g_autotune_ctx;  ///< Auto-tune context
static bool is_autotuning = false;         ///< Auto-tune in progress flag

// =============================================================================
// Function Prototypes
// =============================================================================

// FreeRTOS Task Functions
void ui_task(void *pvParameters);           ///< User interface task - handles display and controls
void temp_control_task(void *pvParameters); ///< Temperature control task - PID control and safety
void watchdog_task(void *pvParameters);     ///< System monitoring task - health checks and safety

// Initialization and Configuration
void init_defaults(void);        ///< Initialize default settings and data
void load_persistent_data(void); ///< Load settings and data from persistent storage
void save_persistent_data(void); ///< Save settings and data to persistent storage

// Pressing Cycle Management
void start_pressing_cycle(void);    ///< Start a new pressing cycle with safety checks
void update_pressing_cycle(void);   ///< Update cycle progress and timing
void complete_pressing_cycle(void); ///< Complete current cycle and update statistics

// Safety and Error Handling Functions
void emergency_shutdown_system(const char *reason); ///< Emergency shutdown with safety actions
bool check_system_safety(void);                     ///< Check overall system safety status
bool read_temperature_safe(float *temperature);     ///< Read temperature with retry logic
void reset_error_state(void);                       ///< Reset error state when safe
bool validate_cycle_safety(void);                   ///< Validate conditions for starting cycle

// Helper Functions
bool can_operate_normally(void);                    ///< Check if system can operate normally
void update_led_indicators(void);                   ///< Update LED indicators based on system state
void handle_pause_button(void);                     ///< Handle pause button press
void control_heating_with_hysteresis(float pid_output); ///< Control heating with hysteresis logic

/**
 * @brief Main application entry point
 *
 * Initializes the Insta Retrofit heat press automation system with:
 * - Component initialization with error handling
 * - Default configuration loading
 * - FreeRTOS task creation for concurrent operations
 * - Safety system initialization
 *
 * The system will halt safely if critical components fail to initialize.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Insta Retrofit application");

    // Initialize defaults first
    init_defaults();

    // Initialize components with comprehensive error handling
    esp_err_t err;
    bool init_success = true;

    ESP_LOGI(TAG, "Initializing system components...");

    // Initialize storage FIRST before loading data
    err = storage_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize persistent storage: %s", esp_err_to_name(err));
        init_success = false;
    }

    // Load persistent data AFTER storage is initialized (may override defaults)
    load_persistent_data();

    err = sensor_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize temperature sensor: %s", esp_err_to_name(err));
        init_success = false;
    }

    err = display_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize OLED display: %s", esp_err_to_name(err));
        init_success = false;
    }

    err = controls_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize user controls: %s", esp_err_to_name(err));
        init_success = false;
    }

    err = heating_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize heating system: %s", esp_err_to_name(err));
        init_success = false;
    }

    // Critical safety check: halt if any component failed
    if (!init_success)
    {
        ESP_LOGE(TAG, "Component initialization failed - system cannot start safely");
        emergency_shutdown_system("Component initialization failure");
        return;
    }

    ESP_LOGI(TAG, "All components initialized successfully");

    // Initialize PID controller with current settings
    pid_config_t pid_config = {
        .kp = settings.pid_kp,
        .ki = settings.pid_ki,
        .kd = settings.pid_kd,
        .setpoint = settings.target_temp,
        .output_min = 0.0f,
        .output_max = 100.0f};
    pid_init(pid_config);

    // Initialize user interface
    ui_init(&settings, &print_run);

    // Initialize safety state - start with all safety systems engaged
    emergency_shutdown = false;
    sensor_error_count = 0;
    last_temp_reading = esp_timer_get_time() / 1000000;
    press_safety_locked = true; // Start with safety lock engaged

    // Initialize temperature tracking for heat-up time measurement
    system_start_time = esp_timer_get_time() / 1000000;
    target_temp_reached = false;
    time_to_target_temp = 0;
    pause_mode = false;         // Start without pause
    heating_was_on = false;     // Start with heating off
    last_press_state = false;   // Initialize press state

    // Initialize task monitoring timestamps
    ui_task_last_run = esp_timer_get_time() / 1000000;
    temp_control_task_last_run = esp_timer_get_time() / 1000000;
    system_healthy = true;

    // Initialize LED indicators (all off at startup)
    controls_set_led_green(false);
    controls_set_led_blue(false);

    ESP_LOGI(TAG, "Creating FreeRTOS tasks...");

    // Create concurrent tasks for system operation
    xTaskCreate(ui_task, "UI Task", UI_TASK_STACK_SIZE, NULL, UI_TASK_PRIORITY, &ui_task_handle);
    xTaskCreate(temp_control_task, "Temp Control", TEMP_CONTROL_TASK_STACK_SIZE, NULL, TEMP_CONTROL_TASK_PRIORITY, &temp_control_task_handle);
    xTaskCreate(watchdog_task, "Watchdog", WATCHDOG_TASK_STACK_SIZE, NULL, WATCHDOG_TASK_PRIORITY, &watchdog_task_handle);

    ESP_LOGI(TAG, "Insta Retrofit system initialized successfully with safety mechanisms active");
}

/**
 * @brief User Interface Task
 *
 * Handles user interface operations including:
 * - Display updates with current temperature and status
 * - Rotary encoder and button input processing
 * - Press cycle initiation with safety interlocks
 * - Menu navigation and state management
 *
 * Runs at 100ms intervals for responsive user interaction.
 *
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void ui_task(void *pvParameters)
{
    (void)pvParameters; // Suppress unused parameter warning

    while (1)
    {
        ui_task_last_run = esp_timer_get_time() / 1000000;

        // Update LED indicators
        update_led_indicators();

        // Safety check - don't update UI if system is in emergency shutdown
        if (!emergency_shutdown)
        {
            ui_update(current_temperature);
        }

        // Handle pause button AFTER UI update (so UI gets button events first)
        handle_pause_button();

        // Get current state
        uint32_t current_time = esp_timer_get_time() / 1000000;
        ui_state_t current_ui_state = ui_get_current_state();

        // Monitor press state changes for cycle control
        bool current_press_state = controls_is_press_closed();

        // Safety interlock: only allow pressing if all safety checks pass and not paused
        bool safety_ok = check_system_safety() && !emergency_shutdown && !pause_mode;

        if (current_press_state && !last_press_state && safety_ok)
        {
            ESP_LOGI(TAG, "Press closed detected. UI state: %d, pressing_active: %d, current_stage: %d",
                     current_ui_state, pressing_active, current_stage);

            // Check if we're in READY state waiting for Stage 2
            if (current_ui_state == UI_STATE_STAGE2_READY && pressing_active)
            {
                // Start Stage 2
                ESP_LOGI(TAG, "Press closed - starting Stage 2");
                current_stage = STAGE2;
                stage_start_time = esp_timer_get_time() / 1000000;
                current_cycle.status = STAGE2;
                ui_set_state(UI_STATE_PRESSING_ACTIVE);
                state_transition_time = current_time;
            }
            // Check if we're in cycle complete state - start new cycle immediately
            else if (current_ui_state == UI_STATE_CYCLE_COMPLETE && validate_cycle_safety())
            {
                press_safety_locked = false; // Release safety lock for validated cycle
                start_pressing_cycle();
                ui_set_state(UI_STATE_PRESSING_ACTIVE);
                state_transition_time = current_time;
                ESP_LOGI(TAG, "Starting next cycle from cycle complete");
            }
            // Press closed - validate conditions before starting new cycle (job mode or free press mode)
            else if ((current_ui_state == UI_STATE_START_PRESSING || current_ui_state == UI_STATE_FREE_PRESS) && validate_cycle_safety())
            {
                press_safety_locked = false; // Release safety lock for validated cycle
                start_pressing_cycle();
                ui_set_state(UI_STATE_PRESSING_ACTIVE); // Transition UI to pressing active state
                state_transition_time = current_time;
                ESP_LOGI(TAG, "Press cycle started with all safety checks passed");
            }
            else if (current_ui_state != UI_STATE_START_PRESSING && current_ui_state != UI_STATE_FREE_PRESS && !pressing_active && current_ui_state != UI_STATE_CYCLE_COMPLETE)
            {
                ESP_LOGW(TAG, "Press closed but not in valid state (current: %d)", current_ui_state);
            }
            else if (!validate_cycle_safety())
            {
                ESP_LOGW(TAG, "Press cycle blocked by safety validation failure");
            }
        }
        else if (!current_press_state && last_press_state)
        {
            // Press opened
            if (current_ui_state == UI_STATE_STAGE1_DONE)
            {
                // Transition from DONE to READY when press opens
                ui_set_state(UI_STATE_STAGE2_READY);
                state_transition_time = current_time;
                ESP_LOGI(TAG, "Press opened - transitioning to READY state");
            }
            else if (pressing_active)
            {
                if (current_stage == STAGE1)
                {
                    // Stage 1 early release - finish stage 1 and go to READY
                    ESP_LOGI(TAG, "Stage 1 early release detected");
                    current_stage = IDLE;
                    current_cycle.status = IDLE;
                    ui_set_state(UI_STATE_STAGE2_READY);
                    state_transition_time = current_time;
                }
                else if (current_stage == STAGE2)
                {
                    // Stage 2 early release or normal completion - complete the cycle
                    ESP_LOGI(TAG, "Stage 2 press opened - completing cycle");
                    complete_pressing_cycle();
                    press_safety_locked = true; // Re-engage safety lock
                    state_transition_time = current_time;
                }
            }
        }

        last_press_state = current_press_state;

        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms update rate for responsive UI
    }
}

/**
 * @brief Temperature Control Task
 *
 * Manages temperature regulation and safety monitoring including:
 * - PID-based temperature control with hysteresis
 * - Sensor reading with retry logic and error handling
 * - Emergency shutdown on temperature or sensor failures
 * - Pressing cycle timing updates
 * - Heating element control with safety interlocks
 *
 * Runs at 1-second intervals for stable temperature control.
 *
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void temp_control_task(void *pvParameters)
{
    (void)pvParameters; // Suppress unused parameter warning

    const TickType_t xDelay = pdMS_TO_TICKS(1000); // 1 second control interval

    while (1)
    {
        temp_control_task_last_run = esp_timer_get_time() / 1000000;

        // Emergency shutdown check - immediate safety response
        if (emergency_shutdown)
        {
            heating_emergency_shutoff();
            vTaskDelay(xDelay);
            continue;
        }

        // Read temperature with comprehensive safety and retry logic
        float new_temperature;
        if (read_temperature_safe(&new_temperature))
        {
            current_temperature = new_temperature;
            last_temp_reading = esp_timer_get_time() / 1000000;
            sensor_error_count = 0; // Reset error count on successful read

            // Log temperature to console
            ESP_LOGI(TAG, "Temperature: %.2f°C", current_temperature);

            // Check if auto-tuning is in progress
            if (is_autotuning)
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

                        is_autotuning = false;
                        heating_set_power(0);  // Turn off heating

                        // Transition UI to results screen
                        ui_set_state(UI_STATE_AUTOTUNE_COMPLETE);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Auto-tune failed to produce valid results");
                        is_autotuning = false;
                        heating_set_power(0);
                    }
                }
                else
                {
                    // Apply auto-tune output (relay feedback)
                    heating_set_power((uint8_t)autotune_output);
                }
            }
            else
            {
                // Normal operation: update pressing cycle timing
                update_pressing_cycle();

                // Control heating only when pressing is active, not paused, and all safety checks pass
                if (pressing_active && !press_safety_locked && check_system_safety() && !pause_mode)
                {
                    // Update PID controller with current temperature
                    float output = pid_update(current_temperature);

                    // Apply hysteresis control
                    control_heating_with_hysteresis(output);
                }
                else
                {
                    // No heating when not pressing, paused, or when safety systems are engaged
                    heating_set_power(0);
                }
            }

            // Critical safety check: emergency shutdown if temperature exceeds limit
            if (current_temperature > MAX_TEMPERATURE)
            {
                emergency_shutdown_system("Temperature exceeded maximum safe limit");

                // Cancel auto-tune if active
                if (is_autotuning)
                {
                    is_autotuning = false;
                    heating_set_power(0);
                }
            }
        }
        else
        {
            // Sensor read failed - implement retry and escalation strategy
            sensor_error_count++;
            statistics.sensor_failures++;
            ESP_LOGW(TAG, "Temperature sensor read failed (attempt %d/%d)",
                     sensor_error_count, SENSOR_RETRY_COUNT);

            // TEMPORARILY DISABLED: Emergency shutdown after maximum retry attempts
            // TODO: Re-enable after display testing
            if (false && sensor_error_count >= SENSOR_RETRY_COUNT)
            {
                emergency_shutdown_system("Temperature sensor failure - too many consecutive errors");
            }
            else
            {
                // Use last valid temperature for control during sensor errors
                current_temperature = last_valid_temperature;
                heating_set_power(0); // Safety: turn off heating during sensor errors
            }
        }

        vTaskDelay(xDelay);
    }
}
void init_defaults(void)
{
    // Default settings
    settings.target_temp = 140.0f;

    // PID defaults optimized for 2200W/230V heat press with high thermal mass
    // These conservative values minimize overshoot for safety
    // Use auto-tune for optimal tuning to your specific heat press
    settings.pid_kp = 3.5f;   // Higher proportional gain for better response to error
    settings.pid_ki = 0.05f;  // Lower integral gain to prevent overshoot with high thermal mass
    settings.pid_kd = 1.2f;   // Higher derivative gain to dampen oscillations

    settings.stage1_default = 15;
    settings.stage2_default = 5;

    // Default print run
    print_run.id = 1;
    print_run.num_shirts = 10;
    print_run.type = SINGLE_SIDED;
    print_run.progress = 0;
    print_run.time_elapsed = 0;
    print_run.shirts_completed = 0;
    print_run.avg_time_per_shirt = 0;

    // Reset run timing
    run_start_time = 0;

    // Initialize statistics to zero
    memset(&statistics, 0, sizeof(statistics_t));
    statistics.session_start_time = esp_timer_get_time() / 1000000;
}

void load_persistent_data(void)
{
    bool has_saved_data = storage_has_saved_data();
    ESP_LOGI(TAG, "Checking for saved data: %s", has_saved_data ? "FOUND" : "NOT FOUND");

    if (has_saved_data)
    {
        esp_err_t load_result = storage_load_settings(&settings);
        ESP_LOGI(TAG, "Load settings result: %s", esp_err_to_name(load_result));

        if (load_result != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to load settings, using defaults");
        }
        else if (!validate_settings(&settings))
        {
            ESP_LOGW(TAG, "Loaded settings failed validation, using defaults");
            ESP_LOGW(TAG, "  target_temp=%.1f, stage1=%d, stage2=%d",
                     settings.target_temp, settings.stage1_default, settings.stage2_default);
            init_defaults(); // Reset to defaults if validation fails
        }
        else
        {
            ESP_LOGI(TAG, "Loaded settings: target_temp=%.1f, stage1=%d, stage2=%d, Kp=%.2f, Ki=%.3f, Kd=%.2f",
                     settings.target_temp, settings.stage1_default, settings.stage2_default,
                     settings.pid_kp, settings.pid_ki, settings.pid_kd);
        }

        if (storage_load_print_run(&print_run) != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to load print run, using defaults");
        }
        else if (!validate_print_run(&print_run))
        {
            ESP_LOGW(TAG, "Loaded print run failed validation, resetting to defaults");
            print_run.id = 1;
            print_run.num_shirts = 10;
            print_run.type = SINGLE_SIDED;
            print_run.progress = 0;
            print_run.time_elapsed = 0;
            print_run.shirts_completed = 0;
            print_run.avg_time_per_shirt = 0;
            run_start_time = 0;
        }
        else if (print_run.shirts_completed > 0 && print_run.time_elapsed > 0)
        {
            // Reconstruct run_start_time from saved time_elapsed
            // This ensures continuity after a reboot mid-run
            uint32_t current_time = esp_timer_get_time() / 1000000;
            run_start_time = current_time - print_run.time_elapsed;
            ESP_LOGI(TAG, "Reconstructed run_start_time from saved data (elapsed: %lu s)",
                     print_run.time_elapsed);
        }
    }
}

void save_persistent_data(void)
{
    // Validate data before saving
    if (validate_settings(&settings))
    {
        ESP_LOGI(TAG, "Saving settings: target_temp=%.1f, stage1=%d, stage2=%d, Kp=%.2f, Ki=%.3f, Kd=%.2f",
                 settings.target_temp, settings.stage1_default, settings.stage2_default,
                 settings.pid_kp, settings.pid_ki, settings.pid_kd);
        storage_save_settings(&settings);
    }
    else
    {
        ESP_LOGE(TAG, "Settings validation failed, not saving corrupted data");
    }

    if (validate_print_run(&print_run))
    {
        storage_save_print_run(&print_run);
    }
    else
    {
        ESP_LOGE(TAG, "Print run validation failed, not saving corrupted data");
    }
}

void start_pressing_cycle(void)
{
    if (!pressing_active && !emergency_shutdown && validate_cycle_safety())
    {
        pressing_active = true;
        current_stage = STAGE1;
        cycle_start_time = esp_timer_get_time() / 1000000; // seconds
        stage_start_time = cycle_start_time;

        // Set run start time on first cycle (separate tracking for free press vs job mode)
        if (ui_is_free_press_mode())
        {
            if (ui_get_free_press_run_start_time() == 0)
            {
                ui_set_free_press_run_start_time(cycle_start_time);
            }
        }
        else
        {
            if (run_start_time == 0)
            {
                run_start_time = cycle_start_time;
            }
        }

        current_cycle.shirt_id = ui_is_free_press_mode() ? 0 : (print_run.progress + 1);
        current_cycle.side = FRONT; // Always start with front
        current_cycle.stage1_duration = settings.stage1_default;
        current_cycle.stage2_duration = settings.stage2_default;
        current_cycle.start_time = cycle_start_time;
        current_cycle.status = STAGE1;

        // Validate cycle configuration before starting
        if (!validate_pressing_cycle(&current_cycle))
        {
            ESP_LOGE(TAG, "Pressing cycle configuration validation failed");
            pressing_active = false;
            current_stage = IDLE;
            return;
        }

        ESP_LOGI(TAG, "Started pressing cycle for shirt %d with safety validation", current_cycle.shirt_id);
    }
    else if (emergency_shutdown)
    {
        ESP_LOGW(TAG, "Pressing cycle blocked - emergency shutdown active");
    }
    else if (!validate_cycle_safety())
    {
        ESP_LOGW(TAG, "Pressing cycle blocked - safety validation failed");
    }
}

void update_pressing_cycle(void)
{
    if (!pressing_active || emergency_shutdown)
        return;

    uint32_t current_time = esp_timer_get_time() / 1000000;
    uint32_t cycle_elapsed = current_time - cycle_start_time;
    uint32_t stage_elapsed = current_time - stage_start_time;

    // Safety check: maximum cycle time protection
    if (cycle_elapsed > MAX_CYCLE_TIME)
    {
        ESP_LOGE(TAG, "Cycle timeout - exceeded maximum cycle time (%d seconds)", MAX_CYCLE_TIME);
        emergency_shutdown_system("Pressing cycle exceeded maximum allowed time");
        return;
    }

    // Safety check: ensure temperature is within safe range during pressing
    if (current_temperature > (settings.target_temp + TEMP_PRESSING_MAX_OFFSET))
    {
        ESP_LOGE(TAG, "Cycle aborted - temperature too high during pressing (%.1f°C)", current_temperature);
        emergency_shutdown_system("Temperature exceeded safe limits during pressing cycle");
        return;
    }

    if (current_stage == STAGE1 && stage_elapsed >= current_cycle.stage1_duration)
    {
        // Stage 1 complete - show DONE message
        current_stage = IDLE;
        current_cycle.status = IDLE;
        ui_set_state(UI_STATE_STAGE1_DONE);
        state_transition_time = current_time;
        ESP_LOGI(TAG, "Stage 1 complete - showing DONE message");
    }
    else if (current_stage == STAGE2 && stage_elapsed >= current_cycle.stage2_duration)
    {
        // Stage 2 complete - show DONE message until press opens
        ui_set_state(UI_STATE_STAGE2_DONE);
        state_transition_time = current_time;
        ESP_LOGI(TAG, "Stage 2 complete - showing DONE message");
    }
}

void complete_pressing_cycle(void)
{
    if (pressing_active)
    {
        uint32_t current_time = esp_timer_get_time() / 1000000;
        uint32_t cycle_duration = current_time - cycle_start_time;

        // Update cycle completion
        current_cycle.status = COMPLETE;

        // Update statistics - total presses
        statistics.total_presses++;
        statistics.presses_since_pid_tune++;

        // Track temperature stability
        float temp_error = current_temperature - settings.target_temp;
        if (temp_error >= -5.0f && temp_error <= 5.0f)
        {
            statistics.presses_in_tolerance++;
        }

        // Check if we're in free press mode
        if (ui_is_free_press_mode())
        {
            // Free press mode - just increment counter and track timing
            ui_increment_free_press_count();

            // Update total elapsed time from free press run start
            uint32_t free_press_start = ui_get_free_press_run_start_time();
            if (free_press_start > 0)
            {
                uint32_t total_elapsed = current_time - free_press_start;
                ui_update_free_press_timing(total_elapsed);
            }

            ESP_LOGI(TAG, "Completed free press cycle in %d seconds", cycle_duration);
        }
        else
        {
            // Job-tracked mode - update print run
            print_run.shirts_completed++;
            print_run.progress = print_run.shirts_completed;

            // Update total elapsed time from run start (includes time between shirts)
            if (run_start_time > 0)
            {
                print_run.time_elapsed = current_time - run_start_time;
            }

            // Update average time per shirt
            if (print_run.shirts_completed > 0)
            {
                print_run.avg_time_per_shirt = print_run.time_elapsed / print_run.shirts_completed;
            }

            // Save progress
            save_persistent_data();

            ESP_LOGI(TAG, "Completed pressing cycle for shirt %d in %d seconds",
                     current_cycle.shirt_id, cycle_duration);
        }

        // Reset cycle state
        pressing_active = false;
        current_stage = IDLE;
        cycle_start_time = 0;
        stage_start_time = 0;

        // Show statistics after cycle complete
        ui_set_state(UI_STATE_CYCLE_COMPLETE);
    }
}

void watchdog_task(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(5000); // 5 second intervals

    while (1)
    {
        uint32_t current_time = esp_timer_get_time() / 1000000;

        // Check if UI task is still running (should update every 100ms)
        if ((current_time - ui_task_last_run) > UI_TASK_TIMEOUT_SEC)
        {
            ESP_LOGE(TAG, "UI task appears unresponsive!");
            system_healthy = false;
        }

        // Check if temperature control task is still running (should update every 1s)
        if ((current_time - temp_control_task_last_run) > TEMP_TASK_TIMEOUT_SEC)
        {
            ESP_LOGE(TAG, "Temperature control task appears unresponsive!");
            system_healthy = false;
            emergency_shutdown_system("Temperature control task failure");
        }

        // Check heap memory (critical for embedded systems)
        uint32_t free_heap = esp_get_free_heap_size();
        if (free_heap < HEAP_MINIMUM)
        {
            ESP_LOGE(TAG, "Critical heap memory low: %d bytes free (minimum: %d)", free_heap, HEAP_MINIMUM);
            emergency_shutdown_system("Critical memory shortage detected");
        }
        else if (free_heap < (HEAP_MINIMUM * 2))
        {
            ESP_LOGW(TAG, "Low heap memory warning: %d bytes free", free_heap);
        }

        // Check for prolonged sensor read failures
        if ((current_time - last_temp_reading) > SENSOR_TIMEOUT_SEC)
        {
            ESP_LOGE(TAG, "No valid temperature reading for %d+ seconds", SENSOR_TIMEOUT_SEC);
            emergency_shutdown_system("Temperature sensor communication lost");
        }

        // Check system health and attempt recovery if possible
        if (!system_healthy && !emergency_shutdown)
        {
            ESP_LOGW(TAG, "System health compromised - attempting recovery");

            // Try to reset error state if conditions are safe
            if (current_temperature < (settings.target_temp - TEMP_RECOVERY_OFFSET) && !pressing_active)
            {
                ESP_LOGI(TAG, "Safe conditions detected - attempting error recovery");
                reset_error_state();
            }
        }

        // Log system health status
        if (system_healthy && !emergency_shutdown)
        {
            ESP_LOGD(TAG, "System health check passed - heap: %d bytes, temp: %.1f°C",
                     free_heap, current_temperature);
        }
        else
        {
            ESP_LOGE(TAG, "System health check failed - emergency shutdown active");
        }

        vTaskDelay(xDelay);
    }
}

// Safety and error handling functions

void emergency_shutdown_system(const char *reason)
{
    if (emergency_shutdown)
    {
        return; // Already in emergency shutdown
    }

    emergency_shutdown = true;
    system_healthy = false;

    // Track emergency stop
    statistics.emergency_stops++;

    ESP_LOGE(TAG, "EMERGENCY SHUTDOWN: %s", reason);

    // Immediate safety actions
    heating_emergency_shutoff();
    pressing_active = false;
    press_safety_locked = true;
    pause_mode = false; // Clear pause mode

    // Turn off all LED indicators
    controls_set_led_green(false);
    controls_set_led_blue(false);

    // Reset cycle state
    current_stage = IDLE;
    cycle_start_time = 0;
    stage_start_time = 0;
    run_start_time = 0;

    // Log emergency state
    ESP_LOGE(TAG, "Emergency shutdown complete - system locked for safety");
}

bool check_system_safety(void)
{
    // Check temperature within safe range
    if (current_temperature > MAX_TEMPERATURE)
    {
        return false;
    }

    // Check heap memory
    if (esp_get_free_heap_size() < HEAP_MINIMUM)
    {
        return false;
    }

    // Check sensor health
    uint32_t current_time = esp_timer_get_time() / 1000000;
    if ((current_time - last_temp_reading) > SENSOR_TIMEOUT_SEC)
    {
        return false;
    }

    // Check emergency shutdown state
    if (emergency_shutdown)
    {
        return false;
    }

    return true;
}

bool read_temperature_safe(float *temperature)
{
    // Try to read temperature with retry logic
    for (int attempt = 0; attempt < SENSOR_RETRY_COUNT; attempt++)
    {
        if (sensor_read_temperature(temperature))
        {
            last_valid_temperature = *temperature;
            return true;
        }

        // Wait before retry
        if (attempt < (SENSOR_RETRY_COUNT - 1))
        {
            vTaskDelay(pdMS_TO_TICKS(SENSOR_RETRY_DELAY_MS));
        }
    }

    return false;
}

void reset_error_state(void)
{
    // Only attempt recovery if in emergency shutdown
    if (!emergency_shutdown)
    {
        return;
    }

    // Check all safety conditions before recovery
    bool temp_safe = (current_temperature >= TEMP_CYCLE_START_MIN) &&
                     (current_temperature < (settings.target_temp + TEMP_RECOVERY_OFFSET));
    bool heap_safe = esp_get_free_heap_size() > (HEAP_MINIMUM * 2);
    bool sensor_responding = sensor_is_operational();
    uint32_t current_time = esp_timer_get_time() / 1000000;
    bool recent_sensor_reading = (current_time - last_temp_reading) < SENSOR_VALIDATION_TIMEOUT_SEC;

    // All conditions must be met for recovery
    if (temp_safe && heap_safe && sensor_responding && recent_sensor_reading && !pressing_active)
    {
        ESP_LOGI(TAG, "Resetting error state - all safety conditions met");
        ESP_LOGI(TAG, "  Temperature: %.1f°C (safe range)", current_temperature);
        ESP_LOGI(TAG, "  Heap: %d bytes free", esp_get_free_heap_size());
        ESP_LOGI(TAG, "  Sensor: operational");

        emergency_shutdown = false;
        system_healthy = true;
        sensor_error_count = 0;
        press_safety_locked = true;
        pause_mode = false;
        heating_was_on = false;

        // Re-initialize LED indicators
        controls_set_led_green(false);
        controls_set_led_blue(false);

        ESP_LOGI(TAG, "Error state reset complete - system ready for operation");
    }
    else
    {
        ESP_LOGW(TAG, "Cannot reset error state - safety conditions not met:");
        if (!temp_safe) ESP_LOGW(TAG, "  Temperature unsafe: %.1f°C", current_temperature);
        if (!heap_safe) ESP_LOGW(TAG, "  Low heap: %d bytes", esp_get_free_heap_size());
        if (!sensor_responding) ESP_LOGW(TAG, "  Sensor not operational");
        if (!recent_sensor_reading) ESP_LOGW(TAG, "  No recent sensor reading");
        if (pressing_active) ESP_LOGW(TAG, "  Pressing cycle active");
    }
}

bool validate_cycle_safety(void)
{
    // Check system is healthy
    if (!system_healthy || emergency_shutdown)
    {
        ESP_LOGW(TAG, "Cycle safety: system not healthy");
        return false;
    }

    // Check temperature is reasonable for starting a cycle
    if (current_temperature > (settings.target_temp + TEMP_CYCLE_START_MAX_OFFSET))
    {
        ESP_LOGW(TAG, "Cycle safety: temperature too high to start cycle (%.1f°C)", current_temperature);
        return false;
    }

    if (current_temperature < TEMP_CYCLE_START_MIN)
    {
        ESP_LOGW(TAG, "Cycle safety: temperature too low (%.1f°C)", current_temperature);
        return false;
    }

    // Check memory is sufficient
    if (esp_get_free_heap_size() < HEAP_MINIMUM)
    {
        ESP_LOGW(TAG, "Cycle safety: insufficient memory");
        return false;
    }

    // Check sensor is working
    uint32_t current_time = esp_timer_get_time() / 1000000;
    if ((current_time - last_temp_reading) > SENSOR_VALIDATION_TIMEOUT_SEC)
    {
        ESP_LOGW(TAG, "Cycle safety: temperature sensor not responding");
        return false;
    }

    ESP_LOGD(TAG, "Cycle safety validation passed");
    return true;
}

/**
 * @brief Check if system can operate normally
 *
 * Helper function to check common conditions for normal operation.
 *
 * @return true if system can operate normally, false otherwise
 */
bool can_operate_normally(void)
{
    return !pressing_active && !emergency_shutdown && !pause_mode;
}

/**
 * @brief Update LED indicators based on system state
 *
 * Controls the green LED (temperature ready) and blue LED (pause mode)
 * according to current system conditions.
 */
void update_led_indicators(void)
{
    // Green LED: Temperature is within range of target (ready for pressing)
    bool temp_ready = (current_temperature >= (settings.target_temp - 5.0f)) &&
                      (current_temperature <= (settings.target_temp + 5.0f)) &&
                      !emergency_shutdown;
    controls_set_led_green(temp_ready);

    // Track time to reach target temperature for the first time (for PID debugging)
    if (!target_temp_reached && temp_ready && system_start_time > 0)
    {
        uint32_t current_time = esp_timer_get_time() / 1000000;
        time_to_target_temp = current_time - system_start_time;
        target_temp_reached = true;

        // Update warmup statistics
        statistics.total_warmup_time += time_to_target_temp;
        statistics.warmup_count++;
        statistics.avg_warmup_time = (float)statistics.total_warmup_time / statistics.warmup_count;

        ESP_LOGI(TAG, "Target temperature reached in %lu seconds (avg: %.1fs)",
                 time_to_target_temp, statistics.avg_warmup_time);
    }

    // Blue LED: System is in pause mode
    controls_set_led_blue(pause_mode);
}

/**
 * @brief Handle pause button press
 *
 * Toggles pause mode when the pause button is pressed.
 * Pause mode prevents new cycles from starting and disables heating.
 */
void handle_pause_button(void)
{
    button_event_t button = controls_get_button_event();
    if (button == BUTTON_PAUSE)
    {
        pause_mode = !pause_mode;
        if (pause_mode)
        {
            ESP_LOGI(TAG, "Pause mode activated");
            // Turn off heating when paused
            heating_set_power(0);
        }
        else
        {
            ESP_LOGI(TAG, "Pause mode deactivated");
        }
    }
}

/**
 * @brief Control heating with hysteresis logic
 *
 * Applies hysteresis to PID output to prevent rapid on/off cycling
 * near the setpoint temperature.
 *
 * @param pid_output PID controller output (0-100%)
 */
void control_heating_with_hysteresis(float pid_output)
{
    if (heating_was_on && current_temperature < (settings.target_temp - TEMP_HYSTERESIS))
    {
        heating_was_on = false;
    }
    else if (!heating_was_on && current_temperature > (settings.target_temp + TEMP_HYSTERESIS))
    {
        heating_was_on = true;
    }

    // Set heating power based on PID output and hysteresis state
    if (heating_was_on)
    {
        heating_set_power((uint8_t)pid_output);
    }
    else
    {
        heating_set_power(0);
    }
}

// =============================================================================
// PID Auto-Tune Functions
// =============================================================================

/**
 * @brief Start PID auto-tuning process
 *
 * Initializes and starts the PID auto-tuning procedure using the relay
 * feedback method. The system will oscillate around the target temperature
 * to determine optimal PID parameters.
 *
 * @param target_temp Target temperature for auto-tuning
 * @return true if auto-tune started successfully, false otherwise
 */
bool start_pid_autotune(float target_temp)
{
    // Safety checks before starting auto-tune
    if (emergency_shutdown)
    {
        ESP_LOGW(TAG, "Cannot start auto-tune: emergency shutdown active");
        return false;
    }

    if (pressing_active)
    {
        ESP_LOGW(TAG, "Cannot start auto-tune: pressing cycle active");
        return false;
    }

    if (is_autotuning)
    {
        ESP_LOGW(TAG, "Auto-tune already in progress");
        return false;
    }

    // Configure auto-tune with conservative settings for heat press
    autotune_config_t config = {
        .setpoint = target_temp,
        .output_step = 50.0f,           // 50% power step for relay
        .noise_band = 2.0f,             // 2°C noise band
        .max_cycles = 5,                // Observe 5 oscillation cycles
        .timeout_seconds = 1800,        // 30 minute timeout
        .initial_output = 0.0f          // Start with heating off
    };

    // Initialize auto-tune context with Tyreus-Luyben rule (conservative, minimal overshoot)
    pid_autotune_init(&g_autotune_ctx, config, TUNING_RULE_TYREUS_LUYBEN);

    // Start auto-tune
    if (!pid_autotune_start(&g_autotune_ctx))
    {
        ESP_LOGE(TAG, "Failed to start auto-tune");
        return false;
    }

    is_autotuning = true;
    ESP_LOGI(TAG, "PID auto-tune started with target temperature %.1f°C", target_temp);
    ESP_LOGI(TAG, "Using Tyreus-Luyben tuning rule for minimal overshoot");

    return true;
}

/**
 * @brief Cancel ongoing PID auto-tuning
 *
 * Safely cancels the auto-tuning process and returns to normal operation.
 */
void cancel_pid_autotune(void)
{
    if (is_autotuning)
    {
        is_autotuning = false;
        heating_set_power(0);  // Turn off heating
        ESP_LOGI(TAG, "PID auto-tune cancelled by user");
    }
}

/**
 * @brief Check if PID auto-tuning is in progress
 *
 * @return true if auto-tuning is active, false otherwise
 */
bool is_pid_autotuning(void)
{
    return is_autotuning;
}

/**
 * @brief Get current auto-tune progress percentage
 *
 * @return Progress percentage (0-100)
 */
uint8_t get_autotune_progress(void)
{
    if (!is_autotuning)
    {
        return 0;
    }

    return pid_autotune_get_progress(&g_autotune_ctx);
}
