# PID Auto-Tune Guide

## Overview

The PID auto-tuning module automatically calculates optimal Kp, Ki, and Kd parameters for your heat press temperature control system. It uses the **Relay Feedback Method** (Åström-Hägglund), which is industry-standard for temperature control applications.

---

## How It Works

### The Relay Feedback Method

1. **Applies relay control**: Switches heating between high/low output
2. **Observes oscillation**: Measures how the temperature responds
3. **Calculates characteristics**:
   - Ultimate Gain (Ku): How much the system oscillates
   - Ultimate Period (Tu): How fast it oscillates
4. **Computes PID parameters**: Uses proven tuning rules to calculate Kp, Ki, Kd

### Why This Method?

✅ **Reliable**: Industry-proven for 30+ years
✅ **Safe**: Non-destructive, controlled oscillation
✅ **Accurate**: Works well for temperature systems
✅ **Fast**: Typically completes in 5-10 minutes
✅ **Automatic**: No manual intervention required

---

## Quick Start

### Basic Usage

```c
#include "pid_autotune.h"
#include "pid_controller.h"

// 1. Create auto-tune context
autotune_context_t autotune;

// 2. Get default configuration
autotune_config_t config = pid_autotune_default_config(140.0f); // 140°C target

// 3. Initialize with tuning rule
pid_autotune_init(&autotune, config, TUNING_RULE_TYREUS_LUYBEN);

// 4. Start auto-tuning
pid_autotune_start(&autotune);

// 5. In your control loop (every 1 second):
float output = pid_autotune_update(&autotune, current_temperature);
heating_set_power((uint8_t)output);

// 6. Check if complete
if (pid_autotune_is_complete(&autotune)) {
    autotune_result_t result;
    if (pid_autotune_get_result(&autotune, &result)) {
        ESP_LOGI(TAG, "Auto-tune successful!");
        ESP_LOGI(TAG, "Kp=%.3f, Ki=%.3f, Kd=%.3f",
                 result.kp, result.ki, result.kd);

        // Apply to PID controller
        pid_autotune_apply_result(&autotune, &my_pid_controller);
    }
}
```

---

## Configuration Options

### Tuning Rules

Choose based on your desired system behavior:

| Rule | Overshoot | Settling Time | Use Case |
|------|-----------|---------------|----------|
| `TUNING_RULE_TYREUS_LUYBEN` | ✅ Low | Medium | **Recommended for heat press** |
| `TUNING_RULE_ZIEGLER_NICHOLS_NO_OVERSHOOT` | ✅ None | Slow | Conservative, safe |
| `TUNING_RULE_ZIEGLER_NICHOLS_CLASSIC` | ⚠️ Some | Fast | Faster response |
| `TUNING_RULE_ZIEGLER_NICHOLS_PESSEN` | ⚠️ More | Faster | Aggressive |
| `TUNING_RULE_ZIEGLER_NICHOLS_SOME_OVERSHOOT` | ⚠️ Some | Medium | Balanced |

**For heat press, use `TUNING_RULE_TYREUS_LUYBEN`** - it's conservative and prevents temperature overshoot.

### Advanced Configuration

```c
autotune_config_t config = {
    .setpoint = 140.0f,          // Target temperature (°C)
    .output_step = 50.0f,        // Relay step size (% of max power)
    .noise_band = 0.5f,          // Temperature noise tolerance (°C)
    .max_cycles = 10,            // Oscillations to observe (more = better)
    .timeout_seconds = 600,      // Safety timeout (10 minutes)
    .initial_output = 20.0f,     // Starting power level (%)
};
```

**Parameter Guidelines:**

- **`output_step`**: 30-50% works well for most systems
  - Too small: Takes longer, less accurate
  - Too large: May overshoot dangerously

- **`noise_band`**: 0.5-1.0°C is typical
  - Too small: False peak detection
  - Too large: Inaccurate measurements

- **`max_cycles`**: 8-12 cycles recommended
  - More cycles = more accurate but slower
  - Fewer cycles = faster but less reliable

- **`timeout_seconds`**: 600s (10 min) is safe
  - Heat press should oscillate within this time
  - Increase if your system is very slow

---

## Integration with Main Application

### Step 1: Add to main.c

```c
#include "pid_autotune.h"

// Global auto-tune context
static autotune_context_t g_autotune_ctx;
static bool autotune_active = false;

// Function to start auto-tune
void start_autotune(float target_temp) {
    autotune_config_t config = pid_autotune_default_config(target_temp);
    pid_autotune_init(&g_autotune_ctx, config, TUNING_RULE_TYREUS_LUYBEN);

    if (pid_autotune_start(&g_autotune_ctx)) {
        autotune_active = true;
        ESP_LOGI(TAG, "Auto-tune started for %.1f°C", target_temp);
    }
}
```

### Step 2: Update Temperature Control Task

```c
void temp_control_task(void *pvParameters) {
    while (1) {
        float current_temp;
        if (read_temperature_safe(&current_temp)) {

            // Check if auto-tuning
            if (autotune_active) {
                float output = pid_autotune_update(&g_autotune_ctx, current_temp);
                heating_set_power((uint8_t)output);

                // Check completion
                if (pid_autotune_is_complete(&g_autotune_ctx)) {
                    autotune_result_t result;
                    if (pid_autotune_get_result(&g_autotune_ctx, &result)) {
                        // Success! Apply to PID
                        settings.pid_kp = result.kp;
                        settings.pid_ki = result.ki;
                        settings.pid_kd = result.kd;

                        // Save to persistent storage
                        storage_save_settings(&settings);

                        ESP_LOGI(TAG, "Auto-tune complete: Kp=%.3f Ki=%.3f Kd=%.3f",
                                 result.kp, result.ki, result.kd);
                    } else {
                        ESP_LOGE(TAG, "Auto-tune failed");
                    }
                    autotune_active = false;
                }
            } else {
                // Normal PID control
                float output = pid_update(current_temp);
                // ... rest of normal control
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### Step 3: Add UI State (Optional)

Add to `ui_state.h`:
```c
typedef enum {
    UI_STATE_INIT,
    UI_STATE_MAIN_MENU,
    UI_STATE_AUTOTUNE,  // NEW
    // ... rest of states
} ui_state_t;
```

Add to menu in `ui_state.c`:
```c
static const char *settings_menu_items[] = {
    "Target Temp",
    "Auto-Tune PID",  // NEW
    "PID Kp",
    "PID Ki",
    // ...
};
```

---

## Safety Considerations

### Before Starting Auto-Tune

✅ **Verify system is safe**
  - No emergency shutdown active
  - Temperature sensor working
  - Heating element functional
  - Adequate ventilation

✅ **Set appropriate limits**
  - Use timeout to prevent runaway
  - Monitor during first run
  - Have emergency stop ready

✅ **Choose safe parameters**
  - Don't exceed maximum safe temperature
  - Use conservative tuning rule initially
  - Start with moderate output_step

### During Auto-Tune

⚠️ **The system will oscillate** - This is normal!
  - Temperature will cycle above/below setpoint
  - Oscillation amplitude typically ±2-5°C
  - Takes 5-15 minutes depending on system

⚠️ **Monitor the process**
  - Watch for excessive overshoot
  - Cancel if temperature exceeds safe limits
  - Ensure heating cycles on/off properly

### After Auto-Tune

✅ **Verify results make sense**
  - Kp should be positive (typically 1-10)
  - Ki should be small (typically 0.01-1.0)
  - Kd should be small (typically 0.01-1.0)

✅ **Test the tuned controller**
  - Start with conservative settings
  - Monitor first few cycles
  - Adjust if needed

---

## Troubleshooting

### Auto-Tune Times Out

**Cause**: System not oscillating or too slow

**Solutions**:
- Increase `output_step` (try 60-70%)
- Reduce `noise_band` (try 0.3°C)
- Increase `timeout_seconds`
- Check heating element is working

### Auto-Tune Fails (Invalid Results)

**Cause**: Insufficient or irregular oscillation

**Solutions**:
- Increase `max_cycles` (try 15)
- Adjust `output_step` (try different values)
- Check for external disturbances
- Verify temperature sensor accuracy

### Results Give Poor Control

**Cause**: Wrong tuning rule or system mismatch

**Solutions**:
- Try different tuning rule (start conservative)
- Manually adjust results (multiply Kp by 0.5-2.0)
- Re-run auto-tune with different config
- Check system for mechanical issues

### Temperature Oscillates Too Much

**Cause**: PID parameters too aggressive

**Solutions**:
- Use `TUNING_RULE_TYREUS_LUYBEN` or `NO_OVERSHOOT`
- Reduce Kp by 50%
- Increase Kd slightly
- Re-run auto-tune

---

## Example: Complete Auto-Tune Sequence

```c
void run_complete_autotune(void) {
    ESP_LOGI(TAG, "Starting complete auto-tune sequence");

    // 1. Configure
    autotune_config_t config = pid_autotune_default_config(140.0f);
    config.max_cycles = 12;  // Extra cycles for accuracy

    // 2. Initialize
    autotune_context_t ctx;
    pid_autotune_init(&ctx, config, TUNING_RULE_TYREUS_LUYBEN);

    // 3. Start
    if (!pid_autotune_start(&ctx)) {
        ESP_LOGE(TAG, "Failed to start auto-tune");
        return;
    }

    // 4. Run loop
    while (!pid_autotune_is_complete(&ctx)) {
        // Read temperature
        float temp;
        if (!sensor_read_temperature(&temp)) {
            ESP_LOGE(TAG, "Sensor read failed during auto-tune");
            pid_autotune_cancel(&ctx);
            return;
        }

        // Update auto-tune
        float output = pid_autotune_update(&ctx, temp);
        heating_set_power((uint8_t)output);

        // Display progress
        uint8_t progress = pid_autotune_get_progress(&ctx);
        ESP_LOGI(TAG, "Auto-tune progress: %d%%, temp=%.1f°C, output=%.1f%%",
                 progress, temp, output);

        // Safety check
        if (temp > 200.0f) {
            ESP_LOGE(TAG, "Temperature too high! Aborting auto-tune");
            pid_autotune_cancel(&ctx);
            heating_emergency_shutoff();
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // 5. Get results
    autotune_result_t result;
    if (pid_autotune_get_result(&ctx, &result)) {
        ESP_LOGI(TAG, "=== Auto-Tune Results ===");
        ESP_LOGI(TAG, "Kp: %.4f", result.kp);
        ESP_LOGI(TAG, "Ki: %.4f", result.ki);
        ESP_LOGI(TAG, "Kd: %.4f", result.kd);
        ESP_LOGI(TAG, "Ultimate Gain: %.4f", result.ultimate_gain);
        ESP_LOGI(TAG, "Ultimate Period: %.1f sec", result.ultimate_period);
        ESP_LOGI(TAG, "Cycles Observed: %lu", result.cycles_observed);

        // 6. Apply and save
        settings.pid_kp = result.kp;
        settings.pid_ki = result.ki;
        settings.pid_kd = result.kd;
        storage_save_settings(&settings);

        ESP_LOGI(TAG, "Auto-tune complete and saved!");
    } else {
        ESP_LOGE(TAG, "Auto-tune failed to produce results");
    }

    // 7. Cleanup
    heating_set_power(0);
}
```

---

## Theory: How the Math Works

### Ultimate Gain Calculation

The relay feedback method determines the **ultimate gain** (Ku) - the gain at which the system becomes marginally stable:

```
Ku = (4 * d) / (π * a)
```

Where:
- `d` = relay output amplitude (your `output_step`)
- `a` = measured oscillation amplitude
- `π` = pi (3.14159...)

### Ziegler-Nichols Tuning Rules

Once we have Ku and Tu (ultimate period), we apply tuning rules:

**Classic Z-N:**
```
Kp = 0.6 * Ku
Ki = Kp / (Tu / 2) = 1.2 * Ku / Tu
Kd = Kp * (Tu / 8) = 0.075 * Ku * Tu
```

**Tyreus-Luyben (Recommended):**
```
Kp = 0.45 * Ku
Ki = Kp / (Tu / 2.2) = 0.99 * Ku / Tu
Kd = Kp * (Tu / 6.7) = 0.067 * Ku * Tu
```

These formulas are battle-tested and work well for most systems!

---

## Performance Expectations

### Typical Auto-Tune Duration

- **Fast systems** (electric heating): 3-5 minutes
- **Heat press** (moderate thermal mass): 5-10 minutes
- **Slow systems** (large thermal mass): 10-20 minutes

### Expected Results

For a heat press at 140°C, typical results might be:

```
Kp: 2.0 - 5.0
Ki: 0.05 - 0.2
Kd: 0.03 - 0.15
Ultimate Period: 30 - 60 seconds
```

Your actual results will vary based on your specific hardware!

---

## FAQ

**Q: How often should I run auto-tune?**
A: Once initially, then re-tune if you change hardware or notice poor control.

**Q: Can I run auto-tune while printing?**
A: No! Auto-tune causes temperature oscillation. Run it during setup only.

**Q: What if auto-tune gives weird results?**
A: Try a different tuning rule or adjust config parameters. You can also manually tweak results.

**Q: Is auto-tune safe?**
A: Yes, with proper safety limits. Always monitor the first run.

**Q: Can I cancel mid-tune?**
A: Yes! Call `pid_autotune_cancel(&ctx)` anytime.

**Q: Do I need to save results?**
A: Yes! Auto-tune doesn't save automatically. Update `settings` and call `storage_save_settings()`.

---

## Additional Resources

- [Åström-Hägglund Paper (1984)](https://www.sciencedirect.com/science/article/pii/0005109884900299)
- [Ziegler-Nichols Method](https://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method)
- [PID Tuning Guide](https://en.wikipedia.org/wiki/PID_controller#Loop_tuning)

---

**Created:** 2025-10-01
**Version:** 1.0.0
**Module:** `pid/pid_autotune.{h,c}`
