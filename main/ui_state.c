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

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_err.h>
#include "ui_state.h"
#include "controls_contract.h"    // components/controls/include/
#include "display_contract.h"     // components/display/include/
#include "heating_contract.h"     // components/heating/include/
#include "data_model.h"           // components/storage/include/
#include "system_config.h"
#include "system_constants.h"     // System-wide constants
#include "main.h"                 // For is_heat_press_ready()

static const char *TAG = "ui_state";

// =============================================================================
// State Variables
// =============================================================================

// Current state
// Non-static variables (shared with ui_helpers.c and future split files)
ui_state_t ui_current_state = UI_STATE_INIT;
bool display_needs_update = true;

// Static variables (local to ui_state.c)
static ui_state_t ui_previous_state = UI_STATE_INIT;

// Menu selection variables (non-static - shared with ui_renderers.c)
menu_item_t menu_selected_item = MENU_PROFILES;
settings_item_t settings_selected_item = SETTINGS_TIMERS;

// Menu items (non-static - shared with ui_renderers.c)
const char *main_menu_items[] = {
    "Profiles",
    "Job Setup",
    "Heat Up",
    "Job Press",
    "Free Press",
    "Statistics",
    "Settings"
};

const char *profile_items[] = {
    "Cotton",
    "Polyester",
    "Blockout",
    "Wood",
    "Metal"
};

const char *stats_menu_items[] = {
    "Production",
    "Temperature",
    "Events",
    "KPIs"
};

const char *settings_menu_items[] = {
    "Timers",
    "Temperature",
    "Reset Stats"
};

const char *timer_menu_items[] = {
    "Stage 1",
    "Stage 2"
};

const char *temp_menu_items[] = {
    "Target C",
    "PID Control"
};

const char *pid_menu_items[] = {
    "Auto-Tune",
    "Kp",
    "Ki",
    "Kd"
};

const char *job_setup_items[] = {
    "Shirts #",
    "Sides #"
};

const char *print_type_items[] = {
    "Single Sided",
    "Double Sided"
};

// Current values for adjustment
static int ui_adjustment_value = 0;
static int ui_adjustment_min = 0;
static int ui_adjustment_max = 100;

// Settings and print run references (non-static - shared with ui_helpers.c)
settings_t *current_settings = NULL;
print_run_t *current_run = NULL;

// Current temperature for display (non-static - shared with ui_helpers.c)
float temperature_display_celsius = 0.0f;

// Callback functions (non-static - shared with ui_renderers.c)
ui_callbacks_t ui_callbacks = {0};

// Job setup state (non-static - shared with ui_renderers.c)
int job_setup_selected_index = 0;
bool job_setup_edit_mode = false;
int job_setup_staged_num_shirts = 0;
printing_type_t job_setup_staged_print_type = SINGLE_SIDED;

// Print type selection state (non-static - shared with ui_renderers.c)
int print_type_selected_index = 0;

// Profile selection state
typedef enum {
    PROFILE_COTTON,
    PROFILE_POLYESTER,
    PROFILE_BLOCKOUT,
    PROFILE_WOOD,
    PROFILE_METAL,
    PROFILE_COUNT
} profile_type_t;

int profile_selected_index = 0;  // non-static - shared with ui_renderers.c

// Statistics submenu state (non-static - shared with ui_renderers.c)
int stats_selected_index = 0;

// Settings submenu state (non-static - shared with ui_renderers.c)
int timer_selected_index = 0;
bool timer_edit_mode = false;
int timer_staged_value = 0;

int temp_selected_index = 0;
bool temp_edit_mode = false;
float temp_staged_value = 0.0f;

int pid_selected_index = 0;
bool pid_edit_mode = false;
float pid_staged_value = 0.0f;

// Press state tracking
static bool was_press_closed_ui = false;

// Free press mode tracking (non-static - shared with ui_helpers.c)
bool free_press_mode = false;              ///< Whether we're in free press mode (no job tracking)
uint16_t free_press_count = 0;             ///< Number of shirts pressed in free press mode
uint32_t free_press_time_elapsed = 0;      ///< Time elapsed in free press mode
uint32_t free_press_avg_time = 0;          ///< Average time per shirt in free press mode
uint32_t free_press_run_start_time = 0;    ///< When free press session started

// Reset statistics state tracking (non-static - shared with ui_renderers.c)
int reset_stats_selected_index = 0;        ///< Selected reset option (0=Job, 1=All)
uint32_t reset_stats_press_start_time = 0; ///< When encoder push started
bool reset_stats_button_pressed = false;   ///< Whether button is currently pressed

// Heat up mode tracking (non-static - shared with ui_helpers.c)
uint32_t heat_up_start_time = 0;           ///< When heat up started
float heat_up_start_temp = 0.0f;           ///< Temperature when heat up started
ui_state_t heat_up_return_state = UI_STATE_MAIN_MENU; ///< State to return to after heat-up completes

// Heat up render state (non-static - shared with ui_helpers.c)
bool heat_up_heating_was_active = false;   ///< Tracks heating switch state changes
uint32_t heat_up_last_update_sec = 0;      ///< Last elapsed second when display updated
bool heat_up_screen_initialized = false;   ///< Whether screen has been initialized

// =============================================================================
// Helper Function Prototypes - See ui_helpers.c
// =============================================================================

// Mode initialization and statistics helpers (defined in ui_helpers.c)
void init_free_press_mode(void);
void init_job_press_mode(void);
void enter_heat_up_mode(ui_state_t return_to);
void reset_free_press_stats(void);
void reset_print_run_stats(void);
void perform_job_stats_reset(void);
void perform_all_stats_reset(void);

// =============================================================================
// State Handler Function Prototypes
// =============================================================================

static void handle_startup_state(ui_event_t event);        // NEW
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
static void handle_free_press_state(ui_event_t event);         // NEW
static void handle_profiles_menu_state(ui_event_t event);      // NEW
static void handle_pressing_active_state(ui_event_t event);
static void handle_stage1_done_state(ui_event_t event);        // NEW
static void handle_stage2_ready_state(ui_event_t event);       // NEW
static void handle_stage2_done_state(ui_event_t event);        // NEW
static void handle_cycle_complete_state(ui_event_t event);     // NEW
static void handle_statistics_state(ui_event_t event);
static void handle_stats_production_state(ui_event_t event);   // NEW
static void handle_stats_temperature_state(ui_event_t event);  // NEW
static void handle_stats_events_state(ui_event_t event);       // NEW
static void handle_stats_kpis_state(ui_event_t event);         // NEW
// Note: handle_autotune_state, handle_autotune_complete_state, handle_reset_stats_state,
// and handle_heat_up_state are now in ui_renderers.c

// Display rendering functions (defined in ui_renderers.c)
void render_startup(void);
void render_main_menu(void);
void render_job_setup(void);
void render_job_setup_adjust(void);
void render_print_type_select(void);
void render_settings_menu(void);
void render_timers_menu(void);
void render_timer_adjust(void);
void render_temperature_menu(void);
void render_temp_adjust(void);
void render_pid_menu(void);
void render_pid_adjust(void);
void render_start_pressing(void);
void render_free_press(void);
void render_profiles_menu(void);
void render_pressing_active(void);
void render_statistics(void);
void render_stats_production(void);
void render_stats_temperature(void);
void render_stats_events(void);
void render_stats_kpis(void);
void render_autotune(void);
void render_autotune_complete(void);
void render_reset_stats(void);
void render_heat_up(void);
void render_stage1_done(void);
void render_stage2_ready(void);
void render_stage2_done(void);
void render_cycle_complete(void);

// Event handlers for specific states (defined in ui_renderers.c)
void handle_autotune_state(ui_event_t event);
void handle_autotune_complete_state(ui_event_t event);
void handle_reset_stats_state(ui_event_t event);
void handle_heat_up_state(ui_event_t event);

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
    {UI_STATE_STARTUP, handle_startup_state, render_startup, "Startup"},
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
    {UI_STATE_FREE_PRESS, handle_free_press_state, render_free_press, "Free Press"},
    {UI_STATE_PROFILES_MENU, handle_profiles_menu_state, render_profiles_menu, "Profiles"},
    {UI_STATE_PRESSING_ACTIVE, handle_pressing_active_state, render_pressing_active, "Pressing Active"},
    {UI_STATE_STAGE1_DONE, handle_stage1_done_state, render_stage1_done, "Stage 1 Done"},
    {UI_STATE_STAGE2_READY, handle_stage2_ready_state, render_stage2_ready, "Stage 2 Ready"},
    {UI_STATE_STAGE2_DONE, handle_stage2_done_state, render_stage2_done, "Stage 2 Done"},
    {UI_STATE_CYCLE_COMPLETE, handle_cycle_complete_state, render_cycle_complete, "Cycle Complete"},
    {UI_STATE_STATISTICS, handle_statistics_state, render_statistics, "Statistics"},
    {UI_STATE_STATS_PRODUCTION, handle_stats_production_state, render_stats_production, "Production Stats"},
    {UI_STATE_STATS_TEMPERATURE, handle_stats_temperature_state, render_stats_temperature, "Temperature Stats"},
    {UI_STATE_STATS_EVENTS, handle_stats_events_state, render_stats_events, "Events Stats"},
    {UI_STATE_STATS_KPIS, handle_stats_kpis_state, render_stats_kpis, "KPI Stats"},
    {UI_STATE_AUTOTUNE, handle_autotune_state, render_autotune, "Auto-Tune"},
    {UI_STATE_AUTOTUNE_COMPLETE, handle_autotune_complete_state, render_autotune_complete, "Results"},
    {UI_STATE_RESET_STATS, handle_reset_stats_state, render_reset_stats, "Reset Stats"},
    {UI_STATE_HEAT_UP, handle_heat_up_state, render_heat_up, "Heat Up"},
};

// =============================================================================
// Public Interface Implementation
// =============================================================================

void ui_init(settings_t *settings, print_run_t *print_run)
{
    // Validation: Check for NULL pointers
    if (settings == NULL)
    {
        ESP_LOGE(TAG, "ui_init: NULL settings pointer");
        return;
    }

    if (print_run == NULL)
    {
        ESP_LOGE(TAG, "ui_init: NULL print_run pointer");
        return;
    }

    current_settings = settings;
    current_run = print_run;
    ui_current_state = UI_STATE_STARTUP; // Start with startup screen
    ESP_LOGI(TAG, "UI state machine initialized - showing startup screen");
}

void ui_register_callbacks(const ui_callbacks_t *callbacks)
{
    if (callbacks == NULL)
    {
        ESP_LOGE(TAG, "ui_register_callbacks: NULL callbacks pointer");
        return;
    }

    ui_callbacks = *callbacks;
    ESP_LOGI(TAG, "UI callbacks registered successfully");
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

    // Update display during heat up only once per second to reduce flicker
    static uint32_t last_heat_up_update = 0;
    if (ui_current_state == UI_STATE_HEAT_UP)
    {
        uint32_t current_time_ms = esp_timer_get_time() / 1000;
        if (current_time_ms - last_heat_up_update >= 1000) // Update once per second
        {
            display_needs_update = true;
            last_heat_up_update = current_time_ms;
        }
    }

    // Check for button release during reset stats (even if not updating display)
    if (ui_current_state == UI_STATE_RESET_STATS && reset_stats_button_pressed)
    {
        // Calculate elapsed time
        uint32_t current_time = esp_timer_get_time() / 1000; // milliseconds
        uint32_t elapsed_ms = current_time - reset_stats_press_start_time;

        // Check if button was released early
        if (!controls_is_rotary_button_pressed() && elapsed_ms < 4000)
        {
            ESP_LOGI(TAG, "Button released too early (%lu ms)", elapsed_ms);
            reset_stats_button_pressed = false;
            reset_stats_press_start_time = 0;
            display_needs_update = true; // Return to menu
        }
        // Force display updates during countdown period (after 1 second)
        else if (elapsed_ms >= 1000)
        {
            display_needs_update = true;
        }
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
    // Validation: Ensure current_settings and current_run are valid before processing
    if (current_settings == NULL || current_run == NULL)
    {
        ESP_LOGE(TAG, "ui_process_event: NULL settings or run pointer, cannot process events");
        return;
    }

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

bool ui_is_free_press_mode(void)
{
    return free_press_mode;
}

void ui_increment_free_press_count(void)
{
    free_press_count++;
}

void ui_update_free_press_timing(uint32_t elapsed_time)
{
    free_press_time_elapsed = elapsed_time;
    if (free_press_count > 0)
    {
        free_press_avg_time = free_press_time_elapsed / free_press_count;
    }
}

uint32_t ui_get_free_press_run_start_time(void)
{
    return free_press_run_start_time;
}

void ui_set_free_press_run_start_time(uint32_t start_time)
{
    free_press_run_start_time = start_time;
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

static void handle_startup_state(ui_event_t event)
{
    // Startup screen doesn't handle any events
    // It will automatically transition to main menu after 3 seconds (handled in main.c)
    (void)event; // Suppress unused parameter warning
}

static void handle_main_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_CW:
        menu_selected_item = MENU_WRAP(menu_selected_item + 1, MENU_COUNT);
        ESP_LOGI(TAG, "Menu item selected: %d", menu_selected_item);
        break;

    case UI_EVENT_ROTARY_CCW:
        menu_selected_item = MENU_WRAP((int)menu_selected_item - 1, MENU_COUNT);
        ESP_LOGI(TAG, "Menu item selected: %d", menu_selected_item);
        break;

    case UI_EVENT_ROTARY_PUSH:
        ESP_LOGI(TAG, "Encoder push pressed, entering menu item: %d", menu_selected_item);
        switch (menu_selected_item)
        {
        case MENU_PROFILES:
            ui_current_state = UI_STATE_PROFILES_MENU;
            profile_selected_index = 0;
            break;
        case MENU_JOB_SETUP:
            ESP_LOGI(TAG, "Entering JOB_SETUP state");
            ui_current_state = UI_STATE_JOB_SETUP;
            job_setup_selected_index = 0;
            break;
        case MENU_HEAT_UP:
            // Always show heat-up screen when Heat Up is explicitly selected
            enter_heat_up_mode(UI_STATE_HEAT_UP);
            break;

        case MENU_START_PRESSING:
            // Check if target temp has been reached at least once
            if (!has_reached_target_temp_once())
            {
                // Not ready - show heat-up screen first
                enter_heat_up_mode(UI_STATE_START_PRESSING);
            }
            else
            {
                // Target reached - go directly to job press
                ui_current_state = UI_STATE_START_PRESSING;
                init_job_press_mode();
            }
            break;

        case MENU_FREE_PRESS:
            // Check if target temp has been reached at least once
            if (!has_reached_target_temp_once())
            {
                // Not ready - show heat-up screen first
                enter_heat_up_mode(UI_STATE_FREE_PRESS);
            }
            else
            {
                // Target reached - go directly to free press
                ui_current_state = UI_STATE_FREE_PRESS;
                init_free_press_mode();
            }
            break;
        case MENU_STATISTICS:
            ui_current_state = UI_STATE_STATISTICS;
            break;
        case MENU_SETTINGS:
            ui_current_state = UI_STATE_SETTINGS_MENU;
            settings_selected_item = 0;
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
        if (job_setup_edit_mode)
        {
            // Exit edit mode without saving - discard staged changes
            job_setup_edit_mode = false;
            ESP_LOGI(TAG, "Job setup edit cancelled");
        }
        else
        {
            // Exit to main menu
            ui_current_state = UI_STATE_MAIN_MENU;
        }
        break;

    case UI_EVENT_ROTARY_CW:
        if (job_setup_edit_mode)
        {
            // Adjust staged value
            if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
            {
                job_setup_staged_num_shirts = CLAMP(job_setup_staged_num_shirts + 1,
                                                      NUM_SHIRTS_MIN,
                                                      NUM_SHIRTS_MAX);
            }
            else if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
            {
                job_setup_staged_print_type = (job_setup_staged_print_type == SINGLE_SIDED) ?
                                               DOUBLE_SIDED : SINGLE_SIDED;
            }
        }
        else
        {
            // Navigate menu
            job_setup_selected_index = MENU_WRAP(job_setup_selected_index + 1,
                                                  JOB_SETUP_ITEM_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (job_setup_edit_mode)
        {
            // Adjust staged value
            if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
            {
                job_setup_staged_num_shirts = CLAMP(job_setup_staged_num_shirts - 1,
                                                      NUM_SHIRTS_MIN,
                                                      NUM_SHIRTS_MAX);
            }
            else if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
            {
                job_setup_staged_print_type = (job_setup_staged_print_type == SINGLE_SIDED) ?
                                               DOUBLE_SIDED : SINGLE_SIDED;
            }
        }
        else
        {
            // Navigate menu
            job_setup_selected_index = MENU_WRAP(job_setup_selected_index - 1,
                                                  JOB_SETUP_ITEM_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (job_setup_edit_mode)
        {
            // Commit staged changes and save
            if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
            {
                current_run->num_shirts = job_setup_staged_num_shirts;
            }
            else if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
            {
                current_run->type = job_setup_staged_print_type;
            }
            save_persistent_data();
            job_setup_edit_mode = false;
            ESP_LOGI(TAG, "Job setup value saved");
        }
        else
        {
            // Enter edit mode - initialize staged value with current value
            if (job_setup_selected_index == JOB_ITEM_NUM_SHIRTS)
            {
                job_setup_staged_num_shirts = current_run->num_shirts;
            }
            else if (job_setup_selected_index == JOB_ITEM_PRINT_TYPE)
            {
                job_setup_staged_print_type = current_run->type;
            }
            job_setup_edit_mode = true;
            ESP_LOGI(TAG, "Entering edit mode for: %s", job_setup_items[job_setup_selected_index]);
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
        save_persistent_data();
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Job setup value confirmed and saved");
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
        save_persistent_data();
        ui_current_state = UI_STATE_JOB_SETUP;
        ESP_LOGI(TAG, "Print type set to: %d and saved", current_run->type);
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
        else if (settings_selected_item == SETTINGS_RESET_STATS)
        {
            ui_current_state = UI_STATE_RESET_STATS;
            reset_stats_selected_index = 0;
            ESP_LOGI(TAG, "Entering Reset Stats menu");
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
        if (timer_edit_mode)
        {
            // Exit edit mode without saving - discard staged changes
            timer_edit_mode = false;
            ESP_LOGI(TAG, "Timer edit cancelled");
        }
        else
        {
            // Exit to settings menu
            ui_current_state = UI_STATE_SETTINGS_MENU;
        }
        break;

    case UI_EVENT_ROTARY_CW:
        if (timer_edit_mode)
        {
            // Adjust staged value
            timer_staged_value = CLAMP(timer_staged_value + 1, 1, 300);
        }
        else
        {
            // Navigate menu
            timer_selected_index = MENU_WRAP(timer_selected_index + 1, TIMER_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (timer_edit_mode)
        {
            // Adjust staged value
            timer_staged_value = CLAMP(timer_staged_value - 1, 1, 300);
        }
        else
        {
            // Navigate menu
            timer_selected_index = MENU_WRAP(timer_selected_index - 1, TIMER_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (timer_edit_mode)
        {
            // Commit staged changes and save
            if (timer_selected_index == TIMER_STAGE1)
            {
                current_settings->stage1_default = timer_staged_value;
            }
            else if (timer_selected_index == TIMER_STAGE2)
            {
                current_settings->stage2_default = timer_staged_value;
            }
            save_persistent_data();
            timer_edit_mode = false;
            ESP_LOGI(TAG, "Timer value saved");
        }
        else
        {
            // Enter edit mode - initialize staged value with current value
            if (timer_selected_index == TIMER_STAGE1)
            {
                timer_staged_value = current_settings->stage1_default;
            }
            else if (timer_selected_index == TIMER_STAGE2)
            {
                timer_staged_value = current_settings->stage2_default;
            }
            timer_edit_mode = true;
            ESP_LOGI(TAG, "Entering edit mode for: %s", timer_menu_items[timer_selected_index]);
        }
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
        if (temp_edit_mode)
        {
            // Exit edit mode without saving - discard staged changes
            temp_edit_mode = false;
            ESP_LOGI(TAG, "Temperature edit cancelled");
        }
        else
        {
            // Exit to settings menu
            ui_current_state = UI_STATE_SETTINGS_MENU;
        }
        break;

    case UI_EVENT_ROTARY_CW:
        if (temp_edit_mode)
        {
            // Adjust staged value (only Target Temp is editable here)
            if (temp_selected_index == TEMP_TARGET_TEMP)
            {
                temp_staged_value = CLAMP(temp_staged_value + 1.0f, 0.0f, 250.0f);
            }
        }
        else
        {
            // Navigate menu
            temp_selected_index = MENU_WRAP(temp_selected_index + 1, TEMP_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (temp_edit_mode)
        {
            // Adjust staged value (only Target Temp is editable here)
            if (temp_selected_index == TEMP_TARGET_TEMP)
            {
                temp_staged_value = CLAMP(temp_staged_value - 1.0f, 0.0f, 250.0f);
            }
        }
        else
        {
            // Navigate menu
            temp_selected_index = MENU_WRAP(temp_selected_index - 1, TEMP_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (temp_edit_mode)
        {
            // Commit staged changes and save
            current_settings->target_temp = temp_staged_value;
            save_persistent_data();
            temp_edit_mode = false;
            ESP_LOGI(TAG, "Temperature value saved");
        }
        else
        {
            // Check what was selected
            if (temp_selected_index == TEMP_TARGET_TEMP)
            {
                // Enter edit mode - initialize staged value with current value
                temp_staged_value = current_settings->target_temp;
                temp_edit_mode = true;
                ESP_LOGI(TAG, "Entering edit mode for Target Temp");
            }
            else if (temp_selected_index == TEMP_PID_CONTROL)
            {
                // Navigate to PID submenu
                ui_current_state = UI_STATE_PID_MENU;
                pid_selected_index = 0;
                ESP_LOGI(TAG, "Entering PID Control menu");
            }
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
        if (pid_edit_mode)
        {
            // Exit edit mode without saving - discard staged changes
            pid_edit_mode = false;
            ESP_LOGI(TAG, "PID edit cancelled");
        }
        else
        {
            // Exit to temperature menu
            ui_current_state = UI_STATE_TEMPERATURE_MENU;
        }
        break;

    case UI_EVENT_ROTARY_CW:
        if (pid_edit_mode)
        {
            // Adjust staged value
            if (pid_selected_index == PID_KP)
            {
                pid_staged_value = CLAMP(pid_staged_value + 0.1f, 0.0f, 100.0f);
            }
            else if (pid_selected_index == PID_KI)
            {
                pid_staged_value = CLAMP(pid_staged_value + 0.01f, 0.0f, 10.0f);
            }
            else if (pid_selected_index == PID_KD)
            {
                pid_staged_value = CLAMP(pid_staged_value + 0.1f, 0.0f, 100.0f);
            }
        }
        else
        {
            // Navigate menu
            pid_selected_index = MENU_WRAP(pid_selected_index + 1, PID_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_CCW:
        if (pid_edit_mode)
        {
            // Adjust staged value
            if (pid_selected_index == PID_KP)
            {
                pid_staged_value = CLAMP(pid_staged_value - 0.1f, 0.0f, 100.0f);
            }
            else if (pid_selected_index == PID_KI)
            {
                pid_staged_value = CLAMP(pid_staged_value - 0.01f, 0.0f, 10.0f);
            }
            else if (pid_selected_index == PID_KD)
            {
                pid_staged_value = CLAMP(pid_staged_value - 0.1f, 0.0f, 100.0f);
            }
        }
        else
        {
            // Navigate menu
            pid_selected_index = MENU_WRAP(pid_selected_index - 1, PID_COUNT);
        }
        break;

    case UI_EVENT_ROTARY_PUSH:
        if (pid_edit_mode)
        {
            // Commit staged changes and save
            if (pid_selected_index == PID_KP)
            {
                current_settings->pid_kp = pid_staged_value;
            }
            else if (pid_selected_index == PID_KI)
            {
                current_settings->pid_ki = pid_staged_value;
            }
            else if (pid_selected_index == PID_KD)
            {
                current_settings->pid_kd = pid_staged_value;
            }
            save_persistent_data();
            pid_edit_mode = false;
            ESP_LOGI(TAG, "PID value saved");
        }
        else
        {
            // Check what was selected
            if (pid_selected_index == PID_AUTOTUNE)
            {
                // Start auto-tune at current target temperature
                if (ui_callbacks.start_autotune != NULL &&
                    ui_callbacks.start_autotune(current_settings->target_temp))
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
                // Enter edit mode - initialize staged value with current value
                if (pid_selected_index == PID_KP)
                {
                    pid_staged_value = current_settings->pid_kp;
                }
                else if (pid_selected_index == PID_KI)
                {
                    pid_staged_value = current_settings->pid_ki;
                }
                else if (pid_selected_index == PID_KD)
                {
                    pid_staged_value = current_settings->pid_kd;
                }
                pid_edit_mode = true;
                ESP_LOGI(TAG, "Entering edit mode for: %s", pid_menu_items[pid_selected_index]);
            }
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

static void handle_free_press_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_PRESS_CLOSED:
        // Don't transition here - let main.c handle the press detection and start the cycle
        // main.c will transition to UI_STATE_PRESSING_ACTIVE when cycle starts
        break;

    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        free_press_mode = false;
        break;

    default:
        break;
    }
}

static void handle_profiles_menu_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_CW:
        profile_selected_index = (profile_selected_index + 1) % PROFILE_COUNT;
        break;

    case UI_EVENT_ROTARY_CCW:
        profile_selected_index = (profile_selected_index - 1 + PROFILE_COUNT) % PROFILE_COUNT;
        break;

    case UI_EVENT_ROTARY_PUSH:
        // Apply selected profile settings
        switch (profile_selected_index)
        {
        case PROFILE_COTTON:
            current_settings->target_temp = 140.0f;
            current_settings->stage1_default = 15;
            current_settings->stage2_default = 5;
            break;
        case PROFILE_POLYESTER:
            current_settings->target_temp = 125.0f;
            current_settings->stage1_default = 12;
            current_settings->stage2_default = 5;
            break;
        case PROFILE_BLOCKOUT:
            current_settings->target_temp = 125.0f;
            current_settings->stage1_default = 12;
            current_settings->stage2_default = 5;
            break;
        case PROFILE_WOOD:
            current_settings->target_temp = 170.0f;
            current_settings->stage1_default = 20;
            current_settings->stage2_default = 5;
            break;
        case PROFILE_METAL:
            current_settings->target_temp = 204.0f;
            current_settings->stage1_default = 80;
            current_settings->stage2_default = 5;
            break;
        }
        save_persistent_data();
        ESP_LOGI(TAG, "Applied profile: %s", profile_items[profile_selected_index]);
        ui_current_state = UI_STATE_MAIN_MENU;
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

static void handle_stage1_done_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        ESP_LOGI(TAG, "Stage 1 done - returning to main menu");
        break;

    default:
        break;
    }
}

static void handle_stage2_ready_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        ESP_LOGI(TAG, "Stage 2 ready - returning to main menu");
        break;

    default:
        break;
    }
}

static void handle_stage2_done_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        ESP_LOGI(TAG, "Stage 2 done - returning to main menu");
        break;

    default:
        break;
    }
}

static void handle_cycle_complete_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        ESP_LOGI(TAG, "Cycle complete - returning to main menu");
        break;

    default:
        break;
    }
}

static void handle_statistics_state(ui_event_t event)
{
    switch (event)
    {
    case UI_EVENT_ROTARY_CW:
        stats_selected_index = (stats_selected_index + 1) % STATS_COUNT;
        break;

    case UI_EVENT_ROTARY_CCW:
        stats_selected_index = (stats_selected_index - 1 + STATS_COUNT) % STATS_COUNT;
        break;

    case UI_EVENT_ROTARY_PUSH:
        switch (stats_selected_index)
        {
        case STATS_PRODUCTION:
            ui_current_state = UI_STATE_STATS_PRODUCTION;
            break;
        case STATS_TEMPERATURE:
            ui_current_state = UI_STATE_STATS_TEMPERATURE;
            break;
        case STATS_EVENTS:
            ui_current_state = UI_STATE_STATS_EVENTS;
            break;
        case STATS_KPIS:
            ui_current_state = UI_STATE_STATS_KPIS;
            break;
        }
        break;

    case UI_EVENT_BUTTON_BACK:
        ui_current_state = UI_STATE_MAIN_MENU;
        break;

    default:
        break;
    }
}

static void handle_stats_production_state(ui_event_t event)
{
    if (event == UI_EVENT_BUTTON_BACK)
    {
        ui_current_state = UI_STATE_STATISTICS;
    }
}

static void handle_stats_temperature_state(ui_event_t event)
{
    if (event == UI_EVENT_BUTTON_BACK)
    {
        ui_current_state = UI_STATE_STATISTICS;
    }
}

static void handle_stats_events_state(ui_event_t event)
{
    if (event == UI_EVENT_BUTTON_BACK)
    {
        ui_current_state = UI_STATE_STATISTICS;
    }
}

static void handle_stats_kpis_state(ui_event_t event)
{
    if (event == UI_EVENT_BUTTON_BACK)
    {
        ui_current_state = UI_STATE_STATISTICS;
    }
}

// =============================================================================
// Helper Functions - See ui_helpers.c
// =============================================================================
// Mode initialization and statistics helpers moved to ui_helpers.c

// =============================================================================
// =============================================================================
// NOTE: Display Rendering Functions
// =============================================================================
// All render_*() functions have been moved to ui_renderers.c
// Event handlers handle_autotune_state, handle_autotune_complete_state,
// handle_reset_stats_state, and handle_heat_up_state are also in ui_renderers.c
