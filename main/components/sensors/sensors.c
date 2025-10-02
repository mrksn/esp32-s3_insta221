#include "sensors.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "sensors";

// SPI configuration
#define SPI_HOST SPI2_HOST
#define PIN_NUM_MISO 19
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5

static spi_device_handle_t spi_handle;

esp_err_t sensor_init(void)
{
    return max31855_init();
}

bool sensor_read_temperature(float *temperature)
{
    if (!temperature)
        return false;

    uint32_t raw_data;
    esp_err_t ret = max31855_read_raw(&raw_data);
    if (ret != ESP_OK)
        return false;

    if (!max31855_check_faults(raw_data))
        return false;

    *temperature = max31855_convert_temperature(raw_data);
    return true;
}

bool sensor_is_operational(void)
{
    uint32_t raw_data;
    esp_err_t ret = max31855_read_raw(&raw_data);
    if (ret != ESP_OK)
        return false;

    return max31855_check_faults(raw_data);
}

esp_err_t max31855_init(void)
{
    esp_err_t ret;

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = -1, // Not used for MAX31855
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
        return ret;

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000, // 4MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK)
    {
        spi_bus_free(SPI_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "MAX31855 sensor initialized");
    return ESP_OK;
}

esp_err_t max31855_read_raw(uint32_t *data)
{
    if (!data)
        return ESP_ERR_INVALID_ARG;

    spi_transaction_t t = {
        .length = 32,
        .rxlength = 32,
        .rx_buffer = data,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // MAX31855 sends MSB first, but ESP32 SPI receives LSB first for 32-bit
    // Need to byte swap
    *data = __builtin_bswap32(*data);

    return ESP_OK;
}

float max31855_convert_temperature(uint32_t raw_data)
{
    // Extract thermocouple temperature (bits 31-18, 14-bit signed)
    int16_t thermocouple_temp = (raw_data >> 18) & 0x3FFF;
    if (thermocouple_temp & 0x2000)
    { // Sign extend
        thermocouple_temp |= 0xC000;
    }

    return thermocouple_temp * 0.25f; // 0.25°C per bit
}

bool max31855_check_faults(uint32_t raw_data)
{
    // Check fault bits (bits 16-17 should be 0 for no fault)
    uint8_t fault = (raw_data >> 16) & 0x03;
    if (fault != 0)
    {
        ESP_LOGW(TAG, "MAX31855 fault detected: %d", fault);
        return false;
    }

    // Check if data is valid (bit 31 should be 0 for valid data)
    if (raw_data & 0x80000000)
    {
        ESP_LOGW(TAG, "MAX31855 data not ready");
        return false;
    }

    return true;
}
