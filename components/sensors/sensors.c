/**
 * @file sensors.c
 * @brief MAX31855 thermocouple sensor implementation
 *
 * This module provides temperature sensing functionality using the MAX31855
 * cold-junction compensated thermocouple-to-digital converter. It handles:
 * - SPI communication with the MAX31855 sensor
 * - Temperature conversion from raw sensor data
 * - Fault detection (open circuit, short circuit)
 * - Error handling and recovery
 *
 * The MAX31855 provides 14-bit resolution with 0.25°C precision and includes
 * cold junction compensation for accurate thermocouple measurements.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include "sensor_contract.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "system_config.h"
#include "esp_timer.h"

static const char *TAG = "sensors";

// SPI Configuration for MAX31855
#define SPI_HOST SPI2_HOST
#define SPI_CLOCK_SPEED_HZ 5000000 ///< 5 MHz SPI clock (within MAX31855 spec)
#define PIN_NUM_MISO GPIO_NUM_10   ///< SPI MISO pin (DO)
#define PIN_NUM_MOSI -1            ///< Not used for MAX31855
#define PIN_NUM_CLK GPIO_NUM_11    ///< SPI clock pin
#define PIN_NUM_CS GPIO_NUM_13     ///< Chip select pin

static spi_device_handle_t spi_handle; ///< SPI device handle for MAX31855

// =============================================================================
// Heat Plate Simulation Model
// =============================================================================

// Simulation state
static float sim_current_temp = 20.0f;      ///< Current simulated temperature (°C)
static uint64_t sim_last_update_time = 0;   ///< Last simulation update time (microseconds)
static float sim_ambient_temp = 20.0f;      ///< Ambient temperature (°C)
static float sim_heating_power = 0.0f;      ///< Current heating power (0-100%)

// Thermal model parameters
#define SIM_THERMAL_MASS 2200.0f        ///< Thermal mass (J/°C) - represents heat capacity of the plate
#define SIM_HEATING_POWER_MAX 2200.0f   ///< Maximum heating power (W) - 2200W heating element
#define SIM_HEAT_LOSS_COEFF 5.0f        ///< Heat loss coefficient (W/°C) - simplified for simulation demonstration
#define SIM_UPDATE_INTERVAL_MS 100      ///< Simulation update interval (ms)

/**
 * @brief Check if simulation mode is enabled
 *
 * @return true if simulation mode is enabled, false otherwise
 */
bool sensor_is_simulation_mode(void)
{
    return SYSTEM_CONFIG.simulation.enabled;
}

/**
 * @brief Update heating power for simulation
 *
 * This function should be called whenever the SSR output changes.
 * In simulation mode, it captures the PWM duty cycle to simulate heating.
 *
 * @param power_percent Heating power as percentage (0-100)
 */
void sensor_sim_set_heating_power(float power_percent)
{
    // Validation: Only allow setting heating power in simulation mode
    if (!SYSTEM_CONFIG.simulation.enabled)
    {
        ESP_LOGW(TAG, "sensor_sim_set_heating_power called but simulation mode is disabled");
        return;
    }

    sim_heating_power = CLAMP(power_percent, 0.0f, 100.0f);
    ESP_LOGI(TAG, "Simulation: Heating power set to %.1f%%", sim_heating_power);
}

/**
 * @brief Update the simulated heat plate temperature
 *
 * This implements a simple thermal model:
 * - Heat input from heating element (proportional to power)
 * - Heat loss to ambient (proportional to temperature difference)
 * - Temperature change based on thermal mass
 *
 * Differential equation:
 * dT/dt = (P_heating - k * (T - T_ambient)) / C
 * where:
 *   T = temperature
 *   P_heating = heating power input
 *   k = heat loss coefficient
 *   T_ambient = ambient temperature
 *   C = thermal mass (heat capacity)
 */
static void sensor_sim_update_temperature(void)
{
    uint64_t current_time = esp_timer_get_time();

    // Initialize on first call
    if (sim_last_update_time == 0)
    {
        sim_last_update_time = current_time;
        return;
    }

    // Calculate time delta in seconds
    float dt = (current_time - sim_last_update_time) / 1000000.0f;
    sim_last_update_time = current_time;

    // Calculate heating power input (W)
    float heating_input = (sim_heating_power / 100.0f) * SIM_HEATING_POWER_MAX;

    // Calculate heat loss (W) - Newton's law of cooling
    float heat_loss = SIM_HEAT_LOSS_COEFF * (sim_current_temp - sim_ambient_temp);

    // Calculate net heat flow (W)
    float net_heat_flow = heating_input - heat_loss;

    // Calculate temperature change (°C)
    // Q = m * c * dT, so dT = Q / (m * c)
    // Power = Q / dt, so Q = Power * dt
    float temp_change = (net_heat_flow * dt) / SIM_THERMAL_MASS;

    // Update temperature
    sim_current_temp += temp_change;

    // Clamp to reasonable bounds
    sim_current_temp = CLAMP(sim_current_temp, sim_ambient_temp - 5.0f, 350.0f);

    ESP_LOGV(TAG, "Simulation: Power=%.1fW, Loss=%.1fW, dT=%.3f°C, Temp=%.2f°C",
             heating_input, heat_loss, temp_change, sim_current_temp);
}

/**
 * @brief Initialize the MAX31855 temperature sensor
 *
 * Configures SPI bus and adds the MAX31855 device for temperature readings.
 * Must be called before any temperature readings are attempted.
 *
 * @return ESP_OK on success, ESP_FAIL on SPI initialization failure
 */
esp_err_t sensor_init(void)
{
    // Check if simulation mode is enabled
    if (SYSTEM_CONFIG.simulation.enabled)
    {
        ESP_LOGI(TAG, "Initializing in SIMULATION mode - no real hardware");
        ESP_LOGI(TAG, "SSR output on GPIO %d can be monitored with oscilloscope", 2);
        sim_current_temp = sim_ambient_temp;
        sim_last_update_time = 0;
        sim_heating_power = 0.0f;
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MAX31855 thermocouple sensor");

    // SPI bus configuration for ESP32-S3
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,   // Not used for SPI mode
        .quadhd_io_num = -1,   // Not used for SPI mode
        .max_transfer_sz = 32, // 32 bits for MAX31855 data
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // SPI device configuration for MAX31855
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,  // Reduce to 1MHz for debugging
        .mode = 1, // SPI mode 1 (CPOL=0, CPHA=1) - try alternative mode
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1, // Single transaction queue
        .flags = SPI_DEVICE_NO_DUMMY,  // No dummy bits
        .pre_cb = NULL,  // No pre-transfer callback
        .post_cb = NULL, // No post-transfer callback
    };

    // Add MAX31855 device to SPI bus
    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add MAX31855 SPI device: %s", esp_err_to_name(ret));
        // Cleanup: Free SPI bus on device addition failure
        spi_bus_free(SPI_HOST);
        spi_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "MAX31855 sensor initialized successfully");
    return ESP_OK;
}

/**
 * @brief Read temperature from MAX31855 sensor
 *
 * Performs SPI transaction to read 32-bit data from MAX31855 and converts
 * it to temperature in Celsius. Handles fault detection and data validation.
 *
 * MAX31855 Data Format:
 * - Bits 31-18: Thermocouple temperature (14-bit signed)
 * - Bits 17-4: Internal temperature (12-bit signed)
 * - Bits 3-2: Reserved
 * - Bit 1: SCV fault (thermocouple short-circuited to VCC)
 * - Bit 0: SCG fault (thermocouple short-circuited to GND)
 *
 * @param[out] temperature Pointer to float where temperature will be stored
 * @return true on successful read, false on failure or fault detection
 */
bool sensor_read_temperature(float *temperature)
{
    if (temperature == NULL)
    {
        ESP_LOGE(TAG, "sensor_read_temperature: NULL temperature pointer");
        return false;
    }

    // Check if simulation mode is enabled
    if (SYSTEM_CONFIG.simulation.enabled)
    {
        // Update simulation model
        sensor_sim_update_temperature();

        // Return simulated temperature with calibration offset applied
        *temperature = sim_current_temp + SYSTEM_CONFIG.temperature.calibration_offset_celsius;

        // Validate simulated temperature is within reasonable bounds
        if (*temperature < -50.0f || *temperature > 500.0f)
        {
            ESP_LOGW(TAG, "Simulation temperature %.2f°C out of reasonable range, clamping", *temperature);
            *temperature = CLAMP(*temperature, -50.0f, 500.0f);
        }

        ESP_LOGI(TAG, "Simulation temperature: %.2f°C (power: %.1f%%)",
                 *temperature, sim_heating_power);
        return true;
    }

    // Real hardware mode - read from MAX31855
    // Validation: Ensure SPI handle is initialized
    if (spi_handle == NULL)
    {
        ESP_LOGE(TAG, "sensor_read_temperature: SPI handle not initialized");
        return false;
    }
    // Prepare SPI transaction for 32-bit read
    spi_transaction_t trans = {
        .length = 32,                  // 32 bits total
        .rxlength = 32,                // Receive 32 bits
        .flags = SPI_TRANS_USE_RXDATA, // Use rx_data buffer
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Combine received bytes into 32-bit value (big-endian)
    uint32_t data = (trans.rx_data[0] << 24) | (trans.rx_data[1] << 16) |
                    (trans.rx_data[2] << 8) | trans.rx_data[3];

    // Check for thermocouple faults
    if (data & 0x01)
    { // SCG fault - short to ground
        ESP_LOGW(TAG, "Thermocouple disconnected or shorted to ground (SCG fault)");
        return false;
    }
    if (data & 0x02)
    { // SCV fault - short to VCC
        ESP_LOGW(TAG, "Thermocouple short-circuited to VCC (SCV fault)");
        return false;
    }

    // Extract thermocouple temperature (bits 31-18, signed 14-bit)
    int16_t temp_raw = (data >> 18) & 0x3FFF;

    // Sign extend from 14-bit to 16-bit
    if (temp_raw & 0x2000)
    {                       // Check sign bit
        temp_raw |= 0xC000; // Sign extend
    }

    // Convert to Celsius (0.25°C per LSB) and apply calibration offset
    *temperature = (temp_raw * 0.25f) + SYSTEM_CONFIG.temperature.calibration_offset_celsius;

    ESP_LOGD(TAG, "Temperature read: %.2f°C (raw: 0x%04X)", *temperature, temp_raw);
    return true;
}

/**
 * @brief Check if the temperature sensor is operational
 *
 * Performs a test read to verify sensor functionality and connectivity.
 * This is a lightweight check that can be used for health monitoring.
 *
 * @return true if sensor is responding correctly, false otherwise
 */
bool sensor_is_operational(void)
{
    float temp;
    return sensor_read_temperature(&temp);
}
