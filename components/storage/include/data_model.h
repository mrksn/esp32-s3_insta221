#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdint.h>
#include <stdbool.h>

// Enums
typedef enum {
    SINGLE_SIDED,
    DOUBLE_SIDED
} printing_type_t;

typedef enum {
    FRONT,
    BACK
} shirt_side_t;

typedef enum {
    IDLE,
    STAGE1,
    STAGE2,
    COMPLETE
} cycle_status_t;

// Structs
typedef struct {
    uint32_t id;
    uint16_t num_shirts;
    printing_type_t type;
    uint16_t progress; // current shirt number
    uint32_t time_elapsed; // seconds
    uint16_t shirts_completed;
    uint32_t avg_time_per_shirt; // seconds
} print_run_t;

typedef struct {
    uint16_t shirt_id;
    shirt_side_t side;
    uint16_t stage1_duration; // seconds
    uint16_t stage2_duration; // seconds
    uint32_t start_time; // timestamp
    cycle_status_t status;
} pressing_cycle_t;

typedef struct {
    float target_temp;
    float pid_kp;
    float pid_ki;
    float pid_kd;
    uint16_t stage1_default; // seconds
    uint16_t stage2_default; // seconds
} settings_t;

// Validation functions
bool validate_print_run(const print_run_t *run);
bool validate_pressing_cycle(const pressing_cycle_t *cycle);
bool validate_settings(const settings_t *settings);

#endif // DATA_MODEL_H