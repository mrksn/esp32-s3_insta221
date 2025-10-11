# System Configuration Component

## Overview

The `system_config` component provides centralized system configuration and constants for the Insta Retrofit heat press automation system. It serves as a single source of truth for all system constraints, safety limits, timing parameters, and temperature thresholds.

## Features

- **Centralized Configuration**: All system parameters in one place
- **Type-Safe Access**: Structured configuration with semantic grouping
- **Validation Functions**: Built-in validation for configuration integrity
- **Legacy Compatibility**: Macros for backward compatibility with existing code
- **Safety Critical**: Configuration parameters directly affect system safety

## Configuration Structure

The configuration is organized into logical groups:

### Safety Limits
- `max_temperature_celsius`: Maximum safe operating temperature (°C)
- `max_cycle_time_seconds`: Maximum pressing cycle time (s)
- `heap_minimum_bytes`: Minimum free heap memory (bytes)

### Timing Parameters
- `ui_task_timeout_sec`: UI task watchdog timeout (s)
- `temp_task_timeout_sec`: Temperature control task watchdog timeout (s)
- `sensor_timeout_sec`: Maximum time without sensor reading (s)
- `sensor_validation_timeout_sec`: Sensor timeout for cycle validation (s)

### Temperature Thresholds
- `hysteresis_celsius`: Temperature hysteresis for PID control (°C)
- `pressing_max_offset_celsius`: Max temp offset during pressing (°C)
- `cycle_start_max_offset_celsius`: Max temp offset for cycle start (°C)
- `cycle_start_min_celsius`: Minimum temperature for cycle start (°C)
- `recovery_offset_celsius`: Temperature offset for error recovery (°C)
- `min_for_pressing_celsius`: Minimum temperature to allow pressing (°C)
- `ready_threshold_celsius`: Temperature threshold to consider heat-up complete (°C)

### Sensor Configuration
- `retry_count`: Number of sensor read retry attempts
- `retry_delay_ms`: Delay between retry attempts (ms)

### Heat-up Display
- `min_temp_change_celsius`: Minimum temp change before calculating ETA (°C)
- `min_elapsed_time_sec`: Minimum elapsed time before calculating ETA (s)
- `min_heating_rate`: Minimum heating rate for valid ETA (°C/s)

### Simulation Mode
- `enabled`: Enable simulation mode for testing

## Usage

### Accessing Configuration

```c
#include "system_config.h"

// Access configuration directly
float max_temp = SYSTEM_CONFIG.safety.max_temperature_celsius;
uint32_t timeout = SYSTEM_CONFIG.timing.sensor_timeout_sec;

// Or use legacy compatibility macros
if (temperature > MAX_TEMPERATURE) {
    // Emergency shutdown
}
```

### Validating Configuration

```c
#include "system_config.h"

void app_main(void) {
    // Validate configuration at startup
    if (!system_config_validate()) {
        ESP_LOGE(TAG, "Invalid configuration: %s",
                 system_config_get_error());
        return;
    }

    // Print configuration for debugging
    system_config_print();
}
```

## Modifying Configuration

To change system parameters:

1. Edit `system_config.c`
2. Modify the `SYSTEM_CONFIG` structure
3. Rebuild the project
4. Test thoroughly - these values affect system safety!

**SAFETY WARNING**: Changes to safety limits and temperature thresholds directly affect system safety. All modifications must be reviewed and tested carefully.

## Legacy Compatibility

For backward compatibility, the following macros are provided:

```c
// Task Configuration
#define UI_TASK_STACK_SIZE 4096
#define TEMP_CONTROL_TASK_STACK_SIZE 4096
#define WATCHDOG_TASK_STACK_SIZE 2048
#define UI_TASK_PRIORITY 5
#define TEMP_CONTROL_TASK_PRIORITY 4
#define WATCHDOG_TASK_PRIORITY 3

// Safety Limits
#define MAX_TEMPERATURE (SYSTEM_CONFIG.safety.max_temperature_celsius)
#define MAX_CYCLE_TIME (SYSTEM_CONFIG.safety.max_cycle_time_seconds)
#define HEAP_MINIMUM (SYSTEM_CONFIG.safety.heap_minimum_bytes)

// And more...
```

These macros map to the structured configuration for gradual migration.

## Migration from system_constants.h

The old `system_constants.h` has been replaced by this component. Key changes:

1. **Location**: Moved from `main/include/` to `components/system_config/`
2. **Structure**: Constants organized into semantic groups
3. **Validation**: Added validation functions
4. **Flexibility**: Easier to extend and modify

### Migration Steps

1. Replace `#include "system_constants.h"` with `#include "system_config.h"`
2. Add `system_config` to component REQUIRES in CMakeLists.txt
3. Use `SYSTEM_CONFIG.group.parameter` for new code
4. Legacy macros continue to work for existing code

## API Reference

### Functions

#### `bool system_config_validate(void)`
Validates that all configuration values are within acceptable ranges.

**Returns**: `true` if configuration is valid, `false` otherwise

#### `const char* system_config_get_error(void)`
Gets the error message from the last validation failure.

**Returns**: Error message string, or `NULL` if no errors

#### `void system_config_print(void)`
Prints the current configuration to the ESP log for debugging.

## Dependencies

- ESP-IDF core components
- `esp_log` for logging

## Testing

Configuration validation should be tested at startup:

```c
void test_system_config(void) {
    assert(system_config_validate());
    assert(SYSTEM_CONFIG.safety.max_temperature_celsius > 0);
    assert(SYSTEM_CONFIG.safety.max_temperature_celsius <= 300);
    // Add more tests...
}
```

## Future Enhancements

- [ ] Runtime configuration modification (with validation)
- [ ] Configuration persistence to NVS
- [ ] Configuration profiles (development, production, testing)
- [ ] Configuration import/export via JSON
