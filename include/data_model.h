/**
 * @file data_model.h
 * @brief Data model definitions for Insta Retrofit heat press system
 *
 * This header defines the core data structures and types used throughout
 * the heat press automation system. It includes:
 * - Print run configuration and tracking
 * - Pressing cycle state management
 * - System settings and PID parameters
 * - Validation functions for data integrity
 *
 * All data structures are designed for efficient storage in NVS and
 * real-time operation in the embedded system.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Type Definitions
// =============================================================================

/**
 * @brief Printing type enumeration
 */
typedef enum
{
    SINGLE_SIDED, ///< Single-sided printing (one side per shirt)
    DOUBLE_SIDED  ///< Double-sided printing (two sides per shirt)
} printing_type_t;

/**
 * @brief Shirt side enumeration for double-sided printing
 */
typedef enum
{
    FRONT, ///< Front side of shirt
    BACK   ///< Back side of shirt
} shirt_side_t;

/**
 * @brief Pressing cycle status enumeration
 */
typedef enum
{
    IDLE,    ///< Cycle not started
    STAGE1,  ///< First pressing stage active
    STAGE2,  ///< Second pressing stage active
    COMPLETE ///< Cycle completed successfully
} cycle_status_t;

// =============================================================================
// Data Structures
// =============================================================================

/**
 * @brief Print run configuration and progress tracking
 *
 * Represents a complete printing session with multiple shirts.
 * Tracks overall progress, timing, and statistics.
 */
typedef struct
{
    uint32_t id;                 ///< Unique print run identifier
    uint16_t num_shirts;         ///< Total number of shirts in run (1-999)
    printing_type_t type;        ///< Single or double-sided printing
    uint16_t progress;           ///< Current shirt number being processed
    uint32_t time_elapsed;       ///< Total elapsed time in seconds
    uint16_t shirts_completed;   ///< Number of completed shirts
    uint32_t avg_time_per_shirt; ///< Average time per shirt in seconds
} print_run_t;

/**
 * @brief Individual pressing cycle state
 *
 * Represents a single pressing operation for one side of one shirt.
 * Contains timing configuration and current status.
 */
typedef struct
{
    uint16_t shirt_id;        ///< Which shirt this cycle is for
    shirt_side_t side;        ///< Which side (for double-sided printing)
    uint16_t stage1_duration; ///< Duration of first stage in seconds
    uint16_t stage2_duration; ///< Duration of second stage in seconds
    uint32_t start_time;      ///< Cycle start timestamp
    cycle_status_t status;    ///< Current cycle status
} pressing_cycle_t;

/**
 * @brief System settings and configuration
 *
 * Contains all configurable parameters for temperature control,
 * timing defaults, and PID controller tuning.
 */
typedef struct
{
    float target_temp;       ///< Target temperature in °C
    float pid_kp;            ///< PID proportional gain
    float pid_ki;            ///< PID integral gain
    float pid_kd;            ///< PID derivative gain
    uint16_t stage1_default; ///< Default stage 1 duration in seconds
    uint16_t stage2_default; ///< Default stage 2 duration in seconds
} settings_t;

/**
 * @brief Comprehensive statistics tracking
 *
 * Tracks detailed production, temperature, and event statistics
 * for analysis and optimization.
 */
typedef struct
{
    // Production statistics
    uint32_t total_presses;              ///< Total number of presses completed
    uint32_t total_operating_time;       ///< Total time system has been operating (seconds)
    uint32_t total_idle_time;            ///< Total time between presses (seconds)
    uint32_t session_start_time;         ///< When current session started (for calculations)

    // Temperature statistics
    uint32_t total_warmup_time;          ///< Cumulative warmup time (seconds)
    uint16_t warmup_count;               ///< Number of warmups tracked
    float avg_warmup_time;               ///< Average time to reach target temp (seconds)
    float temp_variance_sum;             ///< Sum of temperature variances (for std dev)
    uint16_t temp_samples;               ///< Number of temperature samples
    float avg_temp_drop;                 ///< Average temperature drop during pressing
    uint16_t presses_since_pid_tune;     ///< Presses since last PID tuning
    uint32_t ssr_switch_count;           ///< SSR on/off cycle count

    // Event tracking
    uint16_t aborted_cycles;             ///< Number of aborted/incomplete cycles
    uint16_t temp_faults;                ///< Temperature sensor fault count
    uint16_t early_releases;             ///< Press opened before timer complete
    uint16_t sensor_failures;            ///< Sensor read failure count
    uint16_t power_cycles;               ///< System restart/power cycle count
    uint16_t emergency_stops;            ///< Emergency stop activation count

    // KPI data (some calculated on-the-fly)
    uint16_t presses_in_tolerance;       ///< Presses completed within temp tolerance
} statistics_t;

// =============================================================================
// Validation Functions
// =============================================================================

/**
 * @brief Validate print run data structure
 *
 * Checks that all fields contain valid values and relationships are consistent.
 *
 * @param run Pointer to print_run_t structure to validate
 * @return true if valid, false if invalid
 */
bool validate_print_run(const print_run_t *run);

/**
 * @brief Validate pressing cycle data structure
 *
 * Checks that cycle parameters are within valid ranges and consistent.
 *
 * @param cycle Pointer to pressing_cycle_t structure to validate
 * @return true if valid, false if invalid
 */
bool validate_pressing_cycle(const pressing_cycle_t *cycle);

/**
 * @brief Validate settings data structure
 *
 * Checks that all configuration parameters are within safe and valid ranges.
 *
 * @param settings Pointer to settings_t structure to validate
 * @return true if valid, false if invalid
 */
bool validate_settings(const settings_t *settings);

#endif // DATA_MODEL_H
