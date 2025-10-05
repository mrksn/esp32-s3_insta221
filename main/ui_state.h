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
    UI_STATE_START_PRESSING,
    UI_STATE_PRESSING_ACTIVE,
    UI_STATE_STAGE1_DONE,        // NEW: Stage 1 done message
    UI_STATE_STAGE2_READY,       // NEW: Ready for Stage 2
    UI_STATE_STAGE2_DONE,        // NEW: Stage 2 done message
    UI_STATE_CYCLE_COMPLETE,     // NEW: Show statistics after cycle
    UI_STATE_SETTINGS_MENU,
    UI_STATE_SETTINGS_ADJUST,    // Adjusting a setting value
    UI_STATE_TIMINGS_MENU,
    UI_STATE_TEMPERATURE_MENU,
    UI_STATE_STATISTICS,
    UI_STATE_AUTOTUNE,           // NEW: Auto-tune PID state
    UI_STATE_AUTOTUNE_COMPLETE,  // NEW: Auto-tune results display
    UI_STATE_ERROR
} ui_state_t;

// Menu items
typedef enum
{
    MENU_JOB_SETUP,
    MENU_START_PRESSING,
    MENU_SETTINGS,
    MENU_STATISTICS,
    MENU_COUNT
} menu_item_t;

// Settings menu items
typedef enum
{
    SETTINGS_TARGET_TEMP,
    SETTINGS_AUTOTUNE_PID,    // NEW: Auto-tune menu item
    SETTINGS_PID_KP,
    SETTINGS_PID_KI,
    SETTINGS_PID_KD,
    SETTINGS_STAGE1_DEFAULT,
    SETTINGS_STAGE2_DEFAULT,
    SETTINGS_COUNT
} settings_item_t;

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

// Display update
void ui_update_display(void);

#endif // UI_STATE_H
