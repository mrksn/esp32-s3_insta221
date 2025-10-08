// controls_contract.h - Input Controls Interface
#ifndef CONTROLS_CONTRACT_H
#define CONTROLS_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Button events
typedef enum
{
    BUTTON_NONE,
    BUTTON_SAVE,
    BUTTON_BACK,
    BUTTON_PAUSE
} button_event_t;

// Rotary encoder events
typedef enum
{
    ROTARY_NONE,
    ROTARY_CW,
    ROTARY_CCW,
    ROTARY_PUSH
} rotary_event_t;

// Initialize controls
esp_err_t controls_init(void);

// Deinitialize controls and free resources
esp_err_t controls_deinit(void);

// Get next button event (non-blocking)
button_event_t controls_get_button_event(void);

// Get next rotary event (non-blocking)
rotary_event_t controls_get_rotary_event(void);

// Check reed switch (press closed)
bool controls_is_press_closed(void);

// Check heating enable switch (physical switch)
bool controls_is_heating_switch_on(void);

// Check if rotary button is currently pressed (for hold detection)
bool controls_is_rotary_button_pressed(void);

// LED control functions
void controls_set_led_green(bool on);
void controls_set_led_blue(bool on);

#endif // CONTROLS_CONTRACT_H
