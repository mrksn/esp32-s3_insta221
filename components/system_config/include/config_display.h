/**
 * @file config_display.h
 * @brief Display layout and UI configuration constants
 *
 * This header centralizes all display-related constants including
 * screen dimensions, layout positions, and text formatting.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef CONFIG_DISPLAY_H
#define CONFIG_DISPLAY_H

#include <stdint.h>

// =============================================================================
// Display Hardware Configuration
// =============================================================================

/**
 * @brief Physical display dimensions
 */
#define DISPLAY_WIDTH_PIXELS   128   ///< Display width in pixels
#define DISPLAY_HEIGHT_PIXELS  64    ///< Display height in pixels
#define DISPLAY_WIDTH_CHARS    21    ///< Display width in characters (6px font)
#define DISPLAY_HEIGHT_LINES   8     ///< Display height in lines (8px font)

/**
 * @brief Character dimensions for standard font
 */
#define DISPLAY_CHAR_WIDTH_PX  6     ///< Character width in pixels
#define DISPLAY_CHAR_HEIGHT_PX 8     ///< Character height in pixels

// =============================================================================
// Layout Positions - Text Display
// =============================================================================

/**
 * @brief Standard text line positions (Y coordinate)
 */
#define DISPLAY_LINE_0  0   ///< First line (header)
#define DISPLAY_LINE_1  1   ///< Second line
#define DISPLAY_LINE_2  2   ///< Third line
#define DISPLAY_LINE_3  3   ///< Fourth line (center)
#define DISPLAY_LINE_4  4   ///< Fifth line
#define DISPLAY_LINE_5  5   ///< Sixth line
#define DISPLAY_LINE_6  6   ///< Seventh line
#define DISPLAY_LINE_7  7   ///< Eighth line (bottom)

/**
 * @brief Column positions for text alignment
 */
#define DISPLAY_COL_LEFT    0    ///< Left-aligned text (column 0)
#define DISPLAY_COL_CENTER  10   ///< Center-aligned text (column 10)
#define DISPLAY_COL_RIGHT   15   ///< Right-aligned text (column 15)

// =============================================================================
// Layout Positions - Large Text and Graphics
// =============================================================================

/**
 * @brief Large text positioning (for countdown displays)
 */
#define DISPLAY_LARGE_TEXT_X_CENTER   20   ///< X position for centered large text
#define DISPLAY_LARGE_TEXT_Y_CENTER   16   ///< Y position for centered large text
#define DISPLAY_LARGE_TEXT_X_LEFT     0    ///< X position for left-aligned large text
#define DISPLAY_LARGE_TEXT_Y_TOP      0    ///< Y position for top-aligned large text

/**
 * @brief Startup screen logo positioning
 */
#define DISPLAY_LOGO_X      28   ///< X position for "DIN" logo text
#define DISPLAY_LOGO_Y      8    ///< Y position for "DIN" logo text

/**
 * @brief Progress bar positioning and sizing
 */
#define DISPLAY_PROGRESS_BAR_X       15   ///< Progress bar X position
#define DISPLAY_PROGRESS_BAR_Y       48   ///< Progress bar Y position
#define DISPLAY_PROGRESS_BAR_WIDTH   98   ///< Progress bar width
#define DISPLAY_PROGRESS_BAR_HEIGHT  12   ///< Progress bar height
#define DISPLAY_PROGRESS_BAR_START   15   ///< Progress bar start X (for filling)
#define DISPLAY_PROGRESS_BAR_END     48   ///< Progress bar end X (for filling)

// =============================================================================
// Menu Display Configuration
// =============================================================================

/**
 * @brief Menu item display settings
 */
#define MENU_ITEMS_PER_SCREEN   4    ///< Maximum menu items visible at once
#define MENU_ITEM_HEIGHT_LINES  2    ///< Lines per menu item (with spacing)
#define MENU_SELECTION_MARKER   ">"  ///< Character for selected menu item
#define MENU_INDENT_SPACES      2    ///< Indentation for non-selected items

/**
 * @brief Menu item positioning
 */
#define MENU_ITEM_LINE_0  0   ///< First menu item line
#define MENU_ITEM_LINE_1  2   ///< Second menu item line
#define MENU_ITEM_LINE_2  4   ///< Third menu item line
#define MENU_ITEM_LINE_3  6   ///< Fourth menu item line

// =============================================================================
// Text Formatting Constants
// =============================================================================

/**
 * @brief Maximum string lengths for various display elements
 */
#define DISPLAY_BUFFER_SIZE      32   ///< General purpose display buffer
#define DISPLAY_TEMP_STRING_LEN  16   ///< Temperature string length
#define DISPLAY_TIME_STRING_LEN  16   ///< Time string length
#define DISPLAY_LABEL_MAX_LEN    20   ///< Maximum label length

/**
 * @brief Temperature display format strings
 */
#define TEMP_FORMAT_CURRENT  "Temp: %.1f C"      ///< Current temperature format
#define TEMP_FORMAT_TARGET   "Target: %.1f C"    ///< Target temperature format
#define TEMP_FORMAT_DELTA    "Delta: %.1f C"     ///< Temperature delta format
#define TEMP_FORMAT_SHORT    "%.1f C"            ///< Short temperature format

/**
 * @brief Time display format strings
 */
#define TIME_FORMAT_SECONDS     "%ds"           ///< Seconds only (e.g., "15s")
#define TIME_FORMAT_MIN_SEC     "%02d:%02d"     ///< Minutes:Seconds (e.g., "05:30")
#define TIME_FORMAT_HOUR_MIN    "%02d:%02d"     ///< Hours:Minutes (e.g., "02:15")
#define TIME_FORMAT_ELAPSED     "Elapsed: %s"   ///< Elapsed time with label

/**
 * @brief Status message display strings
 */
#define STATUS_HEATING       "Heating..."
#define STATUS_READY         "Ready"
#define STATUS_PRESSING      "Pressing"
#define STATUS_DONE          "DONE"
#define STATUS_WAITING       "Waiting"
#define STATUS_ERROR         "ERROR"
#define STATUS_PAUSED        "PAUSED"

// =============================================================================
// Animation and Update Timing
// =============================================================================

/**
 * @brief Display update rates (milliseconds)
 */
#define DISPLAY_UPDATE_RATE_NORMAL_MS   100    ///< Normal UI update rate
#define DISPLAY_UPDATE_RATE_PRESSING_MS 50     ///< Pressing countdown update rate
#define DISPLAY_UPDATE_RATE_HEATUP_MS   1000   ///< Heat-up screen update rate

/**
 * @brief Timeout durations for various screens
 */
#define DISPLAY_STARTUP_TIMEOUT_SEC     3      ///< Startup screen timeout
#define DISPLAY_MESSAGE_TIMEOUT_SEC     2      ///< Info message timeout
#define DISPLAY_ERROR_TIMEOUT_SEC       5      ///< Error message timeout

// =============================================================================
// Special Characters and Symbols
// =============================================================================

/**
 * @brief Special display characters
 */
#define CHAR_DEGREE       "\xF8"   ///< Degree symbol (°)
#define CHAR_ARROW_UP     "^"      ///< Up arrow
#define CHAR_ARROW_DOWN   "v"      ///< Down arrow
#define CHAR_ARROW_LEFT   "<"      ///< Left arrow
#define CHAR_ARROW_RIGHT  ">"      ///< Right arrow
#define CHAR_CHECK        "+"      ///< Check mark (limited charset)
#define CHAR_CROSS        "X"      ///< Cross mark

// =============================================================================
// Display Validation Macros
// =============================================================================

/**
 * @brief Validate display coordinates
 */
#define DISPLAY_VALID_X(x) ((x) < DISPLAY_WIDTH_CHARS)
#define DISPLAY_VALID_Y(y) ((y) < DISPLAY_HEIGHT_LINES)
#define DISPLAY_VALID_PIXEL_X(x) ((x) < DISPLAY_WIDTH_PIXELS)
#define DISPLAY_VALID_PIXEL_Y(y) ((y) < DISPLAY_HEIGHT_PIXELS)

/**
 * @brief Clamp coordinates to valid range
 */
#define DISPLAY_CLAMP_X(x) ((x) >= DISPLAY_WIDTH_CHARS ? (DISPLAY_WIDTH_CHARS - 1) : (x))
#define DISPLAY_CLAMP_Y(y) ((y) >= DISPLAY_HEIGHT_LINES ? (DISPLAY_HEIGHT_LINES - 1) : (y))

// =============================================================================
// Helper Macros for Display Layout
// =============================================================================

/**
 * @brief Calculate pixel position from character position
 */
#define CHAR_TO_PIXEL_X(col) ((col) * DISPLAY_CHAR_WIDTH_PX)
#define CHAR_TO_PIXEL_Y(line) ((line) * DISPLAY_CHAR_HEIGHT_PX)

/**
 * @brief Calculate character position from pixel position
 */
#define PIXEL_TO_CHAR_X(px) ((px) / DISPLAY_CHAR_WIDTH_PX)
#define PIXEL_TO_CHAR_Y(py) ((py) / DISPLAY_CHAR_HEIGHT_PX)

/**
 * @brief Center text horizontally (returns column position)
 * @param text_len Length of text string in characters
 */
#define CENTER_TEXT_X(text_len) ((DISPLAY_WIDTH_CHARS - (text_len)) / 2)

/**
 * @brief Center text vertically (returns line position)
 */
#define CENTER_TEXT_Y() (DISPLAY_HEIGHT_LINES / 2)

#endif // CONFIG_DISPLAY_H
