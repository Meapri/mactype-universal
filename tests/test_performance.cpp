/**
 * @file test_performance.cpp
 * @brief Performance tests for MacType
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <random>

#include "modern_cpp.h"

// Test fixture for performance tests
class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up performance test environment
    }

    void TearDown() override {
        // Clean up
    }

    // Helper to measure execution time
    template<typename Func>
    std::chrono::milliseconds measure_time(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }
};

TEST_F(PerformanceTest, MemoryPoolAllocation) {
    using namespace performance;

    MemoryPool pool(4096, 1024 * 1024); // 4KB blocks, 1MB pool

    const int NUM_ALLOCATIONS = 1000;
    std::vector<void*> allocations;

    auto time_taken = measure_time([&]() {
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            void* ptr = pool.allocate(2048); // 2KB allocations
            ASSERT_NE(ptr, nullptr);
            allocations.push_back(ptr);
        }

        // Deallocate
        for (auto ptr : allocations) {
            pool.deallocate(ptr, 2048);
        }
    });

    std::cout << "Memory pool allocation test: " << NUM_ALLOCATIONS
              << " allocations in " << time_taken.count() << "ms" << std::endl;

    // Should complete within reasonable time
    EXPECT_LT(time_taken.count(), 100); // Less than 100ms for 1000 allocations
}

TEST_F(PerformanceTest, ThreadSafeMemoryPool) {
    using namespace performance;

    ThreadSafeMemoryPool pool;
    const int NUM_THREADS = 4;
    const int ALLOCATIONS_PER_THREAD = 100;

    auto time_taken = measure_time([&]() {
        std::vector<std::thread> threads;

        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&]() {
                std::vector<void*> allocations;
                for (int i = 0; i < ALLOCATIONS_PER_THREAD; ++i) {
                    void* ptr = pool.allocate(1024);
                    ASSERT_NE(ptr, nullptr);
                    allocations.push_back(ptr);
                }

                // Deallocate
                for (auto ptr : allocations) {
                    pool.deallocate(ptr, 1024);
                }
            });
        }

        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
    });

    std::cout << "Thread-safe memory pool test: " << (NUM_THREADS * ALLOCATIONS_PER_THREAD)
              << " allocations in " << time_taken.count() << "ms" << std::endl;

    // Should complete within reasonable time
    EXPECT_LT(time_taken.count(), 200); // Less than 200ms for concurrent allocations
}

TEST_F(PerformanceTest, SIMDPerformance) {
    using namespace performance::simd;

    const size_t DATA_SIZE = 1024 * 1024; // 1MB
    std::vector<char> src(DATA_SIZE);
    std::vector<char> dst(DATA_SIZE, 0);

    // Fill source with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (auto& byte : src) {
        byte = static_cast<char>(dis(gen));
    }

    auto time_taken = measure_time([&]() {
        fast_copy(dst.data(), src.data(), DATA_SIZE);
    });

    // Verify correctness
    EXPECT_EQ(src, dst);

    std::cout << "SIMD copy performance: " << DATA_SIZE << " bytes in "
              << time_taken.count() << "ms" << std::endl;

    // Should be reasonably fast
    EXPECT_LT(time_taken.count(), 50); // Less than 50ms for 1MB
}

TEST_F(PerformanceTest, StringUtilsPerformance) {
    using namespace string_utils;

    const int NUM_ITERATIONS = 10000;

    // Test string splitting performance
    std::string test_string = "apple,banana,cherry,date,elderberry,fig,grape,honeydew";
    std::vector<std::string> results;

    auto time_taken = measure_time([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            results = split(test_string, ",");
        }
    });

    // Verify result
    ASSERT_EQ(results.size(), 8);
    EXPECT_EQ(results[0], "apple");
    EXPECT_EQ(results[7], "honeydew");

    std::cout << "String split performance: " << NUM_ITERATIONS
              << " splits in " << time_taken.count() << "ms" << std::endl;

    // Should be very fast
    EXPECT_LT(time_taken.count(), 100); // Less than 100ms for 10k splits
}

TEST_F(PerformanceTest, ScopedTimer) {
    using namespace performance;

    bool executed = false;

    auto time_taken = measure_time([&]() {
        ScopedTimer timer(L"Test Operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        executed = true;
    });

    EXPECT_TRUE(executed);
    EXPECT_GE(time_taken.count(), 10); // Should take at least 10ms
    EXPECT_LT(time_taken.count(), 50); // Should not take too long
}

TEST_F(PerformanceTest, ErrorHandlingPerformance) {
    using namespace error_handling;

    const int NUM_ITERATIONS = 100000;

    auto time_taken = measure_time([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            expected<int, MacTypeError> result = 42;
            int value = result.value_or(0);
            EXPECT_EQ(value, 42);
        }
    });

    std::cout << "Error handling performance: " << NUM_ITERATIONS
              << " operations in " << time_taken.count() << "ms" << std::endl;

    // Should be very fast
    EXPECT_LT(time_taken.count(), 50); // Less than 50ms for 100k operations
}

TEST_F(PerformanceTest, RAIIWrapperPerformance) {
    using namespace error_handling;

    const int NUM_ITERATIONS = 10000;

    auto time_taken = measure_time([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            // Test RAII wrapper creation and destruction
            HDC mockDC = reinterpret_cast<HDC>(0x1234);
            UniqueHDC wrapper(mockDC, DeleteDC);
            EXPECT_EQ(wrapper.get(), mockDC);
        }
    });

    std::cout << "RAII wrapper performance: " << NUM_ITERATIONS
              << " wrappers in " << time_taken.count() << "ms" << std::endl;

    // Should be very fast
    EXPECT_LT(time_taken.count(), 10); // Less than 10ms for 10k wrappers
}

// Memory usage test
TEST_F(PerformanceTest, MemoryUsage) {
    using namespace performance;

    // Test memory pool memory efficiency
    MemoryPool pool(1024, 64 * 1024); // 1KB blocks, 64KB pool

    // Allocate maximum possible
    std::vector<void*> allocations;
    size_t total_allocated = 0;

    while (total_allocated < 1024 * 1024) { // 1MB limit
        void* ptr = pool.allocate(1024);
        if (!ptr) break;

        allocations.push_back(ptr);
        total_allocated += 1024;
    }

    // Should have allocated a reasonable amount
    EXPECT_GT(allocations.size(), 0);
    EXPECT_LE(allocations.size(), 1024); // Should not exceed reasonable limit

    // Clean up
    for (auto ptr : allocations) {
        pool.deallocate(ptr, 1024);
    }

    std::cout << "Memory pool efficiency: " << allocations.size()
              << " blocks allocated (" << total_allocated << " bytes)" << std::endl;
}

// Benchmark-style test
TEST_F(PerformanceTest, DISABLED_BenchmarkComparison) {
    // Disabled by default - run with --gtest_also_run_disabled_tests

    std::cout << "\n=== Performance Benchmark Results ===" << std::endl;

    // Test 1: Memory allocation comparison
    {
        const int ITERATIONS = 10000;

        auto standard_time = measure_time([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                void* ptr = malloc(1024);
                free(ptr);
            }
        });

        performance::MemoryPool pool;
        auto pool_time = measure_time([&]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                void* ptr = pool.allocate(1024);
                pool.deallocate(ptr, 1024);
            }
        });

        std::cout << "Memory allocation comparison:" << std::endl;
        std::cout << "  Standard malloc/free: " << standard_time.count() << "ms" << std::endl;
        std::cout << "  Memory pool: " << pool_time.count() << "ms" << std::endl;
        std::cout << "  Improvement: " << (standard_time.count() * 100.0 / pool_time.count()) << "%" << std::endl;
    }

    std::cout << std::endl;
}
