/**
 * @file ui_state_internal.h
 * @brief Internal declarations for UI state machine implementation
 *
 * This header contains shared declarations used across the UI state machine
 * implementation files. It should NOT be included by code outside the UI module.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef UI_STATE_INTERNAL_H
#define UI_STATE_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_state.h"
#include "data_model.h"

// =============================================================================
// External Dependencies
// =============================================================================

// These functions are provided by the main application
extern void save_persistent_data(void);

// =============================================================================
// Shared State Variables (defined in ui_state_machine.c)
// =============================================================================

// Core state machine state
extern ui_state_t ui_current_state;
extern bool display_needs_update;

// Configuration pointers
extern settings_t *current_settings;
extern print_run_t *current_run;

// Current temperature for display
extern float temperature_display_celsius;

// Callback functions
extern ui_callbacks_t ui_callbacks;

// Job setup state
extern int job_setup_selected_index;
extern bool job_setup_edit_mode;
extern int job_setup_staged_num_shirts;
extern printing_type_t job_setup_staged_print_type;

// Print type selection state
extern int print_type_selected_index;

// Profile selection state
extern int profile_selected_index;

// Statistics submenu state
extern int stats_selected_index;

// Settings submenu state
extern int timer_selected_index;
extern bool timer_edit_mode;
extern int timer_staged_value;

extern int temp_selected_index;
extern bool temp_edit_mode;
extern float temp_staged_value;

extern int pid_selected_index;
extern bool pid_edit_mode;
extern float pid_staged_value;

// Press state tracking
extern bool was_press_closed_ui;

// Free press mode tracking
extern bool free_press_mode;
extern uint16_t free_press_count;
extern uint32_t free_press_time_elapsed;
extern uint32_t free_press_avg_time;
extern uint32_t free_press_run_start_time;

// Reset statistics state tracking
extern int reset_stats_selected_index;
extern uint32_t reset_stats_press_start_time;
extern bool reset_stats_button_pressed;

// Heat up mode tracking
extern uint32_t heat_up_start_time;
extern float heat_up_start_temp;
extern bool heat_up_heating_enabled;
extern ui_state_t heat_up_return_state;
extern bool heat_up_heating_was_active;
extern uint32_t heat_up_last_update_sec;
extern bool heat_up_screen_initialized;

// Main menu state
extern int menu_selected_item;

// Settings menu state
extern int settings_selected_index;

// =============================================================================
// Profile Types
// =============================================================================

typedef enum {
    PROFILE_COTTON,
    PROFILE_POLYESTER,
    PROFILE_BLOCKOUT,
    PROFILE_WOOD,
    PROFILE_METAL,
    PROFILE_COUNT
} profile_type_t;

// =============================================================================
// Helper Functions (implemented in ui_helpers.c)
// =============================================================================

void init_free_press_mode(void);
void init_job_press_mode(void);
void enter_heat_up_mode(ui_state_t return_to);
void reset_free_press_stats(void);
void reset_print_run_stats(void);
void perform_job_stats_reset(void);
void perform_all_stats_reset(void);

// =============================================================================
// Event Handlers (implemented in ui_event_handlers.c)
// =============================================================================

void handle_startup_state(ui_event_t event);
void handle_main_menu_state(ui_event_t event);
void handle_job_setup_state(ui_event_t event);
void handle_job_setup_adjust_state(ui_event_t event);
void handle_print_type_select_state(ui_event_t event);
void handle_settings_menu_state(ui_event_t event);
void handle_timers_menu_state(ui_event_t event);
void handle_timer_adjust_state(ui_event_t event);
void handle_temperature_menu_state(ui_event_t event);
void handle_temp_adjust_state(ui_event_t event);
void handle_pid_menu_state(ui_event_t event);
void handle_pid_adjust_state(ui_event_t event);
void handle_start_pressing_state(ui_event_t event);
void handle_free_press_state(ui_event_t event);
void handle_profiles_menu_state(ui_event_t event);
void handle_pressing_active_state(ui_event_t event);
void handle_stage1_done_state(ui_event_t event);
void handle_stage2_ready_state(ui_event_t event);
void handle_stage2_done_state(ui_event_t event);
void handle_cycle_complete_state(ui_event_t event);
void handle_statistics_state(ui_event_t event);
void handle_stats_production_state(ui_event_t event);
void handle_stats_temperature_state(ui_event_t event);
void handle_stats_events_state(ui_event_t event);
void handle_stats_kpis_state(ui_event_t event);
void handle_autotune_state(ui_event_t event);
void handle_autotune_complete_state(ui_event_t event);
void handle_reset_stats_state(ui_event_t event);
void handle_heat_up_state(ui_event_t event);

// =============================================================================
// Renderers (implemented in ui_renderers.c)
// =============================================================================

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
void render_stage1_done(void);
void render_stage2_ready(void);
void render_stage2_done(void);
void render_cycle_complete(void);
void render_statistics(void);
void render_stats_production(void);
void render_stats_temperature(void);
void render_stats_events(void);
void render_stats_kpis(void);
void render_autotune(void);
void render_autotune_complete(void);
void render_reset_stats(void);
void render_heat_up(void);

#endif // UI_STATE_INTERNAL_H
