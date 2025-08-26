/**
 * @file test_main.cpp
 * @brief Main entry point for MacType test suite
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <string>

// Modern C++ utilities for testing
#include "modern_cpp.h"

/**
 * @brief Test environment setup for MacType
 */
class MacTypeTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Initialize test environment
        std::cout << "Setting up MacType test environment...\n";

        // Set up Windows-specific test environment if needed
        #ifdef _WIN32
        // Initialize COM if needed for DirectWrite tests
        #endif

        // Initialize logging for tests
        std::cout << "Test environment initialized.\n";
    }

    void TearDown() override {
        std::cout << "Tearing down MacType test environment...\n";
        // Clean up test environment
    }
};

/**
 * @brief Custom test listener for detailed test output
 */
class MacTypeTestListener : public ::testing::TestEventListener {
public:
    explicit MacTypeTestListener(::testing::TestEventListener* default_listener)
        : default_listener_(default_listener) {}

    void OnTestProgramStart(const ::testing::UnitTest& unit_test) override {
        std::cout << "=== MacType Test Suite Started ===\n";
        std::cout << "Total tests: " << unit_test.total_test_count() << "\n\n";
        default_listener_->OnTestProgramStart(unit_test);
    }

    void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override {
        std::cout << "\n=== MacType Test Suite Completed ===\n";
        std::cout << "Tests run: " << unit_test.total_test_count() << "\n";
        std::cout << "Tests passed: " << unit_test.successful_test_count() << "\n";
        std::cout << "Tests failed: " << unit_test.failed_test_count() << "\n";
        default_listener_->OnTestProgramEnd(unit_test);
    }

    // Delegate other events to default listener
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        default_listener_->OnTestStart(test_info);
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        default_listener_->OnTestEnd(test_info);
    }

    void OnTestPartResult(const ::testing::TestPartResult& result) override {
        default_listener_->OnTestPartResult(result);
    }

    void OnTestSuiteStart(const ::testing::TestSuite& test_suite) override {
        std::cout << "Running test suite: " << test_suite.name() << "\n";
        default_listener_->OnTestSuiteStart(test_suite);
    }

    void OnTestSuiteEnd(const ::testing::TestSuite& test_suite) override {
        std::cout << "Completed test suite: " << test_suite.name() << "\n\n";
        default_listener_->OnTestSuiteEnd(test_suite);
    }

private:
    ::testing::TestEventListener* default_listener_;
};

// Main test entry point
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    // Set up test environment
    ::testing::GTEST_FLAG(detect_leaks) = true;
    ::testing::GTEST_FLAG(throw_on_failure) = true;

    // Set up custom test environment
    ::testing::AddGlobalTestEnvironment(new MacTypeTestEnvironment());

    // Add custom test listener for better output
    auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
    auto default_listener = listeners.Release(listeners.default_result_printer());
    listeners.Append(new MacTypeTestListener(default_listener));

    std::cout << "MacType Test Suite v2.0.0\n";
    std::cout << "Platform: Windows\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n\n";

    return RUN_ALL_TESTS();
}
