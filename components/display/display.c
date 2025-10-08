#include "display_contract.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

// I2C configuration
#define I2C_MASTER_SCL_IO GPIO_NUM_35
#define I2C_MASTER_SDA_IO GPIO_NUM_36
#define I2C_MASTER_FREQ_HZ 100000

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
#define SH1106_CMD_SEG_REMAP 0xA1        // Flip horizontally
#define SH1106_CMD_COM_SCAN_DEC 0xC8     // Flip vertically
#define SH1106_CMD_INVERT_DISPLAY 0xA7   // Invert display
#define SH1106_CMD_NORMAL_DISPLAY 0xA6   // Normal display

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t sh1106_dev_handle;
static uint8_t display_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

// Simple 5x8 font for ASCII characters 32-127
static const uint8_t font5x8[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x56, 0x20, 0x50}, // &
    {0x00, 0x08, 0x07, 0x03, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x80, 0x70, 0x30, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x00, 0x60, 0x60, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x72, 0x49, 0x49, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x49, 0x4D, 0x33}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x31}, // 6
    {0x41, 0x21, 0x11, 0x09, 0x07}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x46, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x00, 0x14, 0x00, 0x00}, // :
    {0x00, 0x40, 0x34, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x59, 0x09, 0x06}, // ?
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, // @
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x73}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x26, 0x49, 0x49, 0x49, 0x32}, // S
    {0x03, 0x01, 0x7F, 0x01, 0x03}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x59, 0x49, 0x4D, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x41, 0x7F}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x03, 0x07, 0x08, 0x00}, // `
    {0x20, 0x54, 0x54, 0x78, 0x40}, // a
    {0x7F, 0x28, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x28}, // c
    {0x38, 0x44, 0x44, 0x28, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x00, 0x08, 0x7E, 0x09, 0x02}, // f
    {0x18, 0xA4, 0xA4, 0x9C, 0x78}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x40, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x78, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0xFC, 0x18, 0x24, 0x24, 0x18}, // p
    {0x18, 0x24, 0x24, 0x18, 0xFC}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x24}, // s
    {0x04, 0x04, 0x3F, 0x44, 0x24}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x4C, 0x90, 0x90, 0x90, 0x7C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x77, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x02, 0x01, 0x02, 0x04, 0x02}, // ~
};

static esp_err_t i2c_write_cmd(uint8_t cmd)
{
    if (sh1106_dev_handle == NULL)
    {
        ESP_LOGE(TAG, "i2c_write_cmd: Device handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t data[2] = {0x00, cmd}; // Control byte 0x00 for command
    return i2c_master_transmit(sh1106_dev_handle, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t i2c_write_data(uint8_t *data, size_t len)
{
    if (sh1106_dev_handle == NULL)
    {
        ESP_LOGE(TAG, "i2c_write_data: Device handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || len == 0)
    {
        ESP_LOGE(TAG, "i2c_write_data: Invalid data pointer or length");
        return ESP_ERR_INVALID_ARG;
    }
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
        // Calculate buffer offset and validate bounds
        uint16_t offset = page * SH1106_WIDTH;
        if (offset >= sizeof(display_buffer))
        {
            ESP_LOGE(TAG, "display_update: Buffer overflow at page %d", page);
            return ESP_ERR_INVALID_SIZE;
        }

        // Set page address
        esp_err_t ret = i2c_write_cmd(SH1106_CMD_SET_PAGE_ADDR | page);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "display_update: Failed to set page address");
            return ret;
        }

        // Set column address (low byte)
        ret = i2c_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_LOW | 2); // Offset for SH1106
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "display_update: Failed to set column address low");
            return ret;
        }

        // Set column address (high byte)
        ret = i2c_write_cmd(SH1106_CMD_SET_COLUMN_ADDR_HIGH | 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "display_update: Failed to set column address high");
            return ret;
        }

        // Send page data (128 bytes per page)
        uint8_t *page_data = &display_buffer[offset];
        ret = i2c_write_data(page_data, SH1106_WIDTH);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "display_update: Failed to write page data");
            return ret;
        }
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
    i2c_write_cmd(SH1106_CMD_SEG_REMAP);      // Flip horizontally
    i2c_write_cmd(SH1106_CMD_COM_SCAN_DEC);   // Flip vertically
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

static void draw_text_internal(uint8_t x, uint8_t y, const char *text)
{
    if (text == NULL)
    {
        ESP_LOGW(TAG, "draw_text_internal: NULL text pointer");
        return;
    }

    if (y >= (SH1106_HEIGHT / 8))
    {
        ESP_LOGW(TAG, "draw_text_internal: y=%d out of bounds", y);
        return;
    }

    size_t len = strlen(text);
    uint8_t col = x;

    // Bounds check for starting position
    if (x >= SH1106_WIDTH)
    {
        ESP_LOGW(TAG, "draw_text_internal: x=%d out of bounds", x);
        return;
    }

    for (size_t i = 0; i < len && col < SH1106_WIDTH; i++)
    {
        char c = text[i];
        if (c >= 32 && c <= 126)
        {
            // Bounds check for font array access
            int font_index = c - 32;
            if (font_index < 0 || font_index >= 96)
            {
                ESP_LOGW(TAG, "draw_text_internal: Invalid font index %d for char '%c'", font_index, c);
                continue;
            }

            // Get font data for this character
            const uint8_t *font_char = font5x8[font_index];

            // Draw 5 columns of the character
            for (int j = 0; j < 5 && col < SH1106_WIDTH; j++)
            {
                // Bounds check for buffer access
                uint16_t buffer_index = y * SH1106_WIDTH + col;
                if (buffer_index >= sizeof(display_buffer))
                {
                    ESP_LOGW(TAG, "draw_text_internal: Buffer overflow at index %d", buffer_index);
                    return;
                }
                display_buffer[buffer_index] = font_char[j];
                col++;
            }

            // Add 1 pixel spacing between characters
            if (col < SH1106_WIDTH)
            {
                uint16_t buffer_index = y * SH1106_WIDTH + col;
                if (buffer_index < sizeof(display_buffer))
                {
                    display_buffer[buffer_index] = 0x00;
                    col++;
                }
            }
        }
    }
}

void display_text(uint8_t x, uint8_t y, const char *text)
{
    draw_text_internal(x, y, text);
    // Note: No auto-update to allow batching multiple text calls
}

void display_flush(void)
{
    display_update();
}

void display_menu(const char **items, uint8_t num_items, uint8_t selected)
{
    if (items == NULL)
    {
        ESP_LOGE(TAG, "display_menu: NULL items pointer");
        return;
    }

    if (num_items == 0)
    {
        ESP_LOGW(TAG, "display_menu: num_items is zero");
        return;
    }

    // Validate selected index
    if (selected >= num_items)
    {
        ESP_LOGW(TAG, "display_menu: selected=%d >= num_items=%d, clamping", selected, num_items);
        selected = num_items - 1;
    }

    memset(display_buffer, 0, sizeof(display_buffer));

    // Calculate scroll offset to keep selected item visible
    // Display can show 4 items at once (8 pages / 2 pages per item)
    uint8_t visible_items = 4;
    uint8_t scroll_offset = 0;

    if (selected >= visible_items) {
        scroll_offset = selected - visible_items + 1;
    }

    // Display up to 4 items starting from scroll_offset
    for (uint8_t i = 0; i < visible_items && (i + scroll_offset) < num_items; i++)
    {
        uint8_t item_index = i + scroll_offset;

        // Validate array index
        if (item_index >= num_items)
        {
            ESP_LOGW(TAG, "display_menu: item_index=%d out of bounds", item_index);
            break;
        }

        // Validate item pointer
        if (items[item_index] == NULL)
        {
            ESP_LOGW(TAG, "display_menu: items[%d] is NULL", item_index);
            continue;
        }

        char line[21];
        if (item_index == selected)
        {
            snprintf(line, sizeof(line), "> %s", items[item_index]);
        }
        else
        {
            snprintf(line, sizeof(line), "  %s", items[item_index]);
        }
        draw_text_internal(0, i * 2, line);
    }

    display_update();
}

void display_status(float current_temp, float target_temp, const char *status)
{
    memset(display_buffer, 0, sizeof(display_buffer));

    char line1[21];
    char line2[21];
    char line3[21];

    snprintf(line1, sizeof(line1), "Temp: %.1f/%.1f C", current_temp, target_temp);
    snprintf(line2, sizeof(line2), "Status: %s", status ? status : "Unknown");
    snprintf(line3, sizeof(line3), "Insta Retrofit");

    draw_text_internal(0, 0, line1);
    draw_text_internal(0, 2, line2);
    draw_text_internal(0, 4, line3);

    display_update();
}

void display_done(void)
{
    display_clear();
    display_text(0, 2, "DONE!");

    // display_update() is already called by display_text(), so no need to call it again
}

// Set a single pixel on/off
void display_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= SH1106_WIDTH || y >= SH1106_HEIGHT) return;

    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    uint16_t index = page * SH1106_WIDTH + x;

    if (on) {
        display_buffer[index] |= (1 << bit);
    } else {
        display_buffer[index] &= ~(1 << bit);
    }
}

// Draw a rectangle
void display_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled)
{
    if (filled) {
        for (uint8_t i = 0; i < width; i++) {
            for (uint8_t j = 0; j < height; j++) {
                display_set_pixel(x + i, y + j, true);
            }
        }
    } else {
        // Draw outline
        for (uint8_t i = 0; i < width; i++) {
            display_set_pixel(x + i, y, true);
            display_set_pixel(x + i, y + height - 1, true);
        }
        for (uint8_t j = 0; j < height; j++) {
            display_set_pixel(x, y + j, true);
            display_set_pixel(x + width - 1, y + j, true);
        }
    }
}

// Draw a progress bar
void display_draw_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress)
{
    // Draw outline
    display_draw_rect(x, y, width, height, false);

    // Draw filled portion based on progress (0-100)
    uint8_t filled_width = ((width - 2) * progress) / 100;
    if (filled_width > 0) {
        display_draw_rect(x + 1, y + 1, filled_width, height - 2, true);
    }
}

// Display a large number (0-99)
void display_large_number(uint8_t x, uint8_t y, uint8_t number)
{
    // Clamp number to valid range
    if (number > 99)
    {
        ESP_LOGW(TAG, "display_large_number: number=%d clamped to 99", number);
        number = 99;
    }

    // For simplicity, use scaled-up regular font
    char buf[4];
    snprintf(buf, sizeof(buf), "%2d", number);

    // Draw larger characters by setting multiple pixels
    for (int i = 0; buf[i] != '\0'; i++) {
        char c = buf[i];
        if (c == ' ') {
            x += 16;
            continue;
        }

        int idx = c - 32;
        if (idx >= 0 && idx < 96) {
            for (int col = 0; col < 5; col++) {
                uint8_t line = font5x8[idx][col];
                for (int bit = 0; bit < 8; bit++) {
                    if (line & (1 << bit)) {
                        // Scale 3x
                        for (int sx = 0; sx < 3; sx++) {
                            for (int sy = 0; sy < 3; sy++) {
                                // Bounds check before setting pixel
                                uint8_t px = x + col * 3 + sx;
                                uint8_t py = y + bit * 3 + sy;
                                if (px < SH1106_WIDTH && py < SH1106_HEIGHT)
                                {
                                    display_set_pixel(px, py, true);
                                }
                            }
                        }
                    }
                }
            }
        }
        x += 16; // Character spacing
    }
}

// Invert the display
void display_invert(bool inverted)
{
    uint8_t cmd = inverted ? SH1106_CMD_INVERT_DISPLAY : SH1106_CMD_NORMAL_DISPLAY;
    i2c_write_cmd(cmd);
}

// Display large text (4x scaled)
void display_large_text(uint8_t x, uint8_t y, const char *text)
{
    if (text == NULL)
    {
        ESP_LOGW(TAG, "display_large_text: NULL text pointer");
        return;
    }

    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == ' ') {
            x += 20;
            continue;
        }

        int idx = c - 32;
        if (idx >= 0 && idx < 96) {
            for (int col = 0; col < 5; col++) {
                uint8_t line = font5x8[idx][col];
                for (int bit = 0; bit < 8; bit++) {
                    if (line & (1 << bit)) {
                        // Scale 4x for larger text
                        for (int sx = 0; sx < 4; sx++) {
                            for (int sy = 0; sy < 4; sy++) {
                                // Bounds check before setting pixel
                                uint8_t px = x + col * 4 + sx;
                                uint8_t py = y + bit * 4 + sy;
                                if (px < SH1106_WIDTH && py < SH1106_HEIGHT)
                                {
                                    display_set_pixel(px, py, true);
                                }
                            }
                        }
                    }
                }
            }
        }
        x += 24; // Character spacing for 4x scale
    }
}
