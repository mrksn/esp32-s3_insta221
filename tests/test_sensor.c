#include <unity.h>
#include <sensor_contract.h>

TEST_CASE("sensor_init", "[sensor]")
{
    esp_err_t result = sensor_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_CASE("sensor_read_temperature", "[sensor]")
{
    float temp;
    bool result = sensor_read_temperature(&temp);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(100.0f, 25.0f, temp); // Reasonable range
}

TEST_CASE("sensor_is_operational", "[sensor]")
{
    bool result = sensor_is_operational();
    TEST_ASSERT_TRUE(result);
}
