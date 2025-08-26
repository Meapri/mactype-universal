/**
 * @file test_cache.cpp
 * @brief Unit tests for MacType cache system
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <array>

#include "modern_cpp.h"
#include "cache.h"

// Mock Windows API for testing
class MockGDI {
public:
    static HDC mockDC;
    static HBITMAP mockBitmap;
    static HPALETTE mockPalette;

    static void Reset() {
        mockDC = reinterpret_cast<HDC>(0x1234);
        mockBitmap = reinterpret_cast<HBITMAP>(0x5678);
        mockPalette = reinterpret_cast<HPALETTE>(0x9ABC);
    }
};

// Initialize mock handles
HDC MockGDI::mockDC = nullptr;
HBITMAP MockGDI::mockBitmap = nullptr;
HBITMAP MockGDI::mockPalette = nullptr;

// Mock Windows API functions
extern "C" {
    HDC CreateCompatibleDC(HDC hdc) {
        return MockGDI::mockDC;
    }

    HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* pbmi, UINT usage,
                            VOID** ppvBits, HANDLE hSection, DWORD offset) {
        *ppvBits = new BYTE[1024]; // Mock pixel data
        return MockGDI::mockBitmap;
    }

    HBRUSH CreateSolidBrush(COLORREF color) {
        return reinterpret_cast<HBRUSH>(0xDEAD);
    }

    BOOL DeleteDC(HDC hdc) {
        return TRUE;
    }

    BOOL DeleteBitmap(HBITMAP hbm) {
        return TRUE;
    }

    int FillRect(HDC hdc, const RECT* lprc, HBRUSH hbr) {
        return 1;
    }

    HGDIOBJ GetCurrentObject(HDC hdc, UINT type) {
        return reinterpret_cast<HGDIOBJ>(MockGDI::mockPalette);
    }
}

class CacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockGDI::Reset();
    }

    void TearDown() override {
        // Clean up any allocated resources
    }
};

TEST_F(CacheTest, MemoryPoolBasicOperations) {
    using namespace performance;

    MemoryPool pool(1024, 64 * 1024); // 1KB blocks, 64KB pool

    // Test allocation
    void* ptr1 = pool.allocate(512);
    ASSERT_NE(ptr1, nullptr);

    void* ptr2 = pool.allocate(1024);
    ASSERT_NE(ptr2, nullptr);

    // Test deallocation
    pool.deallocate(ptr1, 512);
    pool.deallocate(ptr2, 1024);

    // Test large allocation (should use direct allocation)
    void* largePtr = pool.allocate(2 * 1024 * 1024); // 2MB
    ASSERT_NE(largePtr, nullptr);
    pool.deallocate(largePtr, 2 * 1024 * 1024);
}

TEST_F(CacheTest, ThreadSafeMemoryPool) {
    using namespace performance;

    ThreadSafeMemoryPool pool;

    // Test concurrent allocations
    std::vector<void*> allocations;
    allocations.reserve(100);

    for (int i = 0; i < 100; ++i) {
        void* ptr = pool.allocate(256);
        ASSERT_NE(ptr, nullptr);
        allocations.push_back(ptr);
    }

    // Clean up
    for (auto ptr : allocations) {
        pool.deallocate(ptr, 256);
    }
}

TEST_F(CacheTest, SIMDUtilities) {
    using namespace performance::simd;

    std::vector<char> src(1024, 'A');
    std::vector<char> dst(1024, 0);

    // Test fast copy
    fast_copy(dst.data(), src.data(), src.size());

    // Verify copy
    EXPECT_EQ(src, dst);

    // Test with different sizes
    std::vector<char> small_src = {'H', 'e', 'l', 'l', 'o'};
    std::vector<char> small_dst(5, 0);

    fast_copy(small_dst.data(), small_src.data(), small_src.size());
    EXPECT_EQ(small_src, small_dst);
}

TEST_F(CacheTest, ColorUtils) {
    using namespace color_utils;

    // Test RGBA function
    COLORREF color1 = RGBA(255, 0, 0, 128); // Semi-transparent red
    COLORREF color2 = RGB(0, 255, 0);       // Solid green

    // Test color components
    EXPECT_EQ(GetRValue(color1), 255);
    EXPECT_EQ(GetGValue(color1), 0);
    EXPECT_EQ(GetBValue(color1), 0);

    EXPECT_EQ(GetRValue(color2), 0);
    EXPECT_EQ(GetGValue(color2), 255);
    EXPECT_EQ(GetBValue(color2), 0);
}

TEST_F(CacheTest, ErrorHandling) {
    using namespace error_handling;

    // Test MacTypeError
    MacTypeError error(ErrorType::MemoryAllocation, "Test error");
    EXPECT_EQ(error.type(), ErrorType::MemoryAllocation);
    EXPECT_EQ(error.message(), "Test error");
    EXPECT_TRUE(error.error_code() == std::error_code());

    std::string fullMsg = error.full_message();
    EXPECT_THAT(fullMsg, ::testing::HasSubstr("Test error"));
}

TEST_F(CacheTest, ExpectedType) {
    using namespace error_handling;

    // Test successful case
    expected<int, MacTypeError> success = 42;
    EXPECT_TRUE(success.has_value());
    EXPECT_EQ(success.value(), 42);

    // Test error case
    expected<int, MacTypeError> failure = MacTypeError(ErrorType::InvalidArgument, "Invalid value");
    EXPECT_FALSE(failure.has_value());

    // Test value_or
    int result = failure.value_or(100);
    EXPECT_EQ(result, 100);
}

TEST_F(CacheTest, PerformanceConstants) {
    using namespace constants;

    // Test performance-related constants
    EXPECT_GT(MAX_CACHE_SIZE, 0u);
    EXPECT_GT(FONT_MAGIC_NUMBER, 0u);
    EXPECT_GT(MAX_CRITICAL_COUNT, 0u);
    EXPECT_GT(BITMAP_REDUCE_COUNTER, 0u);
}

TEST_F(CacheTest, CacheConstants) {
    using namespace cache_constants;

    // Test cache-specific constants
    EXPECT_GT(BITMAP_REDUCE_COUNTER, 0);
    EXPECT_GT(MAX_CACHE_SIZE, 1024u * 1024u); // At least 1MB
    EXPECT_GT(MEMORY_POOL_BLOCK_SIZE, 1024u); // At least 1KB
}

// Integration test for cache system
TEST_F(CacheTest, CacheIntegration) {
    // Test the interaction between different components

    // Test that performance namespace works with error handling
    using namespace performance;
    using namespace error_handling;

    MemoryPool pool;
    void* ptr = pool.allocate(1024);
    ASSERT_NE(ptr, nullptr);

    // Test that we can create expected types with performance utilities
    expected<void*, MacTypeError> result = ptr;
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), ptr);

    pool.deallocate(ptr, 1024);
}

// Performance regression test
TEST_F(CacheTest, DISABLED_PerformanceRegression) {
    // This test is disabled by default as it takes time
    // Run with --gtest_also_run_disabled_tests to execute

    using namespace performance;

    const size_t ITERATIONS = 10000;
    const size_t ALLOCATION_SIZE = 1024;

    MemoryPool pool;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* ptr = pool.allocate(ALLOCATION_SIZE);
        ASSERT_NE(ptr, nullptr);
        pool.deallocate(ptr, ALLOCATION_SIZE);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Memory pool performance: " << ITERATIONS << " allocations/deallocations in "
              << duration.count() << "ms" << std::endl;

    // Basic performance check - should complete within reasonable time
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
}
