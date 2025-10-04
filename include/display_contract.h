// display_contract.h - OLED Display Interface
#ifndef DISPLAY_CONTRACT_H
#define DISPLAY_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

// Initialize display
esp_err_t display_init(void);

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

#endif // DISPLAY_CONTRACT_H
