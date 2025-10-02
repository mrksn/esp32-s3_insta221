#include <unity.h>
#include <display_contract.h>

TEST_CASE("display_init", "[display]")
{
    esp_err_t result = display_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_CASE("display_clear", "[display]")
{
    display_clear();
    // Visual test - would need hardware verification
    TEST_PASS();
}

TEST_CASE("display_text", "[display]")
{
    display_text(0, 0, "Test");
    // Visual test
    TEST_PASS();
}

TEST_CASE("display_menu", "[display]")
{
    const char *items[] = {"Item1", "Item2"};
    display_menu(items, 2, 0);
    // Visual test
    TEST_PASS();
}

TEST_CASE("display_status", "[display]")
{
    display_status(150.0f, 140.0f, "Heating");
    // Visual test
    TEST_PASS();
}

TEST_CASE("display_done", "[display]")
{
    display_done();
    // Visual test
    TEST_PASS();
}
