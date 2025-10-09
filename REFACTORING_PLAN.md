# UI State Machine Refactoring Plan

## Current State
- **File**: `main/ui_state.c`
- **Size**: 2,707 lines
- **Functions**: ~142 functions
- **Problem**: Monolithic file, hard to maintain and navigate

## Proposed Structure

```
main/
├── ui_state_machine.c      # Core state machine (350 lines)
├── ui_event_handlers.c     # Event handling logic (900 lines)
├── ui_renderers.c          # Display rendering (1200 lines)
├── ui_helpers.c            # Helper/utility functions (250 lines)
├── ui_state_internal.h     # Shared internal declarations
└── include/
    └── ui_state.h          # Public API (existing)
```

## File Breakdown

### 1. ui_state_machine.c (Core State Machine)
**Responsibilities:**
- State machine infrastructure
- State variable definitions
- Public API implementation
- State handler dispatch table

**Functions:**
- `ui_init()` - Initialize UI
- `ui_register_callbacks()` - Register callbacks
- `ui_update()` - Update cycle
- `ui_get_event()` - Get input events
- `ui_process_event()` - Process events
- `ui_update_display()` - Update display
- `ui_get_current_state()` - Get state
- `ui_set_state()` - Set state
- Public getters/setters

**State Variables:**
- All extern variables declared in ui_state_internal.h
- Callback structure
- State handler table

### 2. ui_event_handlers.c (Event Handling)
**Responsibilities:**
- Handle user input for each state
- Navigate between states
- Update state-specific variables
- Trigger actions (save, reset, autotune, etc.)

**Functions** (~25 handlers):
```c
void handle_startup_state(ui_event_t event);
void handle_main_menu_state(ui_event_t event);
void handle_job_setup_state(ui_event_t event);
void handle_job_setup_adjust_state(ui_event_t event);
void handle_print_type_select_state(ui_event_t event);
void handle_settings_menu_state(ui_event_t event);
void handle_timers_menu_state(ui_event_t event);
void handle_timer_adjust_state(ui_event_t event);
void handle_temperature_menu_state(ui_event_t event);
void handle_temp_adjust_state(ui_event_t event);
void handle_pid_menu_state(ui_event_t event);
void handle_pid_adjust_state(ui_event_t event);
void handle_start_pressing_state(ui_event_t event);
void handle_free_press_state(ui_event_t event);
void handle_profiles_menu_state(ui_event_t event);
void handle_pressing_active_state(ui_event_t event);
void handle_stage1_done_state(ui_event_t event);
void handle_stage2_ready_state(ui_event_t event);
void handle_stage2_done_state(ui_event_t event);
void handle_cycle_complete_state(ui_event_t event);
void handle_statistics_state(ui_event_t event);
void handle_stats_production_state(ui_event_t event);
void handle_stats_temperature_state(ui_event_t event);
void handle_stats_events_state(ui_event_t event);
void handle_stats_kpis_state(ui_event_t event);
void handle_autotune_state(ui_event_t event);
void handle_autotune_complete_state(ui_event_t event);
void handle_reset_stats_state(ui_event_t event);
void handle_heat_up_state(ui_event_t event);
```

### 3. ui_renderers.c (Display Rendering)
**Responsibilities:**
- Render UI for each state
- Format display strings
- Call display driver functions
- Handle multi-line layouts

**Functions** (~29 renderers):
```c
void render_startup(void);
void render_main_menu(void);
void render_job_setup(void);
void render_job_setup_adjust(void);
void render_print_type_select(void);
void render_settings_menu(void);
void render_timers_menu(void);
void render_timer_adjust(void);
void render_temperature_menu(void);
void render_temp_adjust(void);
void render_pid_menu(void);
void render_pid_adjust(void);
void render_start_pressing(void);
void render_free_press(void);
void render_profiles_menu(void);
void render_pressing_active(void);
void render_stage1_done(void);
void render_stage2_ready(void);
void render_stage2_done(void);
void render_cycle_complete(void);
void render_statistics(void);
void render_stats_production(void);
void render_stats_temperature(void);
void render_stats_events(void);
void render_stats_kpis(void);
void render_autotune(void);
void render_autotune_complete(void);
void render_reset_stats(void);
void render_heat_up(void);
```

### 4. ui_helpers.c (Helper Functions)
**Responsibilities:**
- Initialize modes (free press, job, heat-up)
- Reset statistics
- Utility functions

**Functions**:
```c
void init_free_press_mode(void);
void init_job_press_mode(void);
void enter_heat_up_mode(ui_state_t return_to);
void reset_free_press_stats(void);
void reset_print_run_stats(void);
void perform_job_stats_reset(void);
void perform_all_stats_reset(void);
```

### 5. ui_state_internal.h (Shared Declarations)
**Responsibilities:**
- Declare all shared state variables (as extern)
- Declare all internal functions
- Define internal types and constants
- NOT included by external code

**Contents:**
- State variable declarations
- Profile type enum
- Function prototypes for handlers, renderers, helpers

## Migration Strategy

### Phase 1: Create Infrastructure ✅
1. ✅ Create `ui_state_internal.h` with all declarations
2. Create `ui_helpers.c` (small, independent)
3. Create `ui_renderers.c` (large, but self-contained)
4. Create `ui_event_handlers.c` (depends on renderers)
5. Refactor `ui_state.c` → `ui_state_machine.c` (core only)

### Phase 2: Extract Functions
1. Copy helper functions to `ui_helpers.c`
2. Copy all `render_*` functions to `ui_renderers.c`
3. Copy all `handle_*` functions to `ui_event_handlers.c`
4. Keep only state machine core in `ui_state_machine.c`

### Phase 3: Update Build
1. Update `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS
        "main.c"
        "ui_state_machine.c"      # Renamed from ui_state.c
        "ui_event_handlers.c"     # NEW
        "ui_renderers.c"          # NEW
        "ui_helpers.c"            # NEW
        "data_model.c"
        "utils/application_state.c"
        "utils/system_config.c"
        "pid/pid_controller.c"
        "pid/pid_autotune.c"
    ...
)
```

### Phase 4: Verify
1. Build and fix any linker errors
2. Test state machine functionality
3. Verify all states render correctly

## Benefits

### Maintainability
- ✅ Each file has single, clear responsibility
- ✅ Easy to find functions (by category)
- ✅ Reduced cognitive load

### Readability
- ✅ ~300-900 lines per file (vs 2700)
- ✅ Logical grouping
- ✅ Clear file naming

### Testability
- ✅ Can test renderers independently
- ✅ Can test handlers independently
- ✅ Can mock state variables

### Collaboration
- ✅ Reduced merge conflicts
- ✅ Easier to review changes
- ✅ Clear ownership boundaries

## Risks & Mitigation

### Risk: Build breaks
**Mitigation:** Keep original file until fully migrated

### Risk: Missing declarations
**Mitigation:** Comprehensive internal header with all externs

### Risk: Circular dependencies
**Mitigation:** Clear dependency hierarchy:
```
ui_state_machine.c (top level)
    ↓ calls
ui_event_handlers.c
    ↓ calls
ui_renderers.c
    ↓ calls
ui_helpers.c (bottom level)
```

## Next Steps

1. **Extract ui_helpers.c** - Smallest, most independent
2. **Extract ui_renderers.c** - Large but self-contained
3. **Extract ui_event_handlers.c** - Depends on renderers
4. **Create ui_state_machine.c** - What remains
5. **Update CMakeLists.txt**
6. **Build & test**

## Success Criteria

- ✅ All files < 1000 lines
- ✅ Build succeeds
- ✅ All UI states work correctly
- ✅ No circular dependencies
- ✅ Clear separation of concerns
