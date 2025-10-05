/**
 * @file ui_state.c
 * @brief User interface state machine implementation (Refactored)
 *
 * This module implements a finite state machine for the user interface
 * using a state handler pattern for better maintainability and clarity.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "ui_state.h"
#include "controls_contract.h"
#include "display_contract.h"
#include "data_model.h"
#include "system_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_err.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "ui_state";

// =============================================================================
// State Variables
// =============================================================================

// Current state
static ui_state_t ui_current_state = UI_STATE_INIT;
static ui_state_t ui_previous_state = UI_STATE_INIT;
static menu_item_t menu_selected_item = MENU_JOB_SETUP;
static settings_item_t settings_selected_item = SETTINGS_TIMERS;
static bool display_needs_update = true;

// Menu items
static const char *main_menu_items[] = {
    "Job Setup",
    "Start Pressing",
    "Settings",
    "Statistics"
};

static const char *settings_menu_items[] = {
    "Timers",
    "Temperature"
};

static const char *timer_menu_items[] = {
    "Stage 1",
    "Stage 2"
};

static const char *temp_menu_items[] = {
    "Target Temp",
    "PID Control"
};

static const char *pid_menu_items[] = {
    "Auto-Tune",
    "Kp",
    "Ki",
    "Kd"
};

static const char *job_setup_items[] = {
    "Num Shirts",
    "Print Type"
};

static const char *print_type_items[] = {
    "Single Sided",
    "Double Sided"
};

// Current values for adjustment
static int ui_adjustment_value = 0;
static int ui_adjustment_min = 0;
static int ui_adjustment_max = 100;

// Settings and print run references
static settings_t *current_settings = NULL;
static print_run_t *current_run = NULL;

// Current temperature for display
static float temperature_display_celsius = 0.0f;

// Job setup state
static int job_setup_selected_index = 0;
static int print_type_selected_index = 0;

// Settings submenu state
static int timer_selected_index = 0;
static int temp_selected_index = 0;
static int pid_selected_index = 0;

// Press state tracking
static bool was_press_closed_ui = false;

// External functions from main.c
extern bool start_pid_autotune(float target_temp);
extern bool is_pid_autotuning(void);
extern uint8_t get_autotune_progress(void);

// =============================================================================
// State Handler Function Prototypes
// =============================================================================

static void handle_main_menu_state(ui_event_t event);
static void handle_job_setup_state(ui_event_t event);
static void handle_job_setup_adjust_state(ui_event_t event);
static void handle_print_type_select_state(ui_event_t event);
static void handle_settings_menu_state(ui_event_t event);
static void handle_timers_menu_state(ui_event_t event);
static void handle_timer_adjust_state(ui_event_t event);
static void handle_temperature_menu_state(ui_event_t event);
static void handle_temp_adjust_state(ui_event_t event);
static void handle_pid_menu_state(ui_event_t event);
static void handle_pid_adjust_state(ui_event_t event);
static void handle_start_pressing_state(ui_event_t event);
static void handle_pressing_active_state(ui_event_t event);
static void handle_statistics_state(ui_event_t event);
static void handle_autotune_state(ui_event_t event);           // NEW
static void handle_autotune_complete_state(ui_event_t event);  // NEW

// Display rendering functions
static void render_main_menu(void);
static void render_job_setup(void);
static void render_job_setup_adjust(void);
static void render_print_type_select(void);
static void render_settings_menu(void);
static void render_timers_menu(void);
static void render_timer_adjust(void);
static void render_temperature_menu(void);
static void render_temp_adjust(void);
static void render_pid_menu(void);
static void render_pid_adjust(void);
static void render_start_pressing(void);
static void render_pressing_active(void);
static void render_statistics(void);
static void render_autotune(void);           // NEW
static void render_autotune_complete(void);  // NEW
static void render_stage1_done(void);        // NEW
static void render_stage2_ready(void);       // NEW
static void render_stage2_done(void);        // NEW
static void render_cycle_complete(void);     // NEW

// =============================================================================
// State Handler Table
// =============================================================================

typedef struct
{
    ui_state_t state;
    void (*handler)(ui_event_t event);
    void (*renderer)(void);
    const char *state_name;
} state_handler_entry_t;

static const state_handler_entry_t state_handlers[] = {
    {UI_STATE_MAIN_MENU, handle_main_menu_state, render_main_menu, "Main Menu"},
    {UI_STATE_JOB_SETUP, handle_job_setup_state, render_job_setup, "Job Setup"},
    {UI_STATE_JOB_SETUP_ADJUST, handle_job_setup_adjust_state, render_job_setup_adjust, "Adjust Job Setup"},
    {UI_STATE_PRINT_TYPE_SELECT, handle_print_type_select_state, render_print_type_select, "Select Print Type"},
    {UI_STATE_SETTINGS_MENU, handle_settings_menu_state, render_settings_menu, "Settings"},
    {UI_STATE_TIMERS_MENU, handle_timers_menu_state, render_timers_menu, "Timers"},
    {UI_STATE_TIMER_ADJUST, handle_timer_adjust_state, render_timer_adjust, "Adjust Timer"},
    {UI_STATE_TEMPERATURE_MENU, handle_temperature_menu_state, render_temperature_menu, "Temperature"},
    {UI_STATE_TEMP_ADJUST, handle_temp_adjust_state, render_temp_adjust, "Adjust Temperature"},
    {UI_STATE_PID_MENU, handle_pid_menu_state, render_pid_menu, "PID Control"},
    {UI_STATE_PID_ADJUST, handle_pid_adjust_state, render_pid_adjust, "Adjust PID"},
    {UI_STATE_START_PRESSING, handle_start_pressing_state, render_start_pressing, "Start Pressing"},
    {UI_STATE_PRESSING_ACTIVE, handle_pressing_active_state, render_pressing_active, "Pressing Active"},
    {UI_STATE_STAGE1_DONE, NULL, render_stage1_done, "Stage 1 Done"},
    {UI_STATE_STAGE2_READY, NULL, render_stage2_ready, "Stage 2 Ready"},
    {UI_STATE_STAGE2_DONE, NULL, render_stage2_done, "Stage 2 Done"},
    {UI_STATE_CYCLE_COMPLETE, NULL, render_cycle_complete, "Cycle Complete"},
    {UI_STATE_STATISTICS, handle_statistics_state, render_statistics, "Statistics"},
    {UI_STATE_AUTOTUNE, handle_autotune_state, render_autotune, "Auto-Tune"},
    {UI_STATE_AUTOTUNE_COMPLETE, handle_autotune_complete_state, render_autotune_complete, "Results"},
};

// =============================================================================
// Public Interface Implementation
// =============================================================================

void ui_init(settings_t *settings, print_run_t *print_run)
{
    current_settings = settings;
    current_run = print_run;
    ui_current_state = UI_STATE_MAIN_MENU;
    ESP_LOGI(TAG, "UI state machine initialized");
}

void ui_update(float current_temp)
{
    temperature_display_celsius = current_temp;
    ui_event_t event = ui_get_event();
    if (event != UI_EVENT_NONE)
    {
        ui_process_event(event);
        display_needs_update = true;  // Mark display for update after event
    }

    // Check if state changed
    if (ui_current_state != ui_previous_state)
    {
        display_needs_update = true;
        ui_previous_state = ui_current_state;
    }

    // Always update display during pressing to show countdown
    if (ui_current_state == UI_STATE_PRESSING_ACTIVE)
    {
        display_needs_update = true;
    }

    // Only update display when needed
    if (display_needs_update)
    {
        ui_update_display();
        display_needs_update = false;
    }
}

ui_event_t ui_get_event(void)
{
    button_event_t button = controls_get_button_event();
    rotary_event_t rotary = controls_get_rotary_event();
    bool is_press_closed = controls_is_press_closed();

    // Debug: log what we received
    if (button != BUTTON_NONE || rotary != ROTARY_NONE) {
        ESP_LOGI(TAG, "Events received - button: %d, rotary: %d", button, rotary);
    }

    // Button events (check these FIRST - they're more important than rotary)
    if (button == BUTTON_SAVE) {
        ESP_LOGI(TAG, "UI Event: BUTTON_SAVE");
        return UI_EVENT_BUTTON_SAVE;
    }
    if (button == BUTTON_BACK) {
        ESP_LOGI(TAG, "UI Event: BUTTON_BACK");
        return UI_EVENT_BUTTON_BACK;
    }

    // Rotary events
    if (rotary == ROTARY_PUSH) {
        ESP_LOGI(TAG, "UI Event: ROTARY_PUSH");
        return UI_EVENT_ROTARY_PUSH;
    }
    if (rotary == ROTARY_CW)
        return UI_EVENT_ROTARY_CW;
    if (rotary == ROTARY_CCW)
        return UI_EVENT_ROTARY_CCW;

    // Press state changes
    if (is_press_closed && !was_press_closed_ui)
    {
        was_press_closed_ui = true;
        return UI_EVENT_PRESS_CLOSED;
    }
    if (!is_press_closed && was_press_closed_ui)
    {
        was_press_closed_ui = false;
        return UI_EVENT_PRESS_OPENED;
    }

    return UI_EVENT_NONE;
}

void ui_process_event(ui_event_t event)
{
    // Find and execute handler for current state
    for (size_t i = 0; i < ARRAY_SIZE(state_handlers); i++)
    {
        if (state_handlers[i].state == ui_current_state)
        {
            if (state_handlers[i].handler != NULL) {
                state_handlers[i].handler(event);
            }
            return;
        }
    }

    ESP_LOGW(TAG, "No handler found for state %d", ui_current_state);
}

void ui_update_display(void)
{
    // Find and execute renderer for current state
    for (size_t i = 0; i < ARRAY_SIZE(state_handlers); i++)
    {
        if (state_handlers[i].state == ui_current_state)
        {
            state_handlers[i].renderer();
            return;
        }
    }

    // Default display if no renderer found
    display_clear();
    display_text(0, 0, "Insta Retrofit");
    char buffer[32];
    sprintf(buffer, "Temp: %.1f C", temperature_display_celsius);
    display_text(0, 1, buffer);
    display_flush();
}

ui_state_t ui_get_current_state(void)
{
    return ui_current_state;
}

void ui_set_state(ui_state_t state)
{
    ESP_LOGI(TAG, "State transition: %d -> %d", ui_current_state, state);
    ui_current_state = state;
}

void ui_select_menu_item(menu_item_t item)
{
    menu_selected_item = item;
}

menu_item_t ui_get_selected_item(void)
{
    return menu_selected_item;
}

void ui_adjust_value(int8_t delta)
{
    ui_adjustment_value = CLAMP(ui_adjustment_value + delta,
                                 ui_adjustment_min,
                                 ui_adjustment_max);
}

// =============================================================================
// State Handler Implementations
// =============================================================================

static void handle_main_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_CW:
        menu_selected_item = MENU_WRAP(menu_selected_item + 1, MENU_COUNT);
        ESP_LOGI(TAG, "Menu item selected: %d", menu_selected_item);
        break;

    case UI_EVENT_ROTARY_CCW:
        menu_selected_item = MENU_WRAP(menu_selected_item - 1, MENU_COUNT);
        ESP_LOGI(TAG, "Menu item selected: %d", menu_selected_item);
        break;

    case UI_EVENT_ROTARY_PUSH:
        ESP_LOGI(TAG, "Encoder push pressed, entering menu item: %d", menu_selected_item);
        switch (menu_selected_item)
        {
        case MENU_JOB_SETUP:
            ESP_LOGI(TAG, "Entering JOB_SETUP state");
            ui_current_state = UI_STATE_JOB_SETUP;
            job_setup_selected_index = 0;
            break;
        case MENU_START_PRESSING:
            ui_current_state = UI_STATE_START_PRESSING;
            break;
        case MENU_SETTINGS:
            ui_current_state = UI_STATE_SETTINGS_MENU;
            settings_selected_item = 0;
            break;
        case MENU_STATISTICS:
            ui_current_state = UI_STATE_STATISTICS;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}

static void handle_job_setup_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    case UI_EVENT_ROTARY_CW:
        job_setup_selected_index = MENU_WRAP(job_setup_selected_index + 1,
                                              JOB_SETUP_ITEM_COUNT);
        break;

    case UI_EVENT_ROTARY_CCW:
        job_setup_selected_index = MENU_WRAP(job_setup_selected_index - 1,
                                              JOB_SETUP_ITEM_COUNT);
        break;

    case UI_EVENT_ROTARY_PUSH:
        // Enter adjustment mode for selected item
        if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
        {
            ui_current_state = UI_STATE_JOB_SETUP_ADJUST;
            ESP_LOGI(TAG, "Entering adjustment mode for Num Shirts");
        }
        else if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
        {
            // Enter print type selection menu
            ui_current_state = UI_STATE_PRINT_TYPE_SELECT;
            print_type_selected_index = current_run->type;  // Initialize with current value
            ESP_LOGI(TAG, "Entering print type selection");
        }
        break;

    default:
        break;
    }
}

static void handle_job_setup_adjust_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        // Exit adjustment mode without saving (but changes already applied)
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Adjustment cancelled");
        break;

    case UI_EVENT_ROTARY_CW:
        // Adjust value up based on selected setting
        if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
        {
            current_run->num_shirts = CLAMP(current_run->num_shirts + 1,
                                             NUM_SHIRTS_MIN,
                                             NUM_SHIRTS_MAX);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        // Adjust value down based on selected setting
        if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
        {
            current_run->num_shirts = CLAMP(current_run->num_shirts - 1,
                                             NUM_SHIRTS_MIN,
                                             NUM_SHIRTS_MAX);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        // Exit adjustment mode and return to menu
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Job setup value confirmed");
        break;

    default:
        break;
    }
}

static void handle_print_type_select_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        // Exit without saving changes
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Print type selection cancelled");
        break;

    case UI_EVENT_ROTARY_CW:
        print_type_selected_index = MENU_WRAP(print_type_selected_index + 1, 2);
        break;

    case UI_EVENT_ROTARY_CCW:
        print_type_selected_index = MENU_WRAP(print_type_selected_index - 1, 2);
        break;

    case UI_EVENT_ROTARY_PUSH:
        // Save selection and return to job setup
        current_run->type = (printing_type_t)print_type_selected_index;
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Print type set to: %d", current_run->type);
        break;

    default:
        break;
    }
}

static void handle_settings_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    case UI_EVENT_ROTARY_CW:
        settings_selected_item = MENU_WRAP(settings_selected_item + 1, SETTINGS_COUNT);
        break;

    case UI_EVENT_ROTARY_CCW:
        settings_selected_item = MENU_WRAP(settings_selected_item - 1, SETTINGS_COUNT);
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (settings_selected_item == SETTINGS_TIMERS)
        {
            ui_current_state = UI_STATE_TIMERS_MENU;
            timer_selected_index = 0;
            ESP_LOGI(TAG, "Entering Timers menu");
        }
        else if (settings_selected_item == SETTINGS_TEMPERATURE)
        {
            ui_current_state = UI_STATE_TEMPERATURE_MENU;
            temp_selected_index = 0;
            ESP_LOGI(TAG, "Entering Temperature menu");
        }
        break;

    default:
        break;
    }
}

static void handle_timers_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_SETTINGS_MENU;
        break;

    case UI_EVENT_ROTARY_CW:
        timer_selected_index = MENU_WRAP(timer_selected_index + 1, TIMER_COUNT);
        break;

    case UI_EVENT_ROTARY_CCW:
        timer_selected_index = MENU_WRAP(timer_selected_index - 1, TIMER_COUNT);
        break;

    case UI_EVENT_ROTARY_PUSH:
        ui_current_state = UI_STATE_TIMER_ADJUST;
        ESP_LOGI(TAG, "Entering timer adjustment for: %d", timer_selected_index);
        break;

    default:
        break;
    }
}

static void handle_timer_adjust_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_TIMERS_MENU;
        ESP_LOGI(TAG, "Timer adjustment cancelled");
        break;

    case UI_EVENT_BUTTON_SAVE:
        save_persistent_data();
        ui_current_state = UI_STATE_TIMERS_MENU;
        ESP_LOGI(TAG, "Timer saved to persistent storage");
        break;

    case UI_EVENT_ROTARY_CW:
        if (timer_selected_index == TIMER_STAGE1)
        {
            current_settings->stage1_default = CLAMP(current_settings->stage1_default + 1, 1, 300);
        }
        else if (timer_selected_index == TIMER_STAGE2)
        {
            current_settings->stage2_default = CLAMP(current_settings->stage2_default + 1, 1, 300);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (timer_selected_index == TIMER_STAGE1)
        {
            current_settings->stage1_default = CLAMP(current_settings->stage1_default - 1, 1, 300);
        }
        else if (timer_selected_index == TIMER_STAGE2)
        {
            current_settings->stage2_default = CLAMP(current_settings->stage2_default - 1, 1, 300);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        save_persistent_data();
        ui_current_state = UI_STATE_TIMERS_MENU;
        ESP_LOGI(TAG, "Timer confirmed and saved");
        break;

    default:
        break;
    }
}

static void handle_temperature_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_SETTINGS_MENU;
        break;

    case UI_EVENT_ROTARY_CW:
        temp_selected_index = MENU_WRAP(temp_selected_index + 1, TEMP_COUNT);
        break;

    case UI_EVENT_ROTARY_CCW:
        temp_selected_index = MENU_WRAP(temp_selected_index - 1, TEMP_COUNT);
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (temp_selected_index == TEMP_TARGET_TEMP)
        {
            ui_current_state = UI_STATE_TEMP_ADJUST;
            ESP_LOGI(TAG, "Entering target temp adjustment");
        }
        else if (temp_selected_index == TEMP_PID_CONTROL)
        {
            ui_current_state = UI_STATE_PID_MENU;
            pid_selected_index = 0;
            ESP_LOGI(TAG, "Entering PID Control menu");
        }
        break;

    default:
        break;
    }
}

static void handle_temp_adjust_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_TEMPERATURE_MENU;
        ESP_LOGI(TAG, "Temperature adjustment cancelled");
        break;

    case UI_EVENT_BUTTON_SAVE:
        save_persistent_data();
        ui_current_state = UI_STATE_TEMPERATURE_MENU;
        ESP_LOGI(TAG, "Temperature saved to persistent storage");
        break;

    case UI_EVENT_ROTARY_CW:
        current_settings->target_temp = CLAMP(current_settings->target_temp + 1.0f, 0.0f, 250.0f);
        break;

    case UI_EVENT_ROTARY_CCW:
        current_settings->target_temp = CLAMP(current_settings->target_temp - 1.0f, 0.0f, 250.0f);
        break;

    case UI_EVENT_ROTARY_PUSH:
        save_persistent_data();
        ui_current_state = UI_STATE_TEMPERATURE_MENU;
        ESP_LOGI(TAG, "Temperature confirmed and saved");
        break;

    default:
        break;
    }
}

static void handle_pid_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_TEMPERATURE_MENU;
        break;

    case UI_EVENT_ROTARY_CW:
        pid_selected_index = MENU_WRAP(pid_selected_index + 1, PID_COUNT);
        break;

    case UI_EVENT_ROTARY_CCW:
        pid_selected_index = MENU_WRAP(pid_selected_index - 1, PID_COUNT);
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (pid_selected_index == PID_AUTOTUNE)
        {
            // Start auto-tune at current target temperature
            if (start_pid_autotune(current_settings->target_temp))
            {
                ui_current_state = UI_STATE_AUTOTUNE;
                ESP_LOGI(TAG, "Starting auto-tune from UI");
            }
            else
            {
                ESP_LOGW(TAG, "Failed to start auto-tune from UI");
            }
        }
        else
        {
            ui_current_state = UI_STATE_PID_ADJUST;
            ESP_LOGI(TAG, "Entering PID adjustment for: %d", pid_selected_index);
        }
        break;

    default:
        break;
    }
}

static void handle_pid_adjust_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_PID_MENU;
        ESP_LOGI(TAG, "PID adjustment cancelled");
        break;

    case UI_EVENT_BUTTON_SAVE:
        save_persistent_data();
        ui_current_state = UI_STATE_PID_MENU;
        ESP_LOGI(TAG, "PID saved to persistent storage");
        break;

    case UI_EVENT_ROTARY_CW:
        if (pid_selected_index == PID_KP)
        {
            current_settings->pid_kp = CLAMP(current_settings->pid_kp + 0.1f, 0.0f, 100.0f);
        }
        else if (pid_selected_index == PID_KI)
        {
            current_settings->pid_ki = CLAMP(current_settings->pid_ki + 0.01f, 0.0f, 10.0f);
        }
        else if (pid_selected_index == PID_KD)
        {
            current_settings->pid_kd = CLAMP(current_settings->pid_kd + 0.1f, 0.0f, 100.0f);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (pid_selected_index == PID_KP)
        {
            current_settings->pid_kp = CLAMP(current_settings->pid_kp - 0.1f, 0.0f, 100.0f);
        }
        else if (pid_selected_index == PID_KI)
        {
            current_settings->pid_ki = CLAMP(current_settings->pid_ki - 0.01f, 0.0f, 10.0f);
        }
        else if (pid_selected_index == PID_KD)
        {
            current_settings->pid_kd = CLAMP(current_settings->pid_kd - 0.1f, 0.0f, 100.0f);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        save_persistent_data();
        ui_current_state = UI_STATE_PID_MENU;
        ESP_LOGI(TAG, "PID confirmed and saved");
        break;

    default:
        break;
    }
}

static void handle_start_pressing_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_PRESS_CLOSED:
        // Don't transition here - let main.c handle the press detection and start the cycle
        // main.c will transition to UI_STATE_PRESSING_ACTIVE when cycle starts
        break;

    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    default:
        break;
    }
}

static void handle_pressing_active_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_PRESS_OPENED:
        // Complete current stage - handled by main application
        break;

    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    default:
        break;
    }
}

static void handle_statistics_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    default:
        break;
    }
}

// =============================================================================
// Display Rendering Implementations
// =============================================================================

static void render_main_menu(void)
{
    display_menu(main_menu_items, MENU_COUNT, menu_selected_item);
}

static void render_job_setup(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < JOB_SETUP_ITEM_COUNT; i++)
    {
        if (i == JOB_ITEM_NUM_SHIRTS)
        {
            if (i == job_setup_selected_index)
            {
                snprintf(line, sizeof(line), "> %-11s %4d", job_setup_items[i], current_run->num_shirts);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-11s %4d", job_setup_items[i], current_run->num_shirts);
            }
        }
        else // JOB_ITEM_PRINT_TYPE
        {
            const char *type_str = (current_run->type == SINGLE_SIDED) ? "SS" : "DS";
            if (i == job_setup_selected_index)
            {
                snprintf(line, sizeof(line), "> %-11s %4s", job_setup_items[i], type_str);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-11s %4s", job_setup_items[i], type_str);
            }
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

static void render_job_setup_adjust(void)
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

static void render_print_type_select(void)
{
    display_menu(print_type_items, 2, print_type_selected_index);
}

static void render_settings_menu(void)
{
    display_menu(settings_menu_items, SETTINGS_COUNT, settings_selected_item);
}

static void render_timers_menu(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < TIMER_COUNT; i++)
    {
        int value = (i == TIMER_STAGE1) ? current_settings->stage1_default : current_settings->stage2_default;

        if (i == timer_selected_index)
        {
            snprintf(line, sizeof(line), "> %-10s %4ds", timer_menu_items[i], value);
        }
        else
        {
            snprintf(line, sizeof(line), "  %-10s %4ds", timer_menu_items[i], value);
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

static void render_timer_adjust(void)
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

static void render_temperature_menu(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < TEMP_COUNT; i++)
    {
        if (i == TEMP_TARGET_TEMP)
        {
            if (i == temp_selected_index)
            {
                snprintf(line, sizeof(line), "> %-9s %5.1fC", temp_menu_items[i], current_settings->target_temp);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-9s %5.1fC", temp_menu_items[i], current_settings->target_temp);
            }
        }
        else // TEMP_PID_CONTROL
        {
            if (i == temp_selected_index)
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

static void render_temp_adjust(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Adjust:");
    display_text(0, 1, "Target Temp");
    sprintf(buffer, ">> %.1f C <<", current_settings->target_temp);
    display_text(0, 2, buffer);
    display_flush();
}

static void render_pid_menu(void)
{
    char line[21];

    display_clear();

    for (uint8_t i = 0; i < PID_COUNT; i++)
    {
        if (i == PID_AUTOTUNE)
        {
            // Auto-Tune doesn't have a value to display
            if (i == pid_selected_index)
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
            if (i == pid_selected_index)
            {
                snprintf(line, sizeof(line), "> %-10s %5.2f", pid_menu_items[i], current_settings->pid_kp);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-10s %5.2f", pid_menu_items[i], current_settings->pid_kp);
            }
        }
        else if (i == PID_KI)
        {
            if (i == pid_selected_index)
            {
                snprintf(line, sizeof(line), "> %-10s %5.3f", pid_menu_items[i], current_settings->pid_ki);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-10s %5.3f", pid_menu_items[i], current_settings->pid_ki);
            }
        }
        else // PID_KD
        {
            if (i == pid_selected_index)
            {
                snprintf(line, sizeof(line), "> %-10s %5.2f", pid_menu_items[i], current_settings->pid_kd);
            }
            else
            {
                snprintf(line, sizeof(line), "  %-10s %5.2f", pid_menu_items[i], current_settings->pid_kd);
            }
        }
        display_text(0, i * 2, line);
    }

    display_flush();
}

static void render_pid_adjust(void)
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

static void render_start_pressing(void)
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

static void render_pressing_active(void)
{
    extern pressing_cycle_t current_cycle;
    extern uint32_t stage_start_time;
    extern cycle_status_t current_stage;

    static uint32_t last_time_remaining = 9999;
    static cycle_status_t last_stage = IDLE;
    static bool screen_initialized = false;

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

        // Draw stage label at top
        display_text(0, 0, (current_stage == STAGE1) ? "Stage 1" : "Stage 2");
        screen_initialized = true;
        last_stage = current_stage;
        last_time_remaining = 9999; // Force update
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

static void render_statistics(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Statistics");
    sprintf(buffer, "Completed: %d/%d",
            current_run->shirts_completed,
            current_run->num_shirts);
    display_text(0, 1, buffer);
    sprintf(buffer, "Avg time: %lu s", current_run->avg_time_per_shirt);
    display_text(0, 2, buffer);
    display_flush();
}

// =============================================================================
// Auto-Tune State Handlers (NEW)
// =============================================================================

static void handle_autotune_state(ui_event_t event)
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

static void handle_autotune_complete_state(ui_event_t event)
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

static void render_autotune(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Auto-Tuning PID");

    // Show progress
    uint8_t progress = get_autotune_progress();
    sprintf(buffer, "Progress: %d%%", progress);
    display_text(0, 1, buffer);

    // Show current temperature
    sprintf(buffer, "Temp: %.1f C", temperature_display_celsius);
    display_text(0, 2, buffer);

    // Show cancel instruction
    display_text(0, 3, "BACK to cancel");
    display_flush();
}

static void render_autotune_complete(void)
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

// New state render functions
static void render_stage1_done(void)
{
    display_clear();
    display_invert(true);
    display_large_text(20, 16, "DONE");
    display_flush();
}

static void render_stage2_ready(void)
{
    display_clear();
    display_invert(false);
    display_large_text(10, 16, "READY");
    display_flush();
}

static void render_stage2_done(void)
{
    display_clear();
    display_invert(true);
    display_large_text(20, 16, "DONE");
    display_flush();
}

static void render_cycle_complete(void)
{
    extern print_run_t print_run;
    char buffer[32];

    display_clear();
    display_invert(false);
    display_text(0, 0, "Cycle Complete!");
    sprintf(buffer, "Completed: %d/%d",
            print_run.shirts_completed,
            print_run.num_shirts);
    display_text(0, 1, buffer);
    sprintf(buffer, "Avg: %lu sec", print_run.avg_time_per_shirt);
    display_text(0, 2, buffer);
    display_text(0, 3, "Close for next");
    display_flush();
}
