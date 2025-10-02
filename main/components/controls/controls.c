#include "controls.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/queue.h"

static const char *TAG = "controls";

// Pin definitions
#define ENCODER_A_PIN 12
#define ENCODER_B_PIN 13
#define BUTTON_CONFIRM_PIN 14
#define BUTTON_BACK_PIN 15
#define REED_SWITCH_PIN 16

// Event queues
static QueueHandle_t button_queue;
static QueueHandle_t rotary_queue;

// Encoder state
static volatile int encoder_pos = 0;
static volatile uint8_t encoder_state = 0;

esp_err_t controls_init(void)
{
    esp_err_t ret;

    // Configure GPIO for reed switch (input)
    gpio_config_t reed_conf = {
        .pin_bit_mask = (1ULL << REED_SWITCH_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&reed_conf);
    if (ret != ESP_OK)
        return ret;

    // Configure GPIO for buttons (input with interrupt)
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << BUTTON_CONFIRM_PIN) | (1ULL << BUTTON_BACK_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ret = gpio_config(&button_conf);
    if (ret != ESP_OK)
        return ret;

    // Configure GPIO for encoder (input with interrupt)
    gpio_config_t encoder_conf = {
        .pin_bit_mask = (1ULL << ENCODER_A_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ret = gpio_config(&encoder_conf);
    if (ret != ESP_OK)
        return ret;

    gpio_config_t encoder_b_conf = {
        .pin_bit_mask = (1ULL << ENCODER_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&encoder_b_conf);
    if (ret != ESP_OK)
        return ret;

    // Create queues
    button_queue = xQueueCreate(10, sizeof(button_event_t));
    rotary_queue = xQueueCreate(10, sizeof(rotary_event_t));

    // Install ISR services
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK)
        return ret;

    // Add ISR handlers
    ret = gpio_isr_handler_add(BUTTON_CONFIRM_PIN, button_isr_handler, (void *)BUTTON_CONFIRM);
    if (ret != ESP_OK)
        return ret;

    ret = gpio_isr_handler_add(BUTTON_BACK_PIN, button_isr_handler, (void *)BUTTON_BACK);
    if (ret != ESP_OK)
        return ret;

    ret = gpio_isr_handler_add(ENCODER_A_PIN, encoder_isr_handler, NULL);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Controls initialized");
    return ESP_OK;
}

button_event_t controls_get_button_event(void)
{
    button_event_t event;
    if (xQueueReceive(button_queue, &event, 0) == pdTRUE)
    {
        return event;
    }
    return BUTTON_NONE;
}

rotary_event_t controls_get_rotary_event(void)
{
    rotary_event_t event;
    if (xQueueReceive(rotary_queue, &event, 0) == pdTRUE)
    {
        return event;
    }
    return ROTARY_NONE;
}

bool controls_is_press_closed(void)
{
    return gpio_get_level(REED_SWITCH_PIN) == 0; // Assuming active low
}

void IRAM_ATTR encoder_isr_handler(void *arg)
{
    static uint8_t last_state = 0;
    uint8_t a = gpio_get_level(ENCODER_A_PIN);
    uint8_t b = gpio_get_level(ENCODER_B_PIN);

    uint8_t state = (b << 1) | a;
    encoder_state = (encoder_state << 2) | state;

    // Check for valid transitions
    if ((encoder_state & 0x0F) == 0x09 || (encoder_state & 0x0F) == 0x06)
    {
        rotary_event_t event = ROTARY_CW;
        xQueueSendFromISR(rotary_queue, &event, NULL);
    }
    else if ((encoder_state & 0x0F) == 0x0B || (encoder_state & 0x0F) == 0x0C)
    {
        rotary_event_t event = ROTARY_CCW;
        xQueueSendFromISR(rotary_queue, &event, NULL);
    }
}

void IRAM_ATTR button_isr_handler(void *arg)
{
    button_event_t event;
    if ((int)arg == BUTTON_CONFIRM)
    {
        event = BUTTON_CONFIRM;
    }
    else if ((int)arg == BUTTON_BACK)
    {
        event = BUTTON_BACK;
    }
    else
    {
        return;
    }
    xQueueSendFromISR(button_queue, &event, NULL);
}
