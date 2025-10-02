#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "controls_contract.h" // For enums

// Controls interface functions (matching contract)
esp_err_t controls_init(void);
button_event_t controls_get_button_event(void);
rotary_event_t controls_get_rotary_event(void);
bool controls_is_press_closed(void);

// Internal functions
void encoder_isr_handler(void *arg);
void button_isr_handler(void *arg);

#endif // CONTROLS_H
