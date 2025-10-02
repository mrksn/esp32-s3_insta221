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
#include <esp_err.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "ui_state";

// =============================================================================
// State Variables
// =============================================================================

// Current state
static ui_state_t ui_current_state = UI_STATE_INIT;
static menu_item_t menu_selected_item = MENU_JOB_SETUP;
static settings_item_t settings_selected_item = SETTINGS_TARGET_TEMP;

// Menu items
static const char *main_menu_items[] = {
    "Job Setup",
    "Start Pressing",
    "Settings",
    "Statistics"
};

static const char *settings_menu_items[] = {
    "Target Temp",
    "Auto-Tune PID",   // NEW
    "PID Kp",
    "PID Ki",
    "PID Kd",
    "Stage1 Time",
    "Stage2 Time"
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
static void handle_settings_menu_state(ui_event_t event);
static void handle_start_pressing_state(ui_event_t event);
static void handle_pressing_active_state(ui_event_t event);
static void handle_statistics_state(ui_event_t event);
static void handle_autotune_state(ui_event_t event);           // NEW
static void handle_autotune_complete_state(ui_event_t event);  // NEW

// Display rendering functions
static void render_main_menu(void);
static void render_job_setup(void);
static void render_settings_menu(void);
static void render_start_pressing(void);
static void render_pressing_active(void);
static void render_statistics(void);
static void render_autotune(void);           // NEW
static void render_autotune_complete(void);  // NEW

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
    {UI_STATE_SETTINGS_MENU, handle_settings_menu_state, render_settings_menu, "Settings"},
    {UI_STATE_START_PRESSING, handle_start_pressing_state, render_start_pressing, "Start Pressing"},
    {UI_STATE_PRESSING_ACTIVE, handle_pressing_active_state, render_pressing_active, "Pressing Active"},
    {UI_STATE_STATISTICS, handle_statistics_state, render_statistics, "Statistics"},
    {UI_STATE_AUTOTUNE, handle_autotune_state, render_autotune, "Auto-Tune"},                      // NEW
    {UI_STATE_AUTOTUNE_COMPLETE, handle_autotune_complete_state, render_autotune_complete, "Results"}, // NEW
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
    }
    ui_update_display();
}

ui_event_t ui_get_event(void)
{
    button_event_t button = controls_get_button_event();
    rotary_event_t rotary = controls_get_rotary_event();
    bool is_press_closed = controls_is_press_closed();

    // Button events
    if (button == BUTTON_CONFIRM)
        return UI_EVENT_BUTTON_CONFIRM;
    if (button == BUTTON_BACK)
        return UI_EVENT_BUTTON_BACK;

    // Rotary events
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
            state_handlers[i].handler(event);
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
        break;

    case UI_EVENT_ROTARY_CCW:
        menu_selected_item = MENU_WRAP(menu_selected_item - 1, MENU_COUNT);
        break;

    case UI_EVENT_BUTTON_CONFIRM:
        switch (menu_selected_item)
        {
        case MENU_JOB_SETUP:
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
        if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
        {
            current_run->num_shirts = CLAMP(current_run->num_shirts + 1,
                                             NUM_SHIRTS_MIN,
                                             NUM_SHIRTS_MAX);
        }
        else
        {
            job_setup_selected_index = MENU_WRAP(job_setup_selected_index + 1,
                                                  JOB_SETUP_ITEM_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
        {
            current_run->num_shirts = CLAMP(current_run->num_shirts - 1,
                                             NUM_SHIRTS_MIN,
                                             NUM_SHIRTS_MAX);
        }
        else
        {
            job_setup_selected_index = MENU_WRAP(job_setup_selected_index - 1,
                                                  JOB_SETUP_ITEM_COUNT);
        }
        break;

    case UI_EVENT_BUTTON_CONFIRM:
        if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
        {
            current_run->type = (current_run->type == SINGLE_SIDED) ?
                                DOUBLE_SIDED : SINGLE_SIDED;
        }
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

    case UI_EVENT_BUTTON_CONFIRM:
        // Check if Auto-Tune PID was selected
        if (settings_selected_item == SETTINGS_AUTOTUNE_PID)
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
        // Could enter adjustment mode for other settings if needed
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
        ui_current_state = UI_STATE_PRESSING_ACTIVE;
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
    char buffer[32];

    display_clear();
    display_text(0, 0, "Job Setup");
    display_text(0, 1, job_setup_items[job_setup_selected_index]);

    if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
    {
        sprintf(buffer, "Value: %d", current_run->num_shirts);
        display_text(0, 2, buffer);
    }
    else
    {
        display_text(0, 2, print_type_items[current_run->type]);
    }
}

static void render_settings_menu(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Settings");
    display_text(0, 1, settings_menu_items[settings_selected_item]);

    switch (settings_selected_item)
    {
    case SETTINGS_TARGET_TEMP:
        sprintf(buffer, "%.1f C", current_settings->target_temp);
        break;
    case SETTINGS_AUTOTUNE_PID:  // NEW
        sprintf(buffer, "Press to start");
        break;
    case SETTINGS_PID_KP:
        sprintf(buffer, "%.2f", current_settings->pid_kp);
        break;
    case SETTINGS_PID_KI:
        sprintf(buffer, "%.2f", current_settings->pid_ki);
        break;
    case SETTINGS_PID_KD:
        sprintf(buffer, "%.2f", current_settings->pid_kd);
        break;
    case SETTINGS_STAGE1_DEFAULT:
        sprintf(buffer, "%d s", current_settings->stage1_default);
        break;
    case SETTINGS_STAGE2_DEFAULT:
        sprintf(buffer, "%d s", current_settings->stage2_default);
        break;
    default:
        sprintf(buffer, "Unknown");
        break;
    }
    display_text(0, 2, buffer);
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
}

static void render_pressing_active(void)
{
    char buffer[32];

    display_clear();
    display_text(0, 0, "Pressing Active");
    sprintf(buffer, "Temp: %.1f C", temperature_display_celsius);
    display_text(0, 1, buffer);
    display_text(0, 2, "Keep press closed");
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
    case UI_EVENT_BUTTON_CONFIRM:
    case UI_EVENT_BUTTON_BACK:
        // Return to settings menu
        ui_current_state = UI_STATE_SETTINGS_MENU;
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
}
