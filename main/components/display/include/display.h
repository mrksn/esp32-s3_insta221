#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Display interface functions (matching contract)
esp_err_t display_init(void);
void display_clear(void);
void display_text(uint8_t x, uint8_t y, const char *text);
void display_menu(const char **items, uint8_t num_items, uint8_t selected);
void display_status(float current_temp, float target_temp, const char *status);
void display_done(void);

// Internal functions
esp_err_t sh1106_init(void);
void sh1106_clear(void);
void sh1106_write_text(uint8_t x, uint8_t y, const char *text);
void sh1106_invert_screen(bool invert);

#endif // DISPLAY_H
