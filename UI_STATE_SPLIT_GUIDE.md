# UI State Machine File Split - Implementation Guide

## Status: READY FOR IMPLEMENTATION

### What We've Created

✅ **[ui_state_internal.h](main/ui_state_internal.h)** - Shared internal header
✅ **[REFACTORING_PLAN.md](REFACTORING_PLAN.md)** - Complete strategy document
✅ **[extract_ui_code.sh](extract_ui_code.sh)** - Helper script for analysis

## Recommended Approach

### Option 1: Manual Extraction (RECOMMENDED - Most Control)

**Why Recommended:**
- Full control over what goes where
- Can verify each step
- Safer for production code
- Better understanding of code structure

**Steps:**

#### 1. Backup Current File
```bash
cp main/ui_state.c main/ui_state.c.original
```

#### 2. Create Helper File First (Smallest)
```bash
# Create main/ui_helpers.c
# Extract these functions (lines ~2350-2450):
# - init_free_press_mode()
# - init_job_press_mode()
# - enter_heat_up_mode()
# - reset_free_press_stats()
# - reset_print_run_stats()
# - perform_job_stats_reset()
# - perform_all_stats_reset()
```

**File Template:**
```c
#include "ui_state_internal.h"
#include "controls_contract.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ui_helpers";

// Paste helper functions here
```

#### 3. Create Renderers File (Largest)
```bash
# Create main/ui_renderers.c
# Extract ALL render_* functions (lines vary, ~30 functions)
```

**File Template:**
```c
#include "ui_state_internal.h"
#include "display_contract.h"
#include "system_constants.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "ui_renderers";

// Menu item arrays (copy from original)
static const char *main_menu_items[] = {...};
static const char *settings_menu_items[] = {...};
// ... etc

// Paste all render_* functions here
```

#### 4. Create Event Handlers File
```bash
# Create main/ui_event_handlers.c
# Extract ALL handle_* functions (~25 functions)
```

**File Template:**
```c
#include "ui_state_internal.h"
#include "controls_contract.h"
#include "heating_contract.h"
#include "main.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ui_handlers";

// Paste all handle_* functions here
```

#### 5. Create State Machine Core
```bash
# Rename and gut main/ui_state.c -> main/ui_state_machine.c
# Keep only:
# - Includes
# - State variable DEFINITIONS (not extern declarations)
# - State handler table
# - Public API functions (ui_init, ui_update, ui_process_event, etc.)
```

**File Template:**
```c
#include "ui_state_internal.h"
#include "controls_contract.h"
#include "display_contract.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ui_state";

// State variable DEFINITIONS
ui_state_t ui_current_state = UI_STATE_INIT;
bool display_needs_update = false;
settings_t *current_settings = NULL;
// ... all other variables

// State handler table
typedef struct {
    ui_state_t state;
    void (*handler)(ui_event_t);
    void (*renderer)(void);
    const char *name;
} state_handler_entry_t;

static const state_handler_entry_t state_handlers[] = {
    {UI_STATE_STARTUP, handle_startup_state, render_startup, "Startup"},
    // ... all states
};

// Public API implementation
void ui_init(...) { }
void ui_register_callbacks(...) { }
void ui_update(...) { }
// ... etc
```

#### 6. Update CMakeLists.txt
```cmake
idf_component_register(
    SRCS
        "main.c"
        "ui_state_machine.c"      # NEW NAME
        "ui_event_handlers.c"     # NEW
        "ui_renderers.c"          # NEW
        "ui_helpers.c"            # NEW
        "data_model.c"
        "utils/application_state.c"
        "utils/system_config.c"
        "pid/pid_controller.c"
        "pid/pid_autotune.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "."
        "utils"
        "pid"
    REQUIRES
        display
        sensors
        controls
        heating
        storage
)
```

#### 7. Build and Fix Issues
```bash
idf.py build
# Fix any missing declarations
# Add includes as needed
```

### Option 2: Automated Extraction (FASTER but needs review)

If you want to automate, I can provide Python scripts to:
1. Parse the C file
2. Extract functions by pattern
3. Generate the split files

**Trade-off:** Requires review and manual fixes, but saves typing.

## Key Points for Success

### 1. Include Chain
```
ui_state_machine.c  ─┐
ui_event_handlers.c ─┼─→ ui_state_internal.h → ui_state.h
ui_renderers.c      ─┤
ui_helpers.c        ─┘
```

### 2. Variable Definitions vs Declarations

**In ui_state_machine.c (DEFINITIONS):**
```c
// These DEFINE the variables (allocate memory)
ui_state_t ui_current_state = UI_STATE_INIT;
settings_t *current_settings = NULL;
```

**In ui_state_internal.h (DECLARATIONS):**
```c
// These DECLARE the variables exist elsewhere
extern ui_state_t ui_current_state;
extern settings_t *current_settings;
```

**In other files:**
```c
// Just include ui_state_internal.h and use the variables
#include "ui_state_internal.h"

void some_function(void) {
    if (ui_current_state == UI_STATE_MAIN_MENU) {
        // Use current_settings, etc.
    }
}
```

### 3. Common Pitfalls

❌ **Don't** define variables in multiple files
❌ **Don't** include ui_state_internal.h in ui_state.h (public header)
❌ **Don't** forget to add new .c files to CMakeLists.txt
✅ **Do** define variables once in ui_state_machine.c
✅ **Do** declare as extern in ui_state_internal.h
✅ **Do** include ui_state_internal.h in all implementation files

### 4. Function Extraction Order

1. **Helpers first** - No dependencies on handlers/renderers
2. **Renderers second** - May call helpers, no handlers
3. **Handlers third** - Call both renderers and helpers
4. **State machine last** - Calls handlers

### 5. Testing Strategy

After each file creation:
```bash
# Try to compile
idf.py build

# If errors, check:
# 1. Missing #includes
# 2. Undefined functions (forgot to extract?)
# 3. Multiple definitions (defined in >1 place?)
```

## Expected Outcome

### Before
```
main/ui_state.c                    2,707 lines  ❌ Too large
```

### After
```
main/ui_state_machine.c             ~350 lines  ✅ Core logic
main/ui_event_handlers.c            ~900 lines  ✅ Event handling
main/ui_renderers.c               ~1,200 lines  ✅ Display code
main/ui_helpers.c                   ~250 lines  ✅ Utilities
main/ui_state_internal.h            ~180 lines  ✅ Shared decls
```

## Quick Start Command Sequence

```bash
# 1. Backup
cp main/ui_state.c main/ui_state.c.backup

# 2. Create new files (templates above)
touch main/ui_helpers.c
touch main/ui_renderers.c
touch main/ui_event_handlers.c

# 3. Copy sections from ui_state.c to new files
# (Use your editor's copy/paste, search for function patterns)

# 4. Rename original
mv main/ui_state.c main/ui_state_machine.c

# 5. Remove extracted code from ui_state_machine.c
# (Keep only core state machine)

# 6. Update CMakeLists.txt

# 7. Build
idf.py build

# 8. Fix any errors
# (Usually missing includes or extern declarations)
```

## Need Help?

The files are structured to make manual extraction straightforward:
- Functions are grouped by prefix (render_, handle_, init_, etc.)
- Each function is self-contained
- ui_state_internal.h has all the glue code

If you encounter issues:
1. Check ui_state_internal.h has the extern declaration
2. Check ui_state_machine.c has the actual definition
3. Check all new .c files include ui_state_internal.h
4. Check CMakeLists.txt includes all new .c files

Good luck! This refactoring will make the codebase much more maintainable! 🚀
