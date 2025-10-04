#include "controls_contract.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "controls";

// GPIO pins (A and B swapped to fix direction detection)
#define ROTARY_A_PIN GPIO_NUM_5
#define ROTARY_B_PIN GPIO_NUM_4
#define ROTARY_BUTTON_PIN GPIO_NUM_6
#define CONFIRM_BUTTON_PIN GPIO_NUM_7
#define BACK_BUTTON_PIN GPIO_NUM_14       // Moved from GPIO 15 (strapping pin)
#define PAUSE_BUTTON_PIN GPIO_NUM_15      // Moved from GPIO 16
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

// Debounce tracking (timestamps in ticks)
static volatile uint32_t last_confirm_time = 0;
static volatile uint32_t last_back_time = 0;
static volatile uint32_t last_pause_time = 0;
static volatile uint32_t last_rotary_button_time = 0;
#define DEBOUNCE_TIME_MS 20  // For regular buttons
#define ROTARY_BUTTON_DEBOUNCE_MS 200  // Encoder button needs longer debounce

static void IRAM_ATTR rotary_isr_handler(void *arg)
{
    static uint8_t last_a = 1;

    uint8_t pin_a = gpio_get_level(ROTARY_A_PIN);
    uint8_t pin_b = gpio_get_level(ROTARY_B_PIN);

    // Simple quadrature detection
    if (pin_a != last_a) {
        if (pin_a == 0) { // Falling edge on A
            if (pin_b == 1) {
                rotary_counter++;  // CW
            } else {
                rotary_counter--;  // CCW
            }
        }
    }

    last_a = pin_a;
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t pin = (uint32_t)arg;
    uint32_t now = xTaskGetTickCountFromISR();
    uint32_t debounce_ticks = pdMS_TO_TICKS(DEBOUNCE_TIME_MS);

    if (pin == CONFIRM_BUTTON_PIN)
    {
        if ((now - last_confirm_time) > debounce_ticks)
        {
            confirm_pressed = true;
            last_confirm_time = now;
        }
    }
    else if (pin == BACK_BUTTON_PIN)
    {
        if ((now - last_back_time) > debounce_ticks)
        {
            back_pressed = true;
            last_back_time = now;
        }
    }
    else if (pin == PAUSE_BUTTON_PIN)
    {
        if ((now - last_pause_time) > debounce_ticks)
        {
            pause_pressed = true;
            last_pause_time = now;
        }
    }
    else if (pin == ROTARY_BUTTON_PIN)
    {
        uint32_t rotary_debounce_ticks = pdMS_TO_TICKS(ROTARY_BUTTON_DEBOUNCE_MS);
        if ((now - last_rotary_button_time) > rotary_debounce_ticks)
        {
            rotary_button_pressed = true;
            last_rotary_button_time = now;
        }
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
    esp_err_t ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) // ESP_ERR_INVALID_STATE means already installed
    {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add ISR handlers
    gpio_isr_handler_add(ROTARY_A_PIN, rotary_isr_handler, NULL);
    gpio_isr_handler_add(ROTARY_B_PIN, rotary_isr_handler, NULL);
    gpio_isr_handler_add(CONFIRM_BUTTON_PIN, button_isr_handler, (void *)(uint32_t)CONFIRM_BUTTON_PIN);
    gpio_isr_handler_add(BACK_BUTTON_PIN, button_isr_handler, (void *)(uint32_t)BACK_BUTTON_PIN);
    gpio_isr_handler_add(PAUSE_BUTTON_PIN, button_isr_handler, (void *)(uint32_t)PAUSE_BUTTON_PIN);
    gpio_isr_handler_add(ROTARY_BUTTON_PIN, button_isr_handler, (void *)(uint32_t)ROTARY_BUTTON_PIN);

    ESP_LOGI(TAG, "Controls initialized successfully");
    return ESP_OK;
}

button_event_t controls_get_button_event(void)
{
    if (confirm_pressed)
    {
        confirm_pressed = false;
        ESP_LOGI(TAG, "Save button pressed");
        return BUTTON_SAVE;
    }
    if (back_pressed)
    {
        back_pressed = false;
        ESP_LOGI(TAG, "Back button pressed");
        return BUTTON_BACK;
    }
    if (pause_pressed)
    {
        pause_pressed = false;
        ESP_LOGI(TAG, "Pause button pressed");
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
        ESP_LOGI(TAG, "Rotary button pushed");
        return ROTARY_PUSH;
    }

    int current_counter = rotary_counter;
    if (current_counter > last_counter)
    {
        last_counter = current_counter;
        ESP_LOGI(TAG, "Rotary CW (counter: %d)", current_counter);
        return ROTARY_CW;
    }
    else if (current_counter < last_counter)
    {
        last_counter = current_counter;
        ESP_LOGI(TAG, "Rotary CCW (counter: %d)", current_counter);
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
