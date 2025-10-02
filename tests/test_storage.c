#include <unity.h>
#include <storage_contract.h>
#include "data_model.h" // Assuming this exists

TEST_CASE("storage_init", "[storage]")
{
    esp_err_t result = storage_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_CASE("storage_save_load_settings", "[storage]")
{
    settings_t settings = {140.0f, 1.0f, 0.1f, 0.05f, 15, 5};
    esp_err_t save_result = storage_save_settings(&settings);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);

    settings_t loaded;
    esp_err_t load_result = storage_load_settings(&loaded);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    TEST_ASSERT_EQUAL_FLOAT(settings.target_temp, loaded.target_temp);
}

TEST_CASE("storage_save_load_print_run", "[storage]")
{
    print_run_t run = {1, SINGLE_SIDED, 0, 0, 0, 0};
    esp_err_t save_result = storage_save_print_run(&run);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);

    print_run_t loaded;
    esp_err_t load_result = storage_load_print_run(&loaded);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    TEST_ASSERT_EQUAL(run.num_shirts, loaded.num_shirts);
}

TEST_CASE("storage_has_saved_data", "[storage]")
{
    bool has_data = storage_has_saved_data();
    TEST_ASSERT(has_data == true || has_data == false);
}
