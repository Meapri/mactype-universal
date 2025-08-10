#pragma once
/*
 * MacType 현대적 C++ 기능 헤더
 * C++17/20 표준 기능과 최신 개발 패턴을 제공합니다.
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

// 10. 컴파일 타임 상수
namespace constants {
    
    constexpr size_t FONT_MAGIC_NUMBER = 0xA8;
    constexpr size_t MAX_CRITICAL_COUNT = 20;
    constexpr size_t BITMAP_REDUCE_COUNTER = 256;

} // namespace constants
