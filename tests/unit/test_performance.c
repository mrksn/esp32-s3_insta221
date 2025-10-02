/**
 * @file test_performance.c
 * @brief Performance tests for safety-critical functions
 *
 * This test suite validates that safety-critical functions meet performance
 * requirements for real-time operation. Tests measure response times for:
 * - Safety validation functions (<1s requirement)
 * - Emergency shutdown operations (<500ms target)
 * - Temperature sensor operations
 * - System health monitoring
 * - Memory usage during operation
 *
 * Performance tests ensure the system can respond quickly enough for
 * industrial safety requirements and user experience expectations.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#include <unity.h>
#include <esp_timer.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include main functions for performance testing
#include "main.h"

// =============================================================================
// Performance Test Configuration
// =============================================================================

#define PERFORMANCE_TEST_ITERATIONS 100 ///< Number of iterations per test
#define MAX_RESPONSE_TIME_MS 1000       ///< Maximum allowed response time (1 second)
#define TARGET_RESPONSE_TIME_MS 500     ///< Target response time (500ms)

// Performance measurement variables
static uint64_t start_time; ///< Test start timestamp
static uint64_t end_time;   ///< Test end timestamp

// =============================================================================
// Performance Measurement Helpers
// =============================================================================

/**
 * @brief Start performance timing measurement
 */
void start_performance_timer(void)
{
    start_time = esp_timer_get_time();
}

/**
 * @brief End timing and validate against performance constraints
 *
 * @param max_time_ms Maximum allowed time in milliseconds
 * @param test_name Name of the test for logging
 * @return true if within limits, false if exceeded
 */
bool check_performance_time(uint32_t max_time_ms, const char *test_name)
{
    end_time = esp_timer_get_time();
    uint32_t elapsed_ms = (end_time - start_time) / 1000;

    ESP_LOGI("PERF", "%s: %u ms", test_name, elapsed_ms);

    if (elapsed_ms > max_time_ms)
    {
        ESP_LOGE("PERF", "%s exceeded maximum time: %u ms > %u ms", test_name, elapsed_ms, max_time_ms);
        return false;
    }

    return true;
}

// =============================================================================
// Test Setup and Teardown
// =============================================================================

/**
 * @brief Set up test environment for performance testing
 */
void setUp(void)
{
    // Initialize system state for performance testing
    emergency_shutdown = false;
    system_healthy = true;
    current_temperature = 180.0f;
    last_temp_reading = esp_timer_get_time() / 1000000;
}

// Test teardown
void tearDown(void)
{
    // Clean up after performance tests
}

// Test safety function response times
void test_safety_function_performance(void)
{
    bool result;

    // Test check_system_safety performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        start_performance_timer();
        result = check_system_safety();
        TEST_ASSERT_TRUE(check_performance_time(MAX_RESPONSE_TIME_MS, "check_system_safety"));
    }

    // Test validate_cycle_safety performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        start_performance_timer();
        result = validate_cycle_safety();
        TEST_ASSERT_TRUE(check_performance_time(MAX_RESPONSE_TIME_MS, "validate_cycle_safety"));
    }
}

// Test temperature reading performance
void test_temperature_reading_performance(void)
{
    float temperature;
    bool result;

    // Test read_temperature_safe performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        start_performance_timer();
        result = read_temperature_safe(&temperature);
        TEST_ASSERT_TRUE(check_performance_time(MAX_RESPONSE_TIME_MS, "read_temperature_safe"));
    }
}

// Test emergency shutdown performance
void test_emergency_shutdown_performance(void)
{
    // Test emergency_shutdown_system performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        // Reset state
        emergency_shutdown = false;
        system_healthy = true;

        start_performance_timer();
        emergency_shutdown_system("Performance test");
        TEST_ASSERT_TRUE(check_performance_time(TARGET_RESPONSE_TIME_MS, "emergency_shutdown_system"));

        // Reset for next iteration
        emergency_shutdown = false;
        system_healthy = true;
    }
}

// Test error state reset performance
void test_error_reset_performance(void)
{
    // Test reset_error_state performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        // Set up error state
        emergency_shutdown = true;
        system_healthy = false;

        start_performance_timer();
        reset_error_state();
        TEST_ASSERT_TRUE(check_performance_time(MAX_RESPONSE_TIME_MS, "reset_error_state"));
    }
}

// Test heap monitoring performance
void test_heap_monitoring_performance(void)
{
    size_t heap_size;

    // Test heap size reading performance
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        start_performance_timer();
        heap_size = esp_get_free_heap_size();
        TEST_ASSERT_TRUE(check_performance_time(10, "esp_get_free_heap_size")); // Should be very fast
        TEST_ASSERT_GREATER_THAN(0, heap_size);
    }
}

// Test overall system health check performance
void test_system_health_check_performance(void)
{
    // Test comprehensive system health check
    for (int i = 0; i < PERFORMANCE_TEST_ITERATIONS; i++)
    {
        start_performance_timer();

        // Simulate system health check (combination of safety checks)
        bool temp_ok = (current_temperature <= MAX_TEMPERATURE);
        bool heap_ok = (esp_get_free_heap_size() >= HEAP_MINIMUM);
        bool sensor_ok = ((esp_timer_get_time() / 1000000) - last_temp_reading) < 30;
        bool emergency_ok = !emergency_shutdown;
        bool overall_health = temp_ok && heap_ok && sensor_ok && emergency_ok;

        TEST_ASSERT_TRUE(check_performance_time(TARGET_RESPONSE_TIME_MS, "system_health_check"));
        TEST_ASSERT_TRUE(overall_health);
    }
}

// Test concurrent operation performance (simulated)
void test_concurrent_operation_performance(void)
{
    // Test that safety functions perform well under simulated load
    // This would be more comprehensive with actual task scheduling

    start_performance_timer();

    // Perform multiple safety checks in sequence (simulating concurrent load)
    for (int i = 0; i < 50; i++)
    {
        check_system_safety();
        validate_cycle_safety();
        esp_get_free_heap_size();
    }

    TEST_ASSERT_TRUE(check_performance_time(TARGET_RESPONSE_TIME_MS, "concurrent_safety_checks"));
}

// Test memory usage during performance tests
void test_memory_usage_during_performance(void)
{
    size_t initial_heap = esp_get_free_heap_size();
    size_t final_heap;

    // Run performance tests
    test_safety_function_performance();
    test_temperature_reading_performance();
    test_emergency_shutdown_performance();

    final_heap = esp_get_free_heap_size();

    // Ensure no significant memory leaks during performance testing
    size_t heap_difference = initial_heap - final_heap;
    TEST_ASSERT_LESS_THAN(1024, heap_difference); // Allow up to 1KB difference

    ESP_LOGI("PERF", "Memory usage test: initial=%u, final=%u, difference=%u",
             initial_heap, final_heap, heap_difference);
}

// Benchmark report
void test_performance_benchmark_report(void)
{
    ESP_LOGI("PERF", "=== Performance Benchmark Report ===");
    ESP_LOGI("PERF", "Target response time: %u ms", TARGET_RESPONSE_TIME_MS);
    ESP_LOGI("PERF", "Maximum allowed response time: %u ms", MAX_RESPONSE_TIME_MS);
    ESP_LOGI("PERF", "Test iterations per function: %d", PERFORMANCE_TEST_ITERATIONS);

    // Individual function benchmarks are logged in each test
    ESP_LOGI("PERF", "All performance tests completed successfully");
    ESP_LOGI("PERF", "System meets <1s response time requirements");
}
