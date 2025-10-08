# Include Hierarchy Documentation

## Overview

This project follows ESP-IDF component best practices for include file organization.

## Directory Structure

```
esp32-s3_insta221/
├── components/                    # Hardware abstraction components
│   ├── controls/
│   │   ├── include/              # PUBLIC: Component API
│   │   │   └── controls_contract.h
│   │   └── controls.c            # PRIVATE: Implementation
│   ├── display/
│   │   ├── include/              # PUBLIC: Component API
│   │   │   └── display_contract.h
│   │   └── display.c             # PRIVATE: Implementation
│   ├── heating/
│   │   ├── include/              # PUBLIC: Component API
│   │   │   └── heating_contract.h
│   │   └── heating.c             # PRIVATE: Implementation
│   ├── sensors/
│   │   ├── include/              # PUBLIC: Component API
│   │   │   └── sensor_contract.h
│   │   └── sensors.c             # PRIVATE: Implementation
│   └── storage/
│       ├── include/              # PUBLIC: Component API
│       │   ├── storage_contract.h
│       │   └── data_model.h
│       └── storage.c             # PRIVATE: Implementation
│
└── main/                         # Main application component
    ├── include/                  # PUBLIC: Main component API
    │   ├── main.h               # Main application interface
    │   ├── system_constants.h   # System-wide constants
    │   └── ui_state.h           # UI state machine interface
    ├── pid/                      # PRIVATE: PID implementation
    │   ├── pid_controller.h
    │   ├── pid_controller.c
    │   ├── pid_autotune.h
    │   └── pid_autotune.c
    ├── utils/                    # PRIVATE: Utility modules
    │   ├── system_config.h
    │   ├── system_config.c
    │   ├── application_state.h
    │   └── application_state.c
    ├── main.c                    # PRIVATE: Main implementation
    ├── ui_state.c                # PRIVATE: UI implementation
    └── data_model.c              # PRIVATE: Data model implementation
```

## Include Rules

### 1. Component Public APIs

**Location:** `components/*/include/*.h`

**Purpose:** Define the contract/interface that other components can use.

**Examples:**
- `sensor_contract.h` - Temperature sensor interface
- `display_contract.h` - Display control interface
- `heating_contract.h` - Heating control interface
- `controls_contract.h` - Input controls interface
- `storage_contract.h` - Data persistence interface
- `data_model.h` - Shared data structures

**How to include:**
```c
#include "sensor_contract.h"      // Automatic - ESP-IDF adds component includes
#include "display_contract.h"
#include "data_model.h"
```

### 2. Main Component Public Headers

**Location:** `main/include/*.h`

**Purpose:** Public interfaces for the main application component.

**Files:**
- `main.h` - Main application safety and state functions
- `system_constants.h` - System-wide constants shared across components
- `ui_state.h` - UI state machine interface

**How to include:**
```c
#include "main.h"
#include "system_constants.h"
#include "ui_state.h"
```

### 3. Private Implementation Headers

**Location:**
- `main/pid/*.h` - PID controller implementation
- `main/utils/*.h` - Internal utilities

**Purpose:** Internal implementation details not exposed to other components.

**How to include (only within main component):**
```c
#include "pid_controller.h"     // From main/pid/
#include "pid_autotune.h"       // From main/pid/
#include "system_config.h"      // From main/utils/
#include "application_state.h"  // From main/utils/
```

## CMakeLists.txt Configuration

### Component Example (e.g., sensors)

```cmake
idf_component_register(
    SRCS "sensors.c"
    INCLUDE_DIRS "include"        # PUBLIC: Other components can access
    REQUIRES ...                   # Dependencies
)
```

### Main Component

```cmake
idf_component_register(
    SRCS "main.c" "ui_state.c" ...
    INCLUDE_DIRS "include"         # PUBLIC: Exported headers
    PRIV_INCLUDE_DIRS              # PRIVATE: Internal headers
        "."                        # For .c files to find each other
        "pid"                      # PID implementation
        "utils"                    # Utilities
    REQUIRES
        display
        sensors
        controls
        heating
        storage
)
```

## Benefits of This Structure

1. **Clear Separation**: Public APIs vs private implementation
2. **Compile-time Enforcement**: ESP-IDF build system enforces component boundaries
3. **Maintainability**: Easy to see what's exposed vs internal
4. **Modularity**: Components can be reused or replaced
5. **Documentation**: File location indicates its purpose
6. **Safety**: Private headers can't accidentally be used by other components

## Migration Notes

**Previous Structure:**
- Headers were in `main/` root alongside source files
- No clear separation between public and private headers

**New Structure (Current):**
- Public headers → `main/include/`
- Private headers → `main/pid/`, `main/utils/`
- CMakeLists.txt updated with `PRIV_INCLUDE_DIRS`
- All components already following best practices in `components/*/include/`

## Quick Reference

**Want to expose a function to other components?**
→ Declare it in a header in `{component}/include/`

**Need an internal helper function?**
→ Declare it in a header in the component's root or subdirectory (e.g., `main/utils/`)

**Need system-wide constants?**
→ Use `system_constants.h` (in `main/include/`)

**Need data structures shared across components?**
→ Use `data_model.h` (in `components/storage/include/`)
