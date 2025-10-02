#include "data_model.h"
#include <stdbool.h>

bool validate_print_run(const print_run_t *run)
{
    if (!run)
        return false;
    if (run->num_shirts == 0 || run->num_shirts > 999)
        return false;
    if (run->progress > run->num_shirts)
        return false;
    if (run->type != SINGLE_SIDED && run->type != DOUBLE_SIDED)
        return false;
    return true;
}

bool validate_pressing_cycle(const pressing_cycle_t *cycle)
{
    if (!cycle)
        return false;
    if (cycle->stage1_duration == 0 || cycle->stage2_duration == 0)
        return false;
    if (cycle->side != FRONT && cycle->side != BACK)
        return false;
    if (cycle->status < IDLE || cycle->status > COMPLETE)
        return false;
    return true;
}

bool validate_settings(const settings_t *settings)
{
    if (!settings)
        return false;
    if (settings->target_temp < 0.0f || settings->target_temp > 250.0f)
        return false;
    if (settings->pid_kp < 0.0f || settings->pid_ki < 0.0f || settings->pid_kd < 0.0f)
        return false;
    if (settings->stage1_default == 0 || settings->stage2_default == 0)
        return false;
    return true;
}
