#include "display_contract.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

// I2C configuration
#define I2C_MASTER_SCL_IO GPIO_NUM_9
#define I2C_MASTER_SDA_IO GPIO_NUM_8
#define I2C_MASTER_FREQ_HZ 400000

// SH1106 configuration
#define SH1106_ADDR 0x3C
#define SH1106_WIDTH 128
#define SH1106_HEIGHT 64

// SH1106 commands
#define SH1106_CMD_SET_CONTRAST 0x81
#define SH1106_CMD_DISPLAY_ON 0xAF
#define SH1106_CMD_DISPLAY_OFF 0xAE
#define SH1106_CMD_SET_PAGE_ADDR 0xB0
#define SH1106_CMD_SET_COLUMN_ADDR_LOW 0x00
#define SH1106_CMD_SET_COLUMN_ADDR_HIGH 0x10

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t sh1106_dev_handle;
static uint8_t display_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

static esp_err_t i2c_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd}; // Control byte 0x00 for command
    return i2c_master_transmit(sh1106_dev_handle, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t i2c_write_data(uint8_t *data, size_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = 0x40; // Control byte 0x40 for data
    memcpy(&buffer[1], data, len);
    return i2c_master_transmit(sh1106_dev_handle, buffer, len + 1, pdMS_TO_TICKS(100));
}

static esp_err_t display_update(void)
{
    // Send the entire display buffer to the OLED
    for (uint8_t page = 0; page < (SH1106_HEIGHT / 8); page++)
    {
        // Set page address
        i2c_write_cmd(SH1106_CMD_SET_PAGE_ADDR | page);

        // Set column address (low byte)
        i2c_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_LOW | 2); // Offset for SH1106

        // Set column address (high byte)
        i2c_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_HIGH | 0);

        // Send page data (128 bytes per page)
        uint8_t *page_data = &display_buffer[page * SH1106_WIDTH];
        i2c_write_data(page_data, SH1106_WIDTH);
    }
    return ESP_OK;
}

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initializing SH1106 display");

    // I2C bus configuration
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C bus creation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // SH1106 device configuration
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SH1106_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &sh1106_dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C device addition failed: %s", esp_err_to_name(ret));
        i2c_del_master_bus(i2c_bus_handle);
        return ret;
    }

    // Initialize display buffer
    memset(display_buffer, 0, sizeof(display_buffer));

    // SH1106 initialization sequence
    i2c_write_cmd(SH1106_CMD_DISPLAY_OFF);
    i2c_write_cmd(SH1106_CMD_SET_CONTRAST);
    i2c_write_cmd(0x7F); // Contrast value
    i2c_write_cmd(SH1106_CMD_DISPLAY_ON);

    ESP_LOGI(TAG, "SH1106 display initialized successfully");
    return ESP_OK;
}

esp_err_t display_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing SH1106 display");

    if (sh1106_dev_handle)
    {
        i2c_master_bus_rm_device(sh1106_dev_handle);
        sh1106_dev_handle = NULL;
    }

    if (i2c_bus_handle)
    {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
    }

    ESP_LOGI(TAG, "SH1106 display deinitialized successfully");
    return ESP_OK;
}

void display_clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
    display_update();
}

void display_text(uint8_t x, uint8_t y, const char *text)
{
    if (!text || x >= SH1106_WIDTH || y >= (SH1106_HEIGHT / 8))
        return;

    uint8_t *buffer = &display_buffer[y * SH1106_WIDTH + x];
    size_t len = strlen(text);

    // Simple text rendering (basic ASCII, 8x8 font approximation)
    for (size_t i = 0; i < len && (x + i * 8) < SH1106_WIDTH; i++)
    {
        char c = text[i];
        if (c >= 32 && c <= 126)
        {
            // Very basic font - just set some pixels
            uint8_t char_pattern = (c == ' ') ? 0 : 0xFF;
            *buffer++ = char_pattern;
            for (int j = 1; j < 8 && (x + i * 8 + j) < SH1106_WIDTH; j++)
            {
                *buffer++ = 0;
            }
        }
    }

    display_update();
}

void display_menu(const char **items, uint8_t num_items, uint8_t selected)
{
    display_clear();

    for (uint8_t i = 0; i < num_items && i < 4; i++)
    {
        char line[21];
        if (i == selected)
        {
            snprintf(line, sizeof(line), "> %s", items[i]);
        }
        else
        {
            snprintf(line, sizeof(line), "  %s", items[i]);
        }
        display_text(0, i, line);
    }

    // display_update() is already called by display_text(), so no need to call it again
}

void display_status(float current_temp, float target_temp, const char *status)
{
    display_clear();

    char line1[21];
    char line2[21];
    char line3[21];

    snprintf(line1, sizeof(line1), "Temp: %.1f/%.1f C", current_temp, target_temp);
    snprintf(line2, sizeof(line2), "Status: %s", status ? status : "Unknown");
    snprintf(line3, sizeof(line3), "Insta Retrofit");

    display_text(0, 0, line1);
    display_text(0, 2, line2);
    display_text(0, 4, line3);

    // display_update() is already called by display_text(), so no need to call it again
}

void display_done(void)
{
    display_clear();
    display_text(0, 2, "DONE!");

    // display_update() is already called by display_text(), so no need to call it again
}
