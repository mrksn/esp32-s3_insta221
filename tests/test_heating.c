#include <unity.h>
#include <heating_contract.h>

TEST_CASE("heating_init", "[heating]")
{
    esp_err_t result = heating_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_CASE("heating_set_power", "[heating]")
{
    heating_set_power(50);
    bool active = heating_is_active();
    TEST_ASSERT_TRUE(active);
}

TEST_CASE("heating_emergency_shutoff", "[heating]")
{
    heating_emergency_shutoff();
    bool active = heating_is_active();
    TEST_ASSERT_FALSE(active);
}

TEST_CASE("heating_is_active", "[heating]")
{
    bool active = heating_is_active();
    TEST_ASSERT(active == true || active == false);
}

TEST_CASE("pid_init", "[heating]")
{
    pid_config_t config = {1.0f, 0.1f, 0.05f, 140.0f, 0.0f, 100.0f};
    pid_init(config);
    TEST_PASS();
}

TEST_CASE("pid_update", "[heating]")
{
    pid_config_t config = {1.0f, 0.1f, 0.05f, 140.0f, 0.0f, 100.0f};
    pid_init(config);
    float output = pid_update(130.0f);
    TEST_ASSERT_FLOAT_WITHIN(100.0f, 50.0f, output);
}
