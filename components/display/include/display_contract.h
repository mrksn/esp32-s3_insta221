// display_contract.h - OLED Display Interface
#ifndef DISPLAY_CONTRACT_H
#define DISPLAY_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Initialize display
esp_err_t display_init(void);

// Deinitialize display
esp_err_t display_deinit(void);

// Clear display
void display_clear(void);

// Display text at position (x,y) - buffers only, doesn't update screen
void display_text(uint8_t x, uint8_t y, const char *text);

// Flush buffered changes to screen
void display_flush(void);

// Display menu with selection
void display_menu(const char **items, uint8_t num_items, uint8_t selected);

// Show temperature and status
void display_status(float current_temp, float target_temp, const char *status);

// Signal completion
void display_done(void);

// New functions for enhanced countdown display
void display_set_pixel(uint8_t x, uint8_t y, bool on);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled);
void display_draw_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress);
void display_large_number(uint8_t x, uint8_t y, uint8_t number);

#endif // DISPLAY_CONTRACT_H
