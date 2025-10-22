/**
 * @file ui_renderers_internal.h
 * @brief Internal header for UI renderer modules
 *
 * This header provides shared declarations and state for UI rendering
 * functions across multiple renderer modules.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef UI_RENDERERS_INTERNAL_H
#define UI_RENDERERS_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "data_model.h"
#include "ui_state.h"

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

// =============================================================================
// Helper Functions (defined in ui_helpers.c)
// =============================================================================

void init_free_press_mode(void);
void init_job_press_mode(void);
void perform_job_stats_reset(void);
void perform_all_stats_reset(void);

// =============================================================================
// Common Display Helpers
// =============================================================================

/**
 * @brief Display an empty line for spacing
 * @param line Line number (0-7 for 64px display)
 */
static inline void display_empty_line(uint8_t line)
{
    display_text(0, line, "");
}

#endif // UI_RENDERERS_INTERNAL_H
