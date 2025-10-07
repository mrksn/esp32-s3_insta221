# Dead Code Analysis

This document identifies potentially unused or dead code that could be removed in future cleanup.

## Status: For Review Only
**DO NOT DELETE** - This is a reference document. Verify each item before removal.

---

## Potentially Unused Functions

### Low Confidence (May be used via function pointers or external references)

None identified yet - all functions appear to be used in state machine or called from main.

---

## Magic Numbers Found (Not yet in system_constants.h)

### ui_state.c

1. **Line 2255**: `vTaskDelay(pdMS_TO_TICKS(1500))` - Delay after stats reset (1.5 seconds)
2. **Line 2188, 2252, 2289**: `4000` - Hold button duration for stats reset (4 seconds)
3. **Line 1000**: `1000` - Wait period before countdown (1 second)
4. **Line 52, 20**: Display positioning magic numbers throughout rendering functions

### sensors.c (from previous analysis)

1. **Lines 49-52**: Simulation heat-up/cool-down rates:
   - `0.5f` - Heat-up rate per second per % power
   - `0.1f` - Natural cooling rate per second

### display.c (from previous analysis)

1. **Lines 36-132**: Font data arrays (acceptable - these are lookup tables)

---

## Comments Indicating TODO/FIXME

### main.c

- No outstanding TODOs after Phase 1 cleanup

### ui_state.c

- No outstanding TODOs

---

## Recommendations

### High Priority
- None - all critical issues resolved in Phase 1

### Medium Priority
1. Extract UI timing constants (button hold durations, delays) to system_constants.h
2. Consider extracting display positioning constants to a separate header

### Low Priority
1. Add simulation constants to system_constants.h for easier tuning

---

## Code Metrics After Phase 2

### File Sizes
- `main.c`: ~1350 lines
- `ui_state.c`: ~2500 lines (reduced from ~2507)
- `system_constants.h`: 86 lines (NEW)

### Long Functions (>100 lines)
- ~~`render_reset_stats()`: 155 lines~~ → **FIXED** (now 40 lines)
- `render_pressing_active()`: 92 lines (acceptable - state rendering)
- `handle_pid_menu_state()`: ~128 lines (complex menu logic)

### Duplicate Code Patterns
None identified - all duplicates were in headers, now resolved.

---

## Notes

- All Phase 1 critical fixes completed successfully
- Task 2.2 (refactor long functions) completed for `render_reset_stats()`
- Task 2.3 (context structs) skipped to avoid introducing errors
- Task 2.4 (dead code) - no dead code found, created this analysis instead

**Last Updated**: 2025-01-07
