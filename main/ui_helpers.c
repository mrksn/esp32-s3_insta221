/**
 * @file ui_helpers.c
 * @brief UI state machine helper functions
 *
 * Contains utility functions for initializing modes, resetting statistics,
 * and other common UI state machine operations.
 */

#include "ui_state.h"
#include "main.h"
#include "data_model.h"
#include "application_state.h"
#include "display_contract.h"
#include "heating_contract.h"
#include "controls_contract.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

static const char *TAG = "ui_helpers";

// =============================================================================
// External State Variables (defined in ui_state.c)
// =============================================================================

extern bool free_press_mode;
extern uint16_t free_press_count;
extern uint32_t free_press_time_elapsed;
extern uint32_t free_press_avg_time;
extern uint32_t free_press_run_start_time;

extern ui_state_t ui_current_state;
extern float temperature_display_celsius;
extern bool display_needs_update;

extern uint32_t heat_up_start_time;
extern float heat_up_start_temp;
extern ui_state_t heat_up_return_state;
extern bool heat_up_heating_was_active;
extern uint32_t heat_up_last_update_sec;
extern bool heat_up_screen_initialized;

extern print_run_t *current_run;

// =============================================================================
// Mode Initialization Functions
// =============================================================================

/**
 * @brief Initialize free press mode
 *
 * Sets up free_press_mode flag and resets all free press statistics.
 * Used when entering freestyle pressing without job tracking.
 */
void init_free_press_mode(void)
{
    free_press_mode = true;
    free_press_count = 0;
    free_press_time_elapsed = 0;
    free_press_avg_time = 0;
    free_press_run_start_time = 0;
    ESP_LOGI(TAG, "Free press mode initialized, statistics reset");
}

/**
 * @brief Initialize job press mode
 *
 * Clears free_press_mode flag to enable job-tracked pressing.
 */
void init_job_press_mode(void)
{
    free_press_mode = false;
    ESP_LOGI(TAG, "Job press mode initialized");
}

/**
 * @brief Enter heat-up state with specified return destination
 *
 * @param return_to State to transition to when heat-up completes
 */
void enter_heat_up_mode(ui_state_t return_to)
{
    ui_current_state = UI_STATE_HEAT_UP;
    heat_up_start_time = esp_timer_get_time() / 1000000;
    heat_up_start_temp = temperature_display_celsius;
    heat_up_return_state = return_to;

    // Reset render state for clean display
    heat_up_heating_was_active = false;
    heat_up_last_update_sec = 0;
    heat_up_screen_initialized = false;

    display_needs_update = true;
    ESP_LOGI(TAG, "Entering heat-up mode, will return to state: %d", return_to);
}

// =============================================================================
// Statistics Reset Functions
// =============================================================================

/**
 * @brief Reset free press statistics
 *
 * Clears all counters and timers related to free press mode.
 */
void reset_free_press_stats(void)
{
    free_press_count = 0;
    free_press_time_elapsed = 0;
    free_press_avg_time = 0;
    free_press_run_start_time = 0;
}

/**
 * @brief Reset current print run statistics
 *
 * Clears progress, time, and completion counters for the active job.
 */
void reset_print_run_stats(void)
{
    if (current_run != NULL)
    {
        current_run->progress = 0;
        current_run->time_elapsed = 0;
        current_run->shirts_completed = 0;
        current_run->avg_time_per_shirt = 0;
    }
}

/**
 * @brief Perform job statistics reset
 *
 * Resets both free press and print run statistics, saves to persistent storage,
 * and displays confirmation message.
 */
void perform_job_stats_reset(void)
{
    ESP_LOGI(TAG, "Resetting job and free press statistics");

    reset_free_press_stats();
    reset_print_run_stats();
    save_persistent_data();

    display_clear();
    display_text(0, 1, "Job Stats Reset!");
    display_flush();
}

/**
 * @brief Perform all statistics reset
 *
 * Wipes all statistics including session stats, free press stats, and print run stats.
 * Saves to persistent storage and displays confirmation message.
 */
void perform_all_stats_reset(void)
{
    ESP_LOGI(TAG, "Wiping all statistics");

    // Reset statistics via main.h function (thread-safe)
    reset_all_statistics();

    reset_free_press_stats();
    reset_print_run_stats();
    save_persistent_data();

    display_clear();
    display_text(0, 1, "All Stats Wiped!");
    display_flush();
}
