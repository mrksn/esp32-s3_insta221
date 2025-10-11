/**
 * @file ui_renderers.c
 * @brief UI state machine rendering functions
 *
 * Contains all display rendering functions for different UI states.
 */

#include "ui_state.h"
#include "main.h"
#include "data_model.h"
#include "display_contract.h"
#include "heating_contract.h"
#include "controls_contract.h"
#include "system_config.h"  // components/system_config/include/

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static const char *TAG = "ui_renderers";

// =============================================================================
// Constants for array sizes
// =============================================================================

#define JOB_SETUP_ITEM_COUNT 2
#define JOB_ITEM_NUM_SHIRTS 0
#define PROFILE_COUNT 5  // Cotton, Polyester, Blockout, Wood, Metal

// =============================================================================
// External State Variables (defined in ui_state.c)
// =============================================================================

extern settings_t *current_settings;
extern print_run_t *current_run;
extern float temperature_display_celsius;
extern ui_callbacks_t ui_callbacks;
extern ui_state_t ui_current_state;

// Menu selection state
extern int menu_selected_item;
extern int job_setup_selected_index;
extern bool job_setup_edit_mode;
extern int job_setup_staged_num_shirts;
extern printing_type_t job_setup_staged_print_type;

extern int print_type_selected_index;
extern int settings_selected_item;
extern int timer_selected_index;
extern bool timer_edit_mode;
extern int timer_staged_value;
extern int temp_selected_index;
extern bool temp_edit_mode;
extern float temp_staged_value;
extern int pid_selected_index;
extern bool pid_edit_mode;
extern float pid_staged_value;

extern bool free_press_mode;
extern uint16_t free_press_count;
extern uint32_t free_press_time_elapsed;
extern uint32_t free_press_avg_time;

extern int profile_selected_index;
extern int stats_selected_index;

extern int reset_stats_selected_index;
extern uint32_t reset_stats_press_start_time;
extern bool reset_stats_button_pressed;

extern uint32_t heat_up_start_time;
extern float heat_up_start_temp;
extern ui_state_t heat_up_return_state;
extern bool heat_up_heating_was_active;
extern uint32_t heat_up_last_update_sec;
extern bool heat_up_screen_initialized;
extern bool display_needs_update;

// Menu arrays (defined in ui_state.c)
extern const char *main_menu_items[];
extern const char *job_setup_items[];
extern const char *print_type_items[];
extern const char *settings_menu_items[];
extern const char *timer_menu_items[];
extern const char *temp_menu_items[];
extern const char *pid_menu_items[];
extern const char *profile_items[];
extern const char *stats_menu_items[];

// Pressing cycle state (defined in main.c)
extern pressing_cycle_t current_cycle;
extern uint32_t stage_start_time;
extern cycle_status_t current_stage;
extern uint32_t system_start_time;
extern print_run_t print_run;

// Forward declarations for helper functions from ui_helpers.c
void init_free_press_mode(void);
void init_job_press_mode(void);
void perform_job_stats_reset(void);
void perform_all_stats_reset(void);

// Forward declarations for helper render functions
void render_reset_countdown(uint32_t elapsed_ms);
void render_reset_stats_menu(void);

// =============================================================================
// Rendering Function Implementations
// =============================================================================
void render_startup(void)
{
    display_clear();

    // Display "DIN" in large text, centered
    display_large_text(28, 8, "DIN");

    // Display "fabrik" in normal text below, centered
    display_text(0, 4, "      fabrik");

    // Display "initialising..." at bottom (line 7 - last line for 64px display)
    display_text(0, 7, "  initialising...");

    display_flush();
}

void render_main_menu(void)
{
    display_menu(main_menu_items, MENU_COUNT, menu_selected_item);
}

void render_job_setup(void)
{
    // Validation: Check for NULL pointer
    if (current_run == NULL)
    {
        ESP_LOGE(TAG, "render_job_setup: NULL current_run pointer");
        display_clear();
        display_text(0, 0, "Error: No run data");
        display_flush();
        return;
    }

    char line[21];

    display_clear();

    for (uint8_t i = 0; i < JOB_SETUP_ITEM_COUNT; i++)
    {
        bool is_selected = (i == job_setup_selected_index);
        bool is_editing = is_selected && job_setup_edit_mode;

        // Bounds check using constant
        if (i >= JOB_SETUP_ITEM_COUNT)
        {
            ESP_LOGW(TAG, "render_job_setup: i=%d out of bounds", i);
            break;
        }

        if (i == JOB_ITEM_NUM_SHIRTS)
        {
            int value = is_editing ? job_setup_staged_num_shirts : current_run->num_shirts;
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-9s  [%3d]", job_setup_items[i], value);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-9s   %3d ", job_setup_items[i], value);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-9s   %3d ", job_setup_items[i], value);
            }
        }
        else // JOB_ITEM_PRINT_TYPE
        {
            printing_type_t type = is_editing ? job_setup_staged_print_type : current_run->type;
            const char *type_str = (type == SINGLE_SIDED) ? "SS" : "DS";
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-9s  [%3s]", job_setup_items[i], type_str);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-9s   %3s ", job_setup_items[i], type_str);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-9s   %3s ", job_setup_items[i], type_str);
            }
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

void render_job_setup_adjust(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Adjust:");
    display_text(0, 1, job_setup_items[job_setup_selected_index]);

    if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
    {
        sprintf(buffer, ">> %d <<", current_run->num_shirts);
        display_text(0, 2, buffer);
    }
    display_flush();
}

void render_print_type_select(void)
{
    display_menu(print_type_items, 2, print_type_selected_index);
}

void render_settings_menu(void)
{
    display_menu(settings_menu_items, SETTINGS_COUNT, settings_selected_item);
}

void render_timers_menu(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < TIMER_COUNT; i++)
    {
        bool is_selected = (i == timer_selected_index);
        bool is_editing = is_selected && timer_edit_mode;

        int value;
        if (is_editing)
        {
            value = timer_staged_value;
        }
        else
        {
            value = (i == TIMER_STAGE1) ? current_settings->stage1_default : current_settings->stage2_default;
        }

        if (is_editing)
        {
            snprintf(line, sizeof(line), "> %-8s  [%3ds]", timer_menu_items[i], value);
        }
        else if (is_selected)
        {
            snprintf(line, sizeof(line), "> %-8s   %3ds ", timer_menu_items[i], value);
        }
        else
        {
            snprintf(line, sizeof(line), "  %-8s   %3ds ", timer_menu_items[i], value);
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

void render_timer_adjust(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Adjust:");
    display_text(0, 1, timer_menu_items[timer_selected_index]);

    if (timer_selected_index == TIMER_STAGE1)
    {
        sprintf(buffer, ">> %d s <<", current_settings->stage1_default);
    }
    else
    {
        sprintf(buffer, ">> %d s <<", current_settings->stage2_default);
    }
    display_text(0, 2, buffer);
    display_flush();
}

void render_temperature_menu(void)
{
    // Validation: Check for NULL pointer
    if (current_settings == NULL)
    {
        ESP_LOGE(TAG, "render_temperature_menu: NULL current_settings pointer");
        display_clear();
        display_text(0, 0, "Error: No settings");
        display_flush();
        return;
    }

    char line[21];

    display_clear();

    for (uint8_t i = 0; i < TEMP_COUNT; i++)
    {
        bool is_selected = (i == temp_selected_index);
        bool is_editing = is_selected && temp_edit_mode;

        // Bounds check using constant (TEMP_COUNT from ui_state.h)
        if (i >= TEMP_COUNT)
        {
            ESP_LOGW(TAG, "render_temperature_menu: i=%d out of bounds", i);
            break;
        }

        if (i == TEMP_TARGET_TEMP)
        {
            int temp_int = is_editing ? (int)temp_staged_value : (int)current_settings->target_temp;
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-9s  [%3d]", temp_menu_items[i], temp_int);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-9s   %3d ", temp_menu_items[i], temp_int);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-9s   %3d ", temp_menu_items[i], temp_int);
            }
        }
        else // TEMP_PID_CONTROL
        {
            if (is_selected)
            {
                snprintf(line, sizeof(line), "> %s", temp_menu_items[i]);
            }
            else
            {
                snprintf(line, sizeof(line), "  %s", temp_menu_items[i]);
            }
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

void render_temp_adjust(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Adjust:");
    display_text(0, 1, "Target Temp");
    sprintf(buffer, ">> %.1f C <<", current_settings->target_temp);
    display_text(0, 2, buffer);
    display_flush();
}

void render_pid_menu(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < PID_COUNT; i++)
    {
        bool is_selected = (i == pid_selected_index);
        bool is_editing = is_selected && pid_edit_mode;

        if (i == PID_AUTOTUNE)
        {
            // Auto-Tune doesn't have a value to display or edit
            if (is_selected)
            {
                snprintf(line, sizeof(line), "> %s", pid_menu_items[i]);
            }
            else
            {
                snprintf(line, sizeof(line), "  %s", pid_menu_items[i]);
            }
        }
        else if (i == PID_KP)
        {
            float value = is_editing ? pid_staged_value : current_settings->pid_kp;
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-3s  [%5.2f]", pid_menu_items[i], value);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-3s   %5.2f ", pid_menu_items[i], value);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-3s   %5.2f ", pid_menu_items[i], value);
            }
        }
        else if (i == PID_KI)
        {
            float value = is_editing ? pid_staged_value : current_settings->pid_ki;
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-3s  [%5.3f]", pid_menu_items[i], value);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-3s   %5.3f ", pid_menu_items[i], value);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-3s   %5.3f ", pid_menu_items[i], value);
            }
        }
        else // PID_KD
        {
            float value = is_editing ? pid_staged_value : current_settings->pid_kd;
            if (is_editing)
            {
                snprintf(line, sizeof(line), "> %-3s  [%5.2f]", pid_menu_items[i], value);
            }
            else if (is_selected)
            {
                snprintf(line, sizeof(line), "> %-3s   %5.2f ", pid_menu_items[i], value);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-3s   %5.2f ", pid_menu_items[i], value);
            }
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

void render_pid_adjust(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Adjust:");
    display_text(0, 1, pid_menu_items[pid_selected_index]);

    if (pid_selected_index == PID_KP)
    {
        sprintf(buffer, ">> %.2f <<", current_settings->pid_kp);
    }
    else if (pid_selected_index == PID_KI)
    {
        sprintf(buffer, ">> %.3f <<", current_settings->pid_ki);
    }
    else if (pid_selected_index == PID_KD)
    {
        sprintf(buffer, ">> %.2f <<", current_settings->pid_kd);
    }
    display_text(0, 2, buffer);
    display_flush();
}

void render_start_pressing(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Ready to Press");
    sprintf(buffer, "Temp: %.1f/%.1f C",
            temperature_display_celsius,
            current_settings->target_temp);
    display_text(0, 1, buffer);
    display_text(0, 2, "Close press to start");
    display_flush();
}

void render_free_press(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Free Press Mode");
    sprintf(buffer, "Temp: %.1f/%.1f C",
            temperature_display_celsius,
            current_settings->target_temp);
    display_text(0, 1, buffer);
    sprintf(buffer, "Pressed: %d", free_press_count);
    display_text(0, 2, buffer);
    display_text(0, 3, "Close press to start");
    display_flush();
}

void render_profiles_menu(void)
{
    display_menu(profile_items, PROFILE_COUNT, profile_selected_index);
}

void render_pressing_active(void)
{
    extern pressing_cycle_t current_cycle;
    extern uint32_t stage_start_time;
    extern cycle_status_t current_stage;

    static uint32_t last_time_remaining = 9999;
    static cycle_status_t last_stage = IDLE;
    static bool screen_initialized = false;
    static float last_displayed_temp = 0.0f;

    uint32_t current_time = esp_timer_get_time() / 1000000;  // Convert to seconds
    uint32_t stage_elapsed = current_time - stage_start_time;
    uint32_t stage_duration = (current_stage == STAGE1) ?
                               current_cycle.stage1_duration :
                               current_cycle.stage2_duration;
    uint32_t time_remaining = (stage_elapsed < stage_duration) ?
                               (stage_duration - stage_elapsed) : 0;

    // Full redraw when stage changes or waiting between stages
    if (current_stage != last_stage || !screen_initialized)
    {
        display_clear();

        if (current_stage == IDLE)
        {
            // Waiting between Stage 1 and Stage 2
            display_text(0, 0, "Stage 1 Done!");
            display_text(0, 1, "Open press, then");
            display_text(0, 2, "close for Stage 2");
            display_flush();
            screen_initialized = true;
            last_stage = current_stage;
            return;  // Don't show countdown when waiting
        }

        // Build complete top line with stage and shirt number
        char top_line[22];  // 21 chars + null terminator
        char shirt_buffer[10];

        if (free_press_mode)
        {
            sprintf(shirt_buffer, "# %d", free_press_count + 1);
        }
        else
        {
            sprintf(shirt_buffer, "# %d", current_cycle.shirt_id);
        }

        // Format: "Stage 1         # 125" (stage left-aligned, shirt number right-aligned)
        const char *stage_text = (current_stage == STAGE1) ? "Stage 1" : "Stage 2";
        int padding = 21 - strlen(stage_text) - strlen(shirt_buffer);
        if (padding < 1) padding = 1;

        sprintf(top_line, "%s%*s", stage_text, padding + (int)strlen(shirt_buffer), shirt_buffer);
        display_text(0, 0, top_line);

        // Display initial temperature on line 1
        char temp_buffer[22];
        sprintf(temp_buffer, "%.1fC / %.1fC", temperature_display_celsius, current_settings->target_temp);
        display_text(0, 1, temp_buffer);
        last_displayed_temp = temperature_display_celsius;

        screen_initialized = true;
        last_stage = current_stage;
        last_time_remaining = 9999; // Force update
    }

    // Update temperature if it changed by 0.5°C or more (without full redraw to avoid flicker)
    if (current_stage != IDLE && fabsf(temperature_display_celsius - last_displayed_temp) >= 0.5f)
    {
        char temp_buffer[22];
        sprintf(temp_buffer, "%.1fC / %.1fC", temperature_display_celsius, current_settings->target_temp);
        display_text(0, 1, temp_buffer);
        display_flush();
        last_displayed_temp = temperature_display_celsius;
    }

    // Update countdown and progress bar when time changes
    if (time_remaining != last_time_remaining && current_stage != IDLE)
    {
        // Clear the countdown area (center of screen for numbers and message area)
        for (uint8_t i = 30; i < 100; i++) {
            for (uint8_t j = 15; j < 48; j++) {
                display_set_pixel(i, j, false);  // Clear pixels
            }
        }

        // Show completion message when done or draw countdown
        if (time_remaining == 0 && current_stage == STAGE2)
        {
            display_text(0, 2, "Open to complete");
        }
        else
        {
            // Draw large countdown number in center
            display_large_number(40, 16, time_remaining > 99 ? 99 : (uint8_t)time_remaining);
        }

        // Calculate progress (inverted - starts at 100% and goes to 0%)
        uint8_t progress = (stage_duration > 0) ?
                          ((stage_duration - time_remaining) * 100) / stage_duration : 100;

        // Draw progress bar at bottom
        display_draw_progress_bar(10, 50, 108, 10, progress);

        display_flush();
        last_time_remaining = time_remaining;
    }
}

void render_statistics(void)
{
    // Show statistics submenu
    display_menu(stats_menu_items, STATS_COUNT, stats_selected_index);
}

void render_stats_production(void)
{
    char buffer[32];
    display_clear();

    display_text(0, 0, "== Production ==");

    // Get statistics via callback
    const statistics_t *stats = (ui_callbacks.get_statistics != NULL) ?
                                ui_callbacks.get_statistics() : NULL;
    if (stats == NULL)
    {
        display_text(0, 2, "No stats available");
        display_flush();
        return;
    }

    // Total presses
    sprintf(buffer, "Total: %lu", stats->total_presses);
    display_text(0, 1, buffer);

    // Calculate current session time
    extern uint32_t system_start_time;
    uint32_t current_time = esp_timer_get_time() / 1000000;
    uint32_t session_time = (system_start_time > 0) ? (current_time - system_start_time) : 0;

    // Operating time (use session time if total_operating_time not tracked)
    uint32_t op_time = (stats->total_operating_time > 0) ? stats->total_operating_time : session_time;
    uint32_t hours = op_time / 3600;
    uint32_t mins = (op_time % 3600) / 60;
    sprintf(buffer, "Time: %luh %lum", hours, mins);
    display_text(0, 2, buffer);

    // Idle time ratio
    uint32_t idle_pct = 0;
    if (op_time > 0 && stats->total_idle_time > 0)
    {
        idle_pct = (stats->total_idle_time * 100) / op_time;
    }
    sprintf(buffer, "Idle: %lu%%", idle_pct);
    display_text(0, 3, buffer);

    display_flush();
}

void render_stats_temperature(void)
{
    char buffer[32];
    display_clear();

    display_text(0, 0, "== Temperature ==");

    // Current vs target
    sprintf(buffer, "Now: %.1f/%.1fC",
            temperature_display_celsius,
            current_settings->target_temp);
    display_text(0, 1, buffer);

    // Get statistics via callback
    const statistics_t *stats = (ui_callbacks.get_statistics != NULL) ?
                                ui_callbacks.get_statistics() : NULL;

    // Average warmup time (show even if zero)
    uint32_t warmup = 0;
    if (ui_callbacks.get_warmup_time != NULL)
    {
        warmup = ui_callbacks.get_warmup_time();
    }
    else if (stats != NULL && stats->warmup_count > 0)
    {
        warmup = (uint32_t)stats->avg_warmup_time;
    }
    sprintf(buffer, "Warmup: %lus", warmup);
    display_text(0, 2, buffer);

    // Presses since PID tune
    uint32_t presses_since_tune = (stats != NULL) ? stats->presses_since_pid_tune : 0;
    sprintf(buffer, "Since tune: %lu", presses_since_tune);
    display_text(0, 3, buffer);

    display_flush();
}

void render_stats_events(void)
{
    char buffer[32];
    display_clear();

    display_text(0, 0, "=== Events ===");

    // Get statistics via callback
    const statistics_t *stats = (ui_callbacks.get_statistics != NULL) ?
                                ui_callbacks.get_statistics() : NULL;
    if (stats == NULL)
    {
        display_text(0, 2, "No stats available");
        display_flush();
        return;
    }

    // Aborted cycles
    sprintf(buffer, "Aborted: %u", stats->aborted_cycles);
    display_text(0, 1, buffer);

    // Errors
    sprintf(buffer, "Errors: %u",
            stats->temp_faults + stats->sensor_failures);
    display_text(0, 2, buffer);

    // Emergency stops
    sprintf(buffer, "E-stops: %u", stats->emergency_stops);
    display_text(0, 3, buffer);

    display_flush();
}

void render_stats_kpis(void)
{
    char buffer[32];
    display_clear();

    display_text(0, 0, "===== KPIs =====");

    // Get statistics via callback
    const statistics_t *stats = (ui_callbacks.get_statistics != NULL) ?
                                ui_callbacks.get_statistics() : NULL;
    if (stats == NULL)
    {
        display_text(0, 2, "No stats available");
        display_flush();
        return;
    }

    // Calculate session time for KPIs
    extern uint32_t system_start_time;
    uint32_t current_time = esp_timer_get_time() / 1000000;
    uint32_t session_time = (system_start_time > 0) ? (current_time - system_start_time) : 0;
    uint32_t op_time = (stats->total_operating_time > 0) ? stats->total_operating_time : session_time;

    // Presses per hour
    uint32_t pph = 0;
    if (op_time > 0)
    {
        pph = (stats->total_presses * 3600) / op_time;
    }
    sprintf(buffer, "Press/hr: %lu", pph);
    display_text(0, 1, buffer);

    // Idle ratio
    uint32_t idle_ratio = 0;
    if (op_time > 0 && stats->total_idle_time > 0)
    {
        idle_ratio = (stats->total_idle_time * 100) / op_time;
    }
    sprintf(buffer, "Idle: %lu%%", idle_ratio);
    display_text(0, 2, buffer);

    // Temperature stability (presses in tolerance)
    uint32_t stability = 0;
    if (stats->total_presses > 0)
    {
        stability = (stats->presses_in_tolerance * 100) / stats->total_presses;
    }
    sprintf(buffer, "Temp OK: %lu%%", stability);
    display_text(0, 3, buffer);

    display_flush();
}

// =============================================================================
// Auto-Tune State Handlers (NEW)
// =============================================================================

void handle_autotune_state(ui_event_t event)
{
    extern void cancel_pid_autotune(void);

    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        // Cancel auto-tune and return to settings
        cancel_pid_autotune();
        ui_current_state = UI_STATE_SETTINGS_MENU;
        ESP_LOGI(TAG, "Auto-tune cancelled by user");
        break;

    default:
        // Check if auto-tune completed (handled by main.c)
        // State will transition to UI_STATE_AUTOTUNE_COMPLETE automatically
        break;
    }
}

void handle_autotune_complete_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_PUSH:
    case UI_EVENT_BUTTON_BACK:
        // Return to PID menu
        ui_current_state = UI_STATE_PID_MENU;
        break;

    default:
        break;
    }
}

void render_autotune(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Auto-Tuning PID");

    // Show progress
    uint8_t progress = (ui_callbacks.get_autotune_progress != NULL) ?
                       ui_callbacks.get_autotune_progress() : 0;
    sprintf(buffer, "Progress: %d%%", progress);
    display_text(0, 1, buffer);

    // Show current temperature
    sprintf(buffer, "Temp: %.1f C", temperature_display_celsius);
    display_text(0, 2, buffer);

    // Show cancel instruction
    display_text(0, 3, "BACK to cancel");
    display_flush();
}

void render_autotune_complete(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Auto-Tune Done!");

    // Show new PID parameters
    sprintf(buffer, "Kp:%.2f Ki:%.3f",
            current_settings->pid_kp,
            current_settings->pid_ki);
    display_text(0, 1, buffer);

    sprintf(buffer, "Kd:%.3f", current_settings->pid_kd);
    display_text(0, 2, buffer);

    display_text(0, 3, "Press any button");
    display_flush();
}

void handle_reset_stats_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_CW:
        if (!reset_stats_button_pressed)
        {
            reset_stats_selected_index = (reset_stats_selected_index + 1) % 2;
            ESP_LOGI(TAG, "Reset stats option: %d", reset_stats_selected_index);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (!reset_stats_button_pressed)
        {
            reset_stats_selected_index = (reset_stats_selected_index - 1 + 2) % 2;
            ESP_LOGI(TAG, "Reset stats option: %d", reset_stats_selected_index);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        // Button pressed - start tracking
        if (!reset_stats_button_pressed)
        {
            reset_stats_button_pressed = true;
            reset_stats_press_start_time = esp_timer_get_time() / 1000; // Convert to milliseconds
            ESP_LOGI(TAG, "Reset stats button pressed for option %d", reset_stats_selected_index);
        }
        break;

    case UI_EVENT_BUTTON_BACK:
        // Cancel and return to settings menu
        reset_stats_button_pressed = false;
        reset_stats_press_start_time = 0;
        reset_stats_selected_index = 0;
        ui_current_state = UI_STATE_SETTINGS_MENU;
        ESP_LOGI(TAG, "Reset stats cancelled, returning to settings menu");
        break;

    default:
        break;
    }
}

// Statistics reset helpers moved to ui_helpers.c

// Helper: Render countdown confirmation screen
void render_reset_countdown(uint32_t elapsed_ms)
{
    static uint32_t last_countdown_sec = 999;
    static bool wait_message_shown = false;
    char buffer[64];

    if (elapsed_ms < 1000)
    {
        // Still in the 1-second wait period - only show message once
        if (!wait_message_shown)
        {
            display_clear();
            const char* option_text = (reset_stats_selected_index == 0) ? "Job Stats" : "ALL STATS";
            sprintf(buffer, "Wiping %s", option_text);
            display_text(0, 0, buffer);
            display_text(0, 1, "");
            display_text(0, 2, "Hold to confirm...");
            display_text(0, 3, "");
            display_flush();
            wait_message_shown = true;
            last_countdown_sec = 999; // Reset for next countdown
        }
    }
    else
    {
        // In countdown period (1000ms to 4000ms)
        wait_message_shown = false; // Reset for next time
        uint32_t countdown_ms = 4000 - elapsed_ms;
        uint32_t countdown_sec = (countdown_ms + 999) / 1000; // Round up

        // Only redraw if the second changed
        if (countdown_sec != last_countdown_sec)
        {
            display_clear();
            display_text(0, 0, "Wiping in:");
            sprintf(buffer, "%lu", countdown_sec);
            display_large_text(52, 20, buffer);
            display_flush();
            last_countdown_sec = countdown_sec;
        }
    }
}

// Helper: Render reset stats menu
void render_reset_stats_menu(void)
{
    display_clear();
    display_text(0, 0, "Reset Statistics");

    if (reset_stats_selected_index == 0)
    {
        display_text(0, 1, "> Wipe Job Stats");
        display_text(0, 2, "  Wipe All Stats");
    }
    else
    {
        display_text(0, 1, "  Wipe Job Stats");
        display_text(0, 2, "> Wipe All Stats");
    }

    display_text(0, 3, "Hold to wipe");
    display_flush();
}

void render_reset_stats(void)
{
    if (reset_stats_button_pressed)
    {
        bool button_still_pressed = controls_is_rotary_button_pressed();
        uint32_t current_time = esp_timer_get_time() / 1000;
        uint32_t elapsed_ms = current_time - reset_stats_press_start_time;

        ESP_LOGI(TAG, "Render reset stats: pressed=%d, elapsed=%lu ms", button_still_pressed, elapsed_ms);

        // Check if we've reached the trigger time (4 seconds)
        if (elapsed_ms >= 4000 && button_still_pressed && reset_stats_button_pressed)
        {
            // Perform the reset
            if (reset_stats_selected_index == 0)
            {
                perform_job_stats_reset();
            }
            else
            {
                perform_all_stats_reset();
            }

            // Cleanup and return to settings
            reset_stats_button_pressed = false;
            reset_stats_press_start_time = 0;
            reset_stats_selected_index = 0;
            vTaskDelay(pdMS_TO_TICKS(1500));
            ui_current_state = UI_STATE_SETTINGS_MENU;
            return;
        }

        // Button still pressed - show countdown
        render_reset_countdown(elapsed_ms);
    }
    else
    {
        // Show menu
        render_reset_stats_menu();
    }
}

// New state render functions
void render_stage1_done(void)
{
    display_clear();
    display_invert(true);
    display_large_text(20, 16, "DONE");
    display_flush();
}

void render_stage2_ready(void)
{
    display_clear();
    display_invert(false);
    display_large_text(10, 16, "READY");
    display_flush();
}

void render_stage2_done(void)
{
    display_clear();
    display_invert(true);
    display_large_text(20, 16, "DONE");
    display_flush();
}

void render_cycle_complete(void)
{
    extern print_run_t print_run;
    char buffer[32];

    display_clear();
    display_invert(false);

    if (free_press_mode)
    {
        display_text(0, 0, "Press Complete!");
        sprintf(buffer, "Count: %d", free_press_count);
        display_text(0, 1, buffer);
        if (free_press_avg_time > 0)
        {
            sprintf(buffer, "Avg: %lu s", free_press_avg_time);
            display_text(0, 2, buffer);
        }
    }
    else
    {
        display_text(0, 0, "Cycle Complete!");
        sprintf(buffer, "Done: %d / %d",
                print_run.shirts_completed,
                print_run.num_shirts);
        display_text(0, 1, buffer);
        if (print_run.avg_time_per_shirt > 0)
        {
            sprintf(buffer, "Avg: %lu s", print_run.avg_time_per_shirt);
            display_text(0, 2, buffer);
        }
    }

    display_text(0, 3, "Close for next");
    display_flush();
}

// =============================================================================
// Heat Up Mode (NEW)
// =============================================================================

void handle_heat_up_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        // Always allow back to main menu
        ui_current_state = UI_STATE_MAIN_MENU;
        heat_up_return_state = UI_STATE_MAIN_MENU; // Reset return state
        ESP_LOGI(TAG, "Heat up mode cancelled - returning to main menu");
        break;

    default:
        // Auto-transition when heat press becomes ready
        if (is_heat_press_ready())
        {
            // Heat press is now ready - transition to the appropriate state
            if (heat_up_return_state == UI_STATE_START_PRESSING)
            {
                ESP_LOGI(TAG, "Heat press ready - transitioning to Job Press");
                ui_current_state = UI_STATE_START_PRESSING;
                init_job_press_mode();
                display_needs_update = true;
            }
            else if (heat_up_return_state == UI_STATE_FREE_PRESS)
            {
                ESP_LOGI(TAG, "Heat press ready - transitioning to Free Press");
                ui_current_state = UI_STATE_FREE_PRESS;
                init_free_press_mode();
                display_needs_update = true;
            }
            else if (heat_up_return_state == UI_STATE_HEAT_UP)
            {
                // User explicitly selected Heat Up - stay on this screen
                // They can press back to exit when ready
            }
        }
        break;
    }
}

void render_heat_up(void)
{
    char buffer[32];
    bool heating_active = heating_is_active();
    uint32_t current_time = esp_timer_get_time() / 1000000;
    uint32_t elapsed_sec = current_time - heat_up_start_time;

    // Check if heating is enabled
    if (!heating_active)
    {
        // Only redraw if heating state changed to avoid flicker
        if (heat_up_heating_was_active || !heat_up_screen_initialized)
        {
            display_clear();
            display_text(0, 0, "Heating Disabled!");
            display_text(0, 1, "");
            display_text(0, 2, "Please connect");
            display_text(0, 3, "heating switch");
            display_flush();
            heat_up_heating_was_active = false;
            heat_up_screen_initialized = true;
        }
        return;
    }

    // Full redraw only when heating state changes or entering for first time
    if (heating_active != heat_up_heating_was_active || !heat_up_screen_initialized)
    {
        display_clear();
        display_text(0, 0, "Heating Up...");
        display_flush();
        heat_up_screen_initialized = true;
        heat_up_heating_was_active = true;
        heat_up_last_update_sec = 0; // Reset to force immediate update
    }

    // Partial update when second changes (only redraw changing values)
    if (elapsed_sec != heat_up_last_update_sec)
    {
        // Show current / target temp
        sprintf(buffer, "%.1f / %.1fC",
                temperature_display_celsius,
                current_settings->target_temp);
        display_text(0, 1, buffer);

        // Show elapsed time
        uint32_t elapsed_min = elapsed_sec / 60;
        uint32_t elapsed_sec_remainder = elapsed_sec % 60;
        sprintf(buffer, "Time: %lum %lus", elapsed_min, elapsed_sec_remainder);
        display_text(0, 2, buffer);

        // Calculate ETA based on heating rate
        float temp_diff = temperature_display_celsius - heat_up_start_temp;
        float temp_remaining = current_settings->target_temp - temperature_display_celsius;

        if (temp_diff > HEAT_UP_MIN_TEMP_CHANGE && elapsed_sec > HEAT_UP_MIN_ELAPSED_TIME)
        {
            // Calculate heating rate (degrees per second)
            float heating_rate = temp_diff / elapsed_sec;

            if (heating_rate > HEAT_UP_MIN_HEATING_RATE)
            {
                uint32_t eta_sec = (uint32_t)(temp_remaining / heating_rate);
                uint32_t eta_min = eta_sec / 60;
                uint32_t eta_sec_remainder = eta_sec % 60;

                if (temp_remaining > HEAT_UP_TEMP_READY_THRESHOLD)
                {
                    sprintf(buffer, "ETA: %lum %lus       ", eta_min, eta_sec_remainder);
                }
                else
                {
                    sprintf(buffer, "ETA: Ready!         ");
                }
                display_text(0, 3, buffer);
            }
            else
            {
                display_text(0, 3, "ETA: Calculating... ");
            }
        }
        else
        {
            display_text(0, 3, "ETA: Calculating... ");
        }

        display_flush();
        heat_up_last_update_sec = elapsed_sec;
    }
}
