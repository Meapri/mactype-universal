#pragma once
/*
 * @file modern_cpp.h
 * @brief MacType Modern C++ Utilities
 *
 * This header provides modern C++17/20 features and patterns for MacType.
 * It includes utilities for:
 * - Memory management (RAII, smart pointers)
 * - Error handling (expected types, error codes)
 * - Performance optimization (memory pools, SIMD)
 * - String processing (modern string views)
 * - Threading (thread-safe operations)
 *
 * @author MacType Development Team
 * @version 2.0.0
 * @date 2024
 */

#include "sdk_compat.h"
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <variant>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <algorithm>
#include <functional>
#include <utility>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <cwctype>
#include <cctype>
#include <locale>
#include <array>

// 1. 스마트 포인터 타입 별칭 (가독성 향상)
template<typename T>
using unique_ptr = std::unique_ptr<T>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;

template<typename T>
using weak_ptr = std::weak_ptr<T>;

// 2. 현대적 문자열 처리 타입
using string_view = std::string_view;
using wstring_view = std::wstring_view;

namespace fs = std::filesystem;
namespace chrono = std::chrono;

// 3. 메모리 안전 factory 함수들
template<typename T, typename... Args>
[[nodiscard]] constexpr auto make_unique(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
[[nodiscard]] constexpr auto make_shared(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// 4. RAII 기반 리소스 관리 유틸리티
template<typename Resource, typename Deleter>
class ScopedResource
{
public:
    explicit ScopedResource(Resource&& resource, Deleter&& deleter) noexcept
        : resource_(std::move(resource)), deleter_(std::move(deleter))
    {
    }

    ~ScopedResource() noexcept
    {
        if (resource_) {
            deleter_(resource_);
        }
    }

    // 복사 불가, 이동 가능
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource& operator=(const ScopedResource&) = delete;

    ScopedResource(ScopedResource&& other) noexcept
        : resource_(std::exchange(other.resource_, {}))
        , deleter_(std::move(other.deleter_))
    {
    }

    ScopedResource& operator=(ScopedResource&& other) noexcept
    {
        if (this != &other) {
            reset();
            resource_ = std::exchange(other.resource_, {});
            deleter_ = std::move(other.deleter_);
        }
        return *this;
    }

    [[nodiscard]] Resource& get() noexcept { return resource_; }
    [[nodiscard]] const Resource& get() const noexcept { return resource_; }

    Resource release() noexcept { return std::exchange(resource_, {}); }

    void reset() noexcept
    {
        if (resource_) {
            deleter_(resource_);
            resource_ = {};
        }
    }

    explicit operator bool() const noexcept { return static_cast<bool>(resource_); }

private:
    Resource resource_;
    Deleter deleter_;
};

// 5. Windows 핸들 관리를 위한 특화된 RAII 클래스들
class ScopedHandle
{
public:
    explicit ScopedHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept : handle_(handle) {}
    
    ~ScopedHandle() noexcept
    {
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr) {
            CloseHandle(handle_);
        }
    }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept : handle_(std::exchange(other.handle_, INVALID_HANDLE_VALUE)) {}
    
    ScopedHandle& operator=(ScopedHandle&& other) noexcept
    {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, INVALID_HANDLE_VALUE);
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept { return handle_; }
    [[nodiscard]] HANDLE* get_address() noexcept { return &handle_; }
    
    HANDLE release() noexcept { return std::exchange(handle_, INVALID_HANDLE_VALUE); }
    
    void reset(HANDLE new_handle = INVALID_HANDLE_VALUE) noexcept
    {
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr) {
            CloseHandle(handle_);
        }
        handle_ = new_handle;
    }

    explicit operator bool() const noexcept 
    { 
        return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr; 
    }

private:
    HANDLE handle_;
};

class ScopedHdc
{
public:
    explicit ScopedHdc(HDC hdc = nullptr, HWND hwnd = nullptr) noexcept 
        : hdc_(hdc), hwnd_(hwnd) {}
    
    ~ScopedHdc() noexcept
    {
        if (hdc_) {
            if (hwnd_) {
                ReleaseDC(hwnd_, hdc_);
            } else {
                DeleteDC(hdc_);
            }
        }
    }

    ScopedHdc(const ScopedHdc&) = delete;
    ScopedHdc& operator=(const ScopedHdc&) = delete;

    ScopedHdc(ScopedHdc&& other) noexcept 
        : hdc_(std::exchange(other.hdc_, nullptr))
        , hwnd_(std::exchange(other.hwnd_, nullptr)) {}
    
    ScopedHdc& operator=(ScopedHdc&& other) noexcept
    {
        if (this != &other) {
            reset();
            hdc_ = std::exchange(other.hdc_, nullptr);
            hwnd_ = std::exchange(other.hwnd_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] HDC get() const noexcept { return hdc_; }
    
    HDC release() noexcept 
    { 
        hwnd_ = nullptr;
        return std::exchange(hdc_, nullptr); 
    }
    
    void reset(HDC new_hdc = nullptr, HWND new_hwnd = nullptr) noexcept
    {
        if (hdc_) {
            if (hwnd_) {
                ReleaseDC(hwnd_, hdc_);
            } else {
                DeleteDC(hdc_);
            }
        }
        hdc_ = new_hdc;
        hwnd_ = new_hwnd;
    }

    explicit operator bool() const noexcept { return hdc_ != nullptr; }

private:
    HDC hdc_;
    HWND hwnd_;
};

// 6. 현대적 동시성 유틸리티
using Mutex = std::mutex;
using SharedMutex = std::shared_mutex;
using RecursiveMutex = std::recursive_mutex;

template<typename Mutex>
using LockGuard = std::lock_guard<Mutex>;

template<typename Mutex>
using UniqueLock = std::unique_lock<Mutex>;

template<typename Mutex>
using SharedLock = std::shared_lock<Mutex>;

// 7. 현대적 문자열 변환 유틸리티
namespace string_utils {

    // UTF-8 <-> UTF-16 변환 (std::codecvt 대체)
    [[nodiscard]] std::wstring utf8_to_utf16(string_view utf8_str);
    [[nodiscard]] std::string utf16_to_utf8(wstring_view utf16_str);

    // 대소문자 변환 (locale 안전)
    [[nodiscard]] std::wstring to_lower(wstring_view str);
    [[nodiscard]] std::wstring to_upper(wstring_view str);
    
    // 문자열 분할 (ranges 기반)
    [[nodiscard]] std::vector<std::wstring> split(wstring_view str, wstring_view delimiter);
    
    // 문자열 결합
    template<typename Container>
    [[nodiscard]] std::wstring join(const Container& strings, wstring_view separator);

    // 문자열 검색 (대소문자 구분 없음)
    [[nodiscard]] bool contains_ignore_case(wstring_view str, wstring_view substr);
    
    // 앞뒤 공백 제거
    [[nodiscard]] std::wstring trim(wstring_view str);
    [[nodiscard]] std::wstring trim_left(wstring_view str);
    [[nodiscard]] std::wstring trim_right(wstring_view str);

} // namespace string_utils

// 8. 에러 처리 개선
namespace error_handling {

    // Result 타입 (std::expected가 C++23이므로 간단한 구현)
    template<typename T, typename E = std::error_code>
    class Result
    {
    public:
        constexpr Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : value_(std::move(value)), has_value_(true) {}
        
        constexpr Result(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
            : value_(value), has_value_(true) {}

        constexpr Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
            : error_(std::move(error)), has_value_(false) {}
        
        constexpr Result(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>)
            : error_(error), has_value_(false) {}

        [[nodiscard]] constexpr bool has_value() const noexcept { return has_value_; }
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value_; }

        [[nodiscard]] constexpr T& value() & { return value_; }
        [[nodiscard]] constexpr const T& value() const & { return value_; }
        [[nodiscard]] constexpr T&& value() && { return std::move(value_); }

        [[nodiscard]] constexpr E& error() & { return error_; }
        [[nodiscard]] constexpr const E& error() const & { return error_; }
        [[nodiscard]] constexpr E&& error() && { return std::move(error_); }

        template<typename U>
        [[nodiscard]] constexpr T value_or(U&& default_value) const &
        {
            return has_value_ ? value_ : static_cast<T>(std::forward<U>(default_value));
        }

    private:
        union {
            T value_;
            E error_;
        };
        bool has_value_;
    };

    // Windows API 에러를 std::error_code로 변환
    [[nodiscard]] inline std::error_code last_error() noexcept
    {
        return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
    }

    // HRESULT를 std::error_code로 변환
    [[nodiscard]] inline std::error_code from_hresult(HRESULT hr) noexcept
    {
        return std::error_code{ hr, std::system_category() };
    }

    // Enhanced error types
    enum class ErrorType {
        WindowsAPI,
        MemoryAllocation,
        InvalidArgument,
        FileSystem,
        FontEngine,
        HookFailure,
        Unknown
    };

    class MacTypeError {
    private:
        ErrorType type_;
        std::string message_;
        std::error_code error_code_;

    public:
        MacTypeError(ErrorType type, std::string message, std::error_code ec = {})
            : type_(type), message_(std::move(message)), error_code_(ec) {}

        [[nodiscard]] ErrorType type() const noexcept { return type_; }
        [[nodiscard]] const std::string& message() const noexcept { return message_; }
        [[nodiscard]] const std::error_code& error_code() const noexcept { return error_code_; }

        [[nodiscard]] std::string full_message() const {
            std::string result = message_;
            if (error_code_) {
                result += " (Error: " + error_code_.message() + ")";
            }
            return result;
        }
    };

    // RAII wrapper for Windows handles
    template<typename HandleType, typename Deleter>
    class HandleWrapper {
    private:
        HandleType handle_;
        Deleter deleter_;

    public:
        explicit HandleWrapper(HandleType handle, Deleter deleter) noexcept
            : handle_(handle), deleter_(deleter) {}

        ~HandleWrapper() noexcept {
            if (handle_) {
                deleter_(handle_);
            }
        }

        // Prevent copying
        HandleWrapper(const HandleWrapper&) = delete;
        HandleWrapper& operator=(const HandleWrapper&) = delete;

        // Allow moving
        HandleWrapper(HandleWrapper&& other) noexcept
            : handle_(std::exchange(other.handle_, {}))
            , deleter_(std::move(other.deleter_)) {}

        HandleWrapper& operator=(HandleWrapper&& other) noexcept {
            if (this != &other) {
                reset();
                handle_ = std::exchange(other.handle_, {});
                deleter_ = std::move(other.deleter_);
            }
            return *this;
        }

        [[nodiscard]] HandleType get() const noexcept { return handle_; }
        [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

        void reset() noexcept {
            if (handle_) {
                deleter_(handle_);
                handle_ = {};
            }
        }

        [[nodiscard]] HandleType release() noexcept {
            return std::exchange(handle_, {});
        }
    };

    // Common handle wrappers
    using UniqueHandle = HandleWrapper<HANDLE, decltype(&CloseHandle)>;
    using UniqueHDC = HandleWrapper<HDC, decltype(&DeleteDC)>;
    using UniqueHBITMAP = HandleWrapper<HBITMAP, decltype(&DeleteBitmap)>;

    // Safe Windows API call wrapper
    template<typename Fn, typename... Args>
    [[nodiscard]] expected<typename std::invoke_result_t<Fn, Args...>, MacTypeError>
    safe_api_call(Fn&& fn, Args&&... args) noexcept {
        try {
            auto result = std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
            if constexpr (std::is_same_v<decltype(result), BOOL>) {
                if (result == FALSE) {
                    return MacTypeError{ErrorType::WindowsAPI, "Windows API call failed", last_error()};
                }
                return true;
            } else if constexpr (std::is_same_v<decltype(result), HRESULT>) {
                if (FAILED(result)) {
                    return MacTypeError{ErrorType::WindowsAPI, "COM API call failed", from_hresult(result)};
                }
                return result;
            } else {
                return result;
            }
        } catch (const std::exception& e) {
            return MacTypeError{ErrorType::Unknown, std::string("Exception in API call: ") + e.what()};
        }
    }

} // namespace error_handling

// 9. 성능 측정 유틸리티
namespace performance {

    class ScopedTimer
    {
    public:
        explicit ScopedTimer(const std::wstring& name) 
            : name_(name), start_(chrono::high_resolution_clock::now()) {}
        
        ~ScopedTimer()
        {
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(end - start_);
            // 로깅 시스템에 출력 (향후 구현)
        }

    private:
        std::wstring name_;
        chrono::high_resolution_clock::time_point start_;
    };

    // 사용법: PERF_TIMER(L"함수명")
    #define PERF_TIMER(name) performance::ScopedTimer _timer(name)

} // namespace performance

// 10. 메모리 및 성능 최적화 유틸리티
namespace performance {

// Memory pool for frequent allocations
class MemoryPool {
private:
    struct Block {
        void* data;
        size_t size;
        bool in_use;
        Block* next;
    };

    Block* free_list_ = nullptr;
    size_t block_size_;
    size_t pool_size_;
    std::vector<std::unique_ptr<char[]>> pools_;

public:
    explicit MemoryPool(size_t block_size = 4096, size_t pool_size = 1024 * 1024)
        : block_size_(block_size), pool_size_(pool_size) {}

    ~MemoryPool() noexcept {
        clear();
    }

    void* allocate(size_t size) {
        if (size > block_size_) {
            // For large allocations, use direct allocation
            return new char[size];
        }

        // Try to find a free block
        if (free_list_) {
            Block* block = free_list_;
            free_list_ = block->next;
            block->in_use = true;
            return block->data;
        }

        // Allocate new pool
        auto new_pool = std::make_unique<char[]>(pool_size_);
        char* pool_data = new_pool.get();
        pools_.push_back(std::move(new_pool));

        // Create blocks from the new pool
        size_t num_blocks = pool_size_ / block_size_;
        for (size_t i = 0; i < num_blocks; ++i) {
            Block* block = new Block{
                pool_data + i * block_size_,
                block_size_,
                false,
                free_list_
            };
            free_list_ = block;
        }

        // Allocate from the new free list
        if (free_list_) {
            Block* block = free_list_;
            free_list_ = block->next;
            block->in_use = true;
            return block->data;
        }

        return nullptr; // Should not reach here
    }

    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;

        if (size > block_size_) {
            delete[] static_cast<char*>(ptr);
            return;
        }

        // Find the block and mark it as free
        // This is a simplified version - in production, you'd want a better data structure
        for (auto& pool : pools_) {
            char* pool_start = pool.get();
            char* pool_end = pool_start + pool_size_;

            if (ptr >= pool_start && ptr < pool_end) {
                Block* new_block = new Block{
                    ptr,
                    block_size_,
                    false,
                    free_list_
                };
                free_list_ = new_block;
                return;
            }
        }
    }

    void clear() noexcept {
        while (free_list_) {
            Block* block = free_list_;
            free_list_ = block->next;
            delete block;
        }
        pools_.clear();
    }
};

// Thread-safe memory pool wrapper
class ThreadSafeMemoryPool {
private:
    MemoryPool pool_;
    std::mutex mutex_;

public:
    void* allocate(size_t size) {
        std::lock_guard lock(mutex_);
        return pool_.allocate(size);
    }

    void deallocate(void* ptr, size_t size) {
        std::lock_guard lock(mutex_);
        pool_.deallocate(ptr, size);
    }
};

// SIMD utilities for font rendering optimization
namespace simd {

#ifdef __AVX2__
    // AVX2-optimized memory operations
    inline void fast_copy(void* dst, const void* src, size_t size) {
        size_t i = 0;
        for (; i + 32 <= size; i += 32) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(static_cast<const char*>(src) + i));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(static_cast<char*>(dst) + i), data);
        }
        // Handle remaining bytes
        for (; i < size; ++i) {
            static_cast<char*>(dst)[i] = static_cast<const char*>(src)[i];
        }
    }
#else
    inline void fast_copy(void* dst, const void* src, size_t size) {
        std::memcpy(dst, src, size);
    }
#endif

} // namespace simd

} // namespace performance

// 11. 레거시 호환성 유틸리티
namespace legacy_compat {

    // Modern string tokenizer for legacy TCHAR support
    class StringTokenizer {
    private:
        std::vector<std::string> tokens_;
        size_t current_index_ = 0;

    public:
        StringTokenizer() = default;

        // Parse comma-separated string (legacy TCHAR support)
        int Parse(std::string_view str) {
            tokens_ = string_utils::split(std::string(str), ",");
            current_index_ = 0;

            // Remove empty tokens at the end for legacy compatibility
            while (!tokens_.empty() && tokens_.back().empty()) {
                tokens_.pop_back();
            }

            return static_cast<int>(tokens_.size());
        }

        // Legacy TCHAR-based parse
        int Parse(LPCTSTR pszStr) {
            if (!pszStr) return 0;
            return Parse(std::string_view(pszStr));
        }

        int GetCount() const {
            return static_cast<int>(tokens_.size());
        }

        LPCTSTR GetArgument(int index) const {
            if (index < 0 || static_cast<size_t>(index) >= tokens_.size()) {
                return nullptr;
            }

            // Store in static buffer for legacy compatibility
            static std::string buffer;
            buffer = tokens_[static_cast<size_t>(index)];
            return buffer.c_str();
        }

        void Reset() {
            tokens_.clear();
            current_index_ = 0;
        }
    };

    // Legacy split function replacement
    inline std::vector<int> SplitString(std::string_view str) {
        auto tokens = string_utils::split(std::string(str), ",");
        std::vector<int> result;
        result.reserve(tokens.size());

        for (const auto& token : tokens) {
            if (!token.empty()) {
                result.push_back(std::stoi(std::string(token)));
            }
        }
        return result;
    }

} // namespace legacy_compat

// 11. 컴파일 타임 상수
namespace constants {

    constexpr size_t FONT_MAGIC_NUMBER = 0xA8;
    constexpr size_t MAX_CRITICAL_COUNT = 20;
    constexpr size_t BITMAP_REDUCE_COUNTER = 256;

} // namespace constants
