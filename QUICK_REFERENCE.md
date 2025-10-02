# Quick Reference Card - Refactored Code

## 📁 New File Structure

```
main/
├── pid/                          # PID Controller Module
│   ├── pid_controller.h         # PID interface
│   └── pid_controller.c         # PID implementation
├── utils/                        # Utility Modules
│   ├── system_config.h          # All constants & config
│   ├── application_state.h      # State structure
│   └── application_state.c      # State implementation
├── main.c                        # Main application
├── ui_state.c                    # Refactored UI (state pattern)
└── data_model.c                  # Data validation
```

## 🔧 Using New Modules

### System Configuration

```c
#include "system_config.h"

// Access configuration
SYSTEM_CONFIG.safety.max_temperature_celsius
SYSTEM_CONFIG.timing.sensor_timeout_sec
SYSTEM_CONFIG.temperature.hysteresis_celsius

// Use helper macros
int value = CLAMP(input, min, max);
int index = MENU_WRAP(idx + 1, count);
size_t size = ARRAY_SIZE(my_array);
```

### PID Controller

```c
#include "pid_controller.h"

// Create and initialize
pid_controller_t my_pid;
pid_config_t config = {
    .kp = 2.0f, .ki = 0.1f, .kd = 0.05f,
    .setpoint = 140.0f,
    .output_min = 0.0f, .output_max = 100.0f
};
pid_controller_init(&my_pid, config);

// Update (call periodically)
float output = pid_controller_update(&my_pid, current_temp);

// Change setpoint
pid_controller_set_setpoint(&my_pid, new_temp, true);

// Reset controller
pid_controller_reset(&my_pid);
```

### Application State (Optional)

```c
#include "application_state.h"

// Initialize at startup
app_state_init();

// Get state pointer
application_state_t *state = app_state_get();

// Access state
state->temperature.current_celsius
state->safety.is_system_paused
state->pressing.is_active

// Or use convenience accessors
float temp = app_state_get_current_temp();
bool paused = app_state_is_paused();
```

## 📋 Naming Conventions

### Booleans
```c
✅ is_system_paused
✅ was_press_closed
✅ is_pressing_active
✅ is_emergency_shutdown
❌ pause_mode
❌ pressing_active
❌ emergency_shutdown
```

### Units in Names
```c
✅ temperature_current_celsius
✅ timeout_seconds
✅ heap_minimum_bytes
❌ temperature
❌ timeout
❌ heap_min
```

### Descriptive Names
```c
✅ sensor_consecutive_errors
✅ job_setup_selected_index
✅ ui_adjustment_value
❌ error_count
❌ selected_job
❌ current_value
```

## 🎯 Common Tasks

### Add a New System Constant

**Edit:** `main/utils/system_config.h`

```c
static const system_config_t SYSTEM_CONFIG = {
    .safety = {
        .max_temperature_celsius = 220.0f,
        .my_new_limit = 42.0f,  // Add here
    },
    // ...
};
```

### Add a New UI State

**Edit:** `main/ui_state.c`

1. Add enum to `ui_state.h`:
```c
typedef enum {
    UI_STATE_INIT,
    UI_STATE_MAIN_MENU,
    UI_STATE_MY_NEW_STATE,  // Add here
    // ...
} ui_state_t;
```

2. Add handler and renderer:
```c
static void handle_my_new_state(ui_event_t event) {
    // Handle events
}

static void render_my_new_state(void) {
    // Render display
}
```

3. Add to state table:
```c
static const state_handler_entry_t state_handlers[] = {
    {UI_STATE_MY_NEW_STATE, handle_my_new_state, render_my_new_state, "My State"},
    // ...
};
```

### Add LED Indicator

**Use:** Controls contract

```c
#include "controls_contract.h"

// Turn on/off LEDs
controls_set_led_green(true);   // Temperature ready
controls_set_led_blue(true);    // Pause mode

controls_set_led_green(false);  // Turn off
```

### Adjust PID Parameters

```c
// At runtime
pid_controller_t *pid = &g_pid_controller;  // Get global instance
pid->config.kp = new_kp;
pid->config.ki = new_ki;
pid->config.kd = new_kd;

// Or reinitialize
pid_config_t new_config = { /* ... */ };
pid_controller_init(pid, new_config);
```

## 🔍 Finding Things

### Where is...?

**PID controller code?**
- Interface: `main/pid/pid_controller.h`
- Implementation: `main/pid/pid_controller.c`
- Usage: `components/heating/heating.c`

**System constants?**
- All in: `main/utils/system_config.h`

**Global state variables?**
- Definition: `main/utils/application_state.h`
- Implementation: `main/utils/application_state.c`
- Legacy: `main/main.c` (still has old globals)

**UI state machine?**
- `main/ui_state.c` (refactored)
- Backup: `main/ui_state.c.backup`

**Helper macros?**
- `main/utils/system_config.h`:
  - `CLAMP()`
  - `MENU_WRAP()`
  - `ARRAY_SIZE()`

## 🐛 Debugging Tips

### Check State

```c
// Log current state
ESP_LOGI(TAG, "State: pressing=%d, paused=%d, shutdown=%d",
         state->pressing.is_active,
         state->safety.is_system_paused,
         state->safety.is_emergency_shutdown);
```

### Check PID

```c
// PID already logs in DEBUG level
// Set log level to see PID updates
esp_log_level_set("pid_controller", ESP_LOG_DEBUG);
```

### Check UI State

```c
// UI logs state transitions
ESP_LOGI(TAG, "State transition: %d -> %d", old_state, new_state);
```

## 📊 Performance

### PID Update Rate

```c
// Minimum update interval (prevents jitter)
#define PID_MIN_UPDATE_INTERVAL_MS 100  // system_config.h

// Updates faster than this return cached output
```

### Task Monitoring

```c
// Watchdog timeouts
SYSTEM_CONFIG.timing.ui_task_timeout_sec       // 1 second
SYSTEM_CONFIG.timing.temp_task_timeout_sec     // 3 seconds
```

## ✅ Compilation

### Build Project

```bash
# Source ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Build
cd /home/mrksnktr/git/gpt5mini
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

### Clean Build

```bash
rm -rf build
idf.py fullclean
idf.py build
```

## 📚 Documentation

- **REFACTORING_SUMMARY.md** - Complete refactoring overview
- **MIGRATION_GUIDE.md** - Step-by-step migration instructions
- **This file** - Quick reference

## 🔗 Links

| Component | Header | Implementation |
|-----------|--------|----------------|
| PID | `pid/pid_controller.h` | `pid/pid_controller.c` |
| Config | `utils/system_config.h` | N/A (header-only) |
| State | `utils/application_state.h` | `utils/application_state.c` |
| UI | `ui_state.h` | `ui_state.c` |
| Heating | `include/heating_contract.h` | `components/heating/heating.c` |

## 💡 Tips

1. **Use constants from system_config.h** instead of hardcoding
2. **Use helper macros** (CLAMP, MENU_WRAP) for clarity
3. **Follow naming conventions** for consistency
4. **Log important state changes** for debugging
5. **Comment complex logic** for maintenance

---

**Version:** 2.0.0
**Last Updated:** 2025-10-01
