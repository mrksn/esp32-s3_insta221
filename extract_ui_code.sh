#!/bin/bash
# Script to help extract UI state code into separate files

set -e

ORIGINAL_FILE="main/ui_state.c"
BACKUP_FILE="main/ui_state.c.backup"

# Backup original file
echo "Creating backup: $BACKUP_FILE"
cp "$ORIGINAL_FILE" "$BACKUP_FILE"

echo "UI State Code Extraction Helper"
echo "================================"
echo ""
echo "This script will help extract code from ui_state.c into modular files."
echo ""
echo "Files to create:"
echo "  1. main/ui_helpers.c - Helper functions (~250 lines)"
echo "  2. main/ui_renderers.c - Rendering functions (~1200 lines)"
echo "  3. main/ui_event_handlers.c - Event handlers (~900 lines)"
echo "  4. main/ui_state_machine.c - Core state machine (~350 lines)"
echo ""
echo "Extraction Statistics:"
echo "======================"

# Count different function types
echo -n "Helper functions (init_, reset_, perform_, enter_): "
grep -c "^static void \(init_\|reset_\|perform_\|enter_\)" "$ORIGINAL_FILE" || echo "0"

echo -n "Render functions (render_): "
grep -c "^static void render_" "$ORIGINAL_FILE" || echo "0"

echo -n "Handler functions (handle_): "
grep -c "^static void handle_" "$ORIGINAL_FILE" || echo "0"

echo -n "Public API functions (ui_): "
grep -c "^void ui_\|^ui_state_t ui_\|^bool ui_\|^uint32_t ui_" "$ORIGINAL_FILE" || echo "0"

echo ""
echo "Line Distribution:"
echo "=================="
total_lines=$(wc -l < "$ORIGINAL_FILE")
echo "Total lines: $total_lines"

# Estimate where functions are
echo ""
echo "Function Locations (approximate):"
grep -n "^static void init_\|^static void reset_\|^static void perform_\|^static void enter_" "$ORIGINAL_FILE" | head -5 || true
echo "..."

echo ""
echo "Ready to extract! Use the following commands:"
echo ""
echo "# Step 1: Extract line ranges for each category"
echo "grep -n '^static void init_\\|^static void reset_\\|^static void perform_\\|^static void enter_' main/ui_state.c"
echo ""
echo "# Step 2: Extract render functions"
echo "grep -n '^static void render_' main/ui_state.c"
echo ""
echo "# Step 3: Extract event handlers"
echo "grep -n '^static void handle_' main/ui_state.c"
echo ""
echo "Backup saved: $BACKUP_FILE"
