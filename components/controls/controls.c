#include "controls_contract.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "controls";

// GPIO pins
#define ROTARY_A_PIN GPIO_NUM_4
#define ROTARY_B_PIN GPIO_NUM_5
#define ROTARY_BUTTON_PIN GPIO_NUM_6
#define CONFIRM_BUTTON_PIN GPIO_NUM_7
#define BACK_BUTTON_PIN GPIO_NUM_15
#define PAUSE_BUTTON_PIN GPIO_NUM_16
#define REED_SWITCH_PIN GPIO_NUM_17
#define LED_GREEN_PIN GPIO_NUM_18    // Temperature ready indicator
#define LED_BLUE_PIN GPIO_NUM_19     // Pause mode indicator

// Rotary encoder state
static volatile int rotary_counter = 0;
static volatile uint8_t rotary_state = 0;

// Button states
static volatile bool confirm_pressed = false;
static volatile bool back_pressed = false;
static volatile bool pause_pressed = false;
static volatile bool rotary_button_pressed = false;

static void IRAM_ATTR rotary_isr_handler(void *arg)
{
    uint8_t pin_a = gpio_get_level(ROTARY_A_PIN);
    uint8_t pin_b = gpio_get_level(ROTARY_B_PIN);

    uint8_t state = (pin_b << 1) | pin_a;
    uint8_t old_state = rotary_state & 0x03;

    if (state != old_state)
    {
        if ((old_state == 0 && state == 1) || (old_state == 2 && state == 3) ||
            (old_state == 3 && state == 0) || (old_state == 1 && state == 2))
        {
            rotary_counter++;
        }
        else if ((old_state == 0 && state == 2) || (old_state == 2 && state == 0) ||
                 (old_state == 3 && state == 1) || (old_state == 1 && state == 3))
        {
            rotary_counter--;
        }
        rotary_state = (rotary_state << 2) | state;
    }
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    int pin = (int)arg;

    switch (pin)
    {
    case CONFIRM_BUTTON_PIN:
        confirm_pressed = true;
        break;
    case BACK_BUTTON_PIN:
        back_pressed = true;
        break;
    case PAUSE_BUTTON_PIN:
        pause_pressed = true;
        break;
    case ROTARY_BUTTON_PIN:
        rotary_button_pressed = true;
        break;
    }
}

esp_err_t controls_init(void)
{
    ESP_LOGI(TAG, "Initializing controls");

    // Configure rotary encoder pins
    gpio_config_t rotary_config = {
        .pin_bit_mask = (1ULL << ROTARY_A_PIN) | (1ULL << ROTARY_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&rotary_config);

    // Configure button pins
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << CONFIRM_BUTTON_PIN) | (1ULL << BACK_BUTTON_PIN) |
                        (1ULL << PAUSE_BUTTON_PIN) | (1ULL << ROTARY_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&button_config);

    // Configure reed switch pin
    gpio_config_t reed_config = {
        .pin_bit_mask = (1ULL << REED_SWITCH_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&reed_config);

    // Configure LED pins (outputs)
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GREEN_PIN) | (1ULL << LED_BLUE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_config);

    // Initialize LEDs to off
    gpio_set_level(LED_GREEN_PIN, 0);
    gpio_set_level(LED_BLUE_PIN, 0);

    // Install ISR service
    gpio_install_isr_service(0);

    // Add ISR handlers
    gpio_isr_handler_add(ROTARY_A_PIN, rotary_isr_handler, NULL);
    gpio_isr_handler_add(ROTARY_B_PIN, rotary_isr_handler, NULL);
    gpio_isr_handler_add(CONFIRM_BUTTON_PIN, button_isr_handler, (void *)CONFIRM_BUTTON_PIN);
    gpio_isr_handler_add(BACK_BUTTON_PIN, button_isr_handler, (void *)BACK_BUTTON_PIN);
    gpio_isr_handler_add(PAUSE_BUTTON_PIN, button_isr_handler, (void *)PAUSE_BUTTON_PIN);
    gpio_isr_handler_add(ROTARY_BUTTON_PIN, button_isr_handler, (void *)ROTARY_BUTTON_PIN);

    ESP_LOGI(TAG, "Controls initialized successfully");
    return ESP_OK;
}

button_event_t controls_get_button_event(void)
{
    if (confirm_pressed)
    {
        confirm_pressed = false;
        return BUTTON_CONFIRM;
    }
    if (back_pressed)
    {
        back_pressed = false;
        return BUTTON_BACK;
    }
    if (pause_pressed)
    {
        pause_pressed = false;
        return BUTTON_PAUSE;
    }
    return BUTTON_NONE;
}

rotary_event_t controls_get_rotary_event(void)
{
    static int last_counter = 0;

    if (rotary_button_pressed)
    {
        rotary_button_pressed = false;
        return ROTARY_PUSH;
    }

    int current_counter = rotary_counter;
    if (current_counter > last_counter)
    {
        last_counter = current_counter;
        return ROTARY_CW;
    }
    else if (current_counter < last_counter)
    {
        last_counter = current_counter;
        return ROTARY_CCW;
    }

    return ROTARY_NONE;
}

bool controls_is_press_closed(void)
{
    return gpio_get_level(REED_SWITCH_PIN) == 0; // Assuming active low
}

void controls_set_led_green(bool on)
{
    gpio_set_level(LED_GREEN_PIN, on ? 1 : 0);
}

void controls_set_led_blue(bool on)
{
    gpio_set_level(LED_BLUE_PIN, on ? 1 : 0);
}
