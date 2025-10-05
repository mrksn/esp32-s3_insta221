#ifndef UI_STATE_H
#define UI_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "data_model.h"

// UI States
typedef enum
{
    UI_STATE_INIT,
    UI_STATE_MAIN_MENU,
    UI_STATE_JOB_SETUP,
    UI_STATE_JOB_SETUP_ADJUST,   // Adjusting a job setup value
    UI_STATE_PRINT_TYPE_SELECT,  // Selecting print type
    UI_STATE_START_PRESSING,
    UI_STATE_FREE_PRESS,         // NEW: Free press mode
    UI_STATE_PROFILES_MENU,      // NEW: Material profiles menu
    UI_STATE_PRESSING_ACTIVE,
    UI_STATE_STAGE1_DONE,        // NEW: Stage 1 done message
    UI_STATE_STAGE2_READY,       // NEW: Ready for Stage 2
    UI_STATE_STAGE2_DONE,        // NEW: Stage 2 done message
    UI_STATE_CYCLE_COMPLETE,     // NEW: Show statistics after cycle
    UI_STATE_SETTINGS_MENU,
    UI_STATE_TIMERS_MENU,        // Timers submenu
    UI_STATE_TIMER_ADJUST,       // Adjusting a timer value
    UI_STATE_TEMPERATURE_MENU,   // Temperature submenu
    UI_STATE_TEMP_ADJUST,        // Adjusting target temperature
    UI_STATE_PID_MENU,           // PID Control submenu
    UI_STATE_PID_ADJUST,         // Adjusting a PID value
    UI_STATE_STATISTICS,         // Statistics main menu
    UI_STATE_STATS_PRODUCTION,   // Production statistics view
    UI_STATE_STATS_TEMPERATURE,  // Temperature statistics view
    UI_STATE_STATS_EVENTS,       // Events statistics view
    UI_STATE_STATS_KPIS,         // KPIs statistics view
    UI_STATE_AUTOTUNE,           // NEW: Auto-tune PID state
    UI_STATE_AUTOTUNE_COMPLETE,  // NEW: Auto-tune results display
    UI_STATE_ERROR
} ui_state_t;

// Menu items
typedef enum
{
    MENU_JOB_SETUP,
    MENU_START_PRESSING,
    MENU_FREE_PRESS,     // NEW: Free press mode without job tracking
    MENU_PROFILES,       // NEW: Material profiles
    MENU_SETTINGS,
    MENU_STATISTICS,
    MENU_COUNT
} menu_item_t;

// Settings menu items (main categories)
typedef enum
{
    SETTINGS_TIMERS,
    SETTINGS_TEMPERATURE,
    SETTINGS_COUNT
} settings_item_t;

// Timer submenu items
typedef enum
{
    TIMER_STAGE1,
    TIMER_STAGE2,
    TIMER_COUNT
} timer_item_t;

// Temperature submenu items
typedef enum
{
    TEMP_TARGET_TEMP,
    TEMP_PID_CONTROL,
    TEMP_COUNT
} temp_item_t;

// PID Control submenu items
typedef enum
{
    PID_AUTOTUNE,
    PID_KP,
    PID_KI,
    PID_KD,
    PID_COUNT
} pid_item_t;

// Statistics submenu items
typedef enum
{
    STATS_PRODUCTION,
    STATS_TEMPERATURE,
    STATS_EVENTS,
    STATS_KPIS,
    STATS_COUNT
} stats_item_t;

// UI Events
typedef enum
{
    UI_EVENT_NONE,
    UI_EVENT_ROTARY_CW,
    UI_EVENT_ROTARY_CCW,
    UI_EVENT_ROTARY_PUSH,
    UI_EVENT_BUTTON_SAVE,
    UI_EVENT_BUTTON_BACK,
    UI_EVENT_PRESS_CLOSED,
    UI_EVENT_PRESS_OPENED,
    UI_EVENT_TIMEOUT
} ui_event_t;

// UI State Machine
void ui_init(settings_t *settings, print_run_t *print_run);
void ui_update(float current_temp);
ui_event_t ui_get_event(void);
void ui_process_event(ui_event_t event);

// State management
ui_state_t ui_get_current_state(void);
void ui_set_state(ui_state_t state);

// Menu management
void ui_select_menu_item(menu_item_t item);
menu_item_t ui_get_selected_item(void);
void ui_adjust_value(int8_t delta);

// Free press mode
bool ui_is_free_press_mode(void);
void ui_increment_free_press_count(void);
void ui_update_free_press_timing(uint32_t elapsed_time);
uint32_t ui_get_free_press_run_start_time(void);
void ui_set_free_press_run_start_time(uint32_t start_time);

// Display update
void ui_update_display(void);

// External functions from main.c
extern void save_persistent_data(void);

#endif // UI_STATE_H
