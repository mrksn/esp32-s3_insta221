#include "display.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "display";

// I2C configuration
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 400000
#define SH1106_ADDR 0x3C

// SH1106 commands
#define SH1106_CMD_SET_CONTRAST 0x81
#define SH1106_CMD_DISPLAY_ON 0xAF
#define SH1106_CMD_DISPLAY_OFF 0xAE
#define SH1106_CMD_SET_PAGE_ADDR 0xB0
#define SH1106_CMD_SET_COLUMN_ADDR_LOW 0x00
#define SH1106_CMD_SET_COLUMN_ADDR_HIGH 0x10
#define SH1106_CMD_INVERT_DISPLAY 0xA7
#define SH1106_CMD_NORMAL_DISPLAY 0xA6

esp_err_t display_init(void)
{
    return sh1106_init();
}

void display_clear(void)
{
    sh1106_clear();
}

void display_text(uint8_t x, uint8_t y, const char *text)
{
    sh1106_write_text(x, y, text);
}

void display_menu(const char **items, uint8_t num_items, uint8_t selected)
{
    display_clear();

    for (uint8_t i = 0; i < num_items && i < 4; i++)
    { // Show up to 4 items
        char buffer[21];
        if (i == selected)
        {
            snprintf(buffer, sizeof(buffer), "> %s", items[i]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "  %s", items[i]);
        }
        display_text(0, i * 2, buffer);
    }
}

void display_status(float current_temp, float target_temp, const char *status)
{
    display_clear();

    char buffer[21];
    snprintf(buffer, sizeof(buffer), "Current: %.1f C", current_temp);
    display_text(0, 0, buffer);

    snprintf(buffer, sizeof(buffer), "Target:  %.1f C", target_temp);
    display_text(0, 2, buffer);

    display_text(0, 4, status);
}

void display_done(void)
{
    display_clear();
    display_text(5, 2, "DONE");
    sh1106_invert_screen(true);
}

esp_err_t sh1106_init(void)
{
    esp_err_t ret;

    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK)
        return ret;

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
        return ret;

    // Initialize SH1106
    uint8_t init_cmds[] = {
        SH1106_CMD_DISPLAY_OFF,
        SH1106_CMD_SET_CONTRAST,
        0x80, // Contrast value
        SH1106_CMD_NORMAL_DISPLAY,
        SH1106_CMD_DISPLAY_ON};

    for (size_t i = 0; i < sizeof(init_cmds); i++)
    {
        ret = sh1106_write_cmd(init_cmds[i]);
        if (ret != ESP_OK)
            return ret;
    }

    sh1106_clear();
    ESP_LOGI(TAG, "SH1106 display initialized");
    return ESP_OK;
}

esp_err_t sh1106_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd}; // Control byte 0x00 for command
    return i2c_master_write_to_device(I2C_MASTER_NUM, SH1106_ADDR, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
}

esp_err_t sh1106_write_data(uint8_t *data, size_t len)
{
    uint8_t buffer[129]; // Max 128 bytes + control
    buffer[0] = 0x40;    // Control byte 0x40 for data
    memcpy(&buffer[1], data, len);
    return i2c_master_write_to_device(I2C_MASTER_NUM, SH1106_ADDR, buffer, len + 1, 1000 / portTICK_PERIOD_MS);
}

void sh1106_clear(void)
{
    uint8_t clear_data[128] = {0};
    for (uint8_t page = 0; page < 8; page++)
    {
        sh1106_write_cmd(SH1106_CMD_SET_PAGE_ADDR | page);
        sh1106_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_LOW | 2); // Offset for 128x64
        sh1106_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_HIGH);
        sh1106_write_data(clear_data, 128);
    }
}

void sh1106_write_text(uint8_t x, uint8_t y, const char *text)
{
    // Simple text rendering - each char is 8x8 pixels
    // For simplicity, just write ASCII values (would need font table for real implementation)
    uint8_t page = y / 8;
    uint8_t col = x;

    sh1106_write_cmd(SH1106_CMD_SET_PAGE_ADDR | page);
    sh1106_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_LOW | (col & 0x0F));
    sh1106_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_HIGH | (col >> 4));

    size_t len = strlen(text);
    if (len > 16)
        len = 16; // Limit to screen width

    uint8_t data[16];
    for (size_t i = 0; i < len; i++)
    {
        data[i] = text[i]; // Placeholder - real implementation needs font
    }

    sh1106_write_data(data, len);
}

void sh1106_invert_screen(bool invert)
{
    sh1106_write_cmd(invert ? SH1106_CMD_INVERT_DISPLAY : SH1106_CMD_NORMAL_DISPLAY);
}
