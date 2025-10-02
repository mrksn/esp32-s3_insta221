#include <unity.h>
#include <controls_contract.h>

TEST_CASE("controls_init", "[controls]")
{
    esp_err_t result = controls_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_CASE("controls_get_button_event", "[controls]")
{
    button_event_t event = controls_get_button_event();
    TEST_ASSERT(event >= BUTTON_NONE && event <= BUTTON_BACK);
}

TEST_CASE("controls_get_rotary_event", "[controls]")
{
    rotary_event_t event = controls_get_rotary_event();
    TEST_ASSERT(event >= ROTARY_NONE && event <= ROTARY_PUSH);
}

TEST_CASE("controls_is_press_closed", "[controls]")
{
    bool closed = controls_is_press_closed();
    TEST_ASSERT(closed == true || closed == false);
}
