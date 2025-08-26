/*
 * WinRT Helper Utilities
 *
 * Helper functions for WinRT and Windows API interoperability
 * - Handle conversions
 * - String conversions
 * - Async operation helpers
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.Threading.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace MacType::Agent::WinRT {

    // Handle wrapper for RAII
    class HandleWrapper {
    private:
        HANDLE m_handle;

    public:
        explicit HandleWrapper(HANDLE handle = nullptr) noexcept;
        ~HandleWrapper() noexcept;

        // Prevent copying
        HandleWrapper(const HandleWrapper&) = delete;
        HandleWrapper& operator=(const HandleWrapper&) = delete;

        // Move semantics
        HandleWrapper(HandleWrapper&& other) noexcept;
        HandleWrapper& operator=(HandleWrapper&& other) noexcept;

        // Handle operations
        HANDLE Get() const noexcept { return m_handle; }
        void Reset(HANDLE handle = nullptr) noexcept;
        HANDLE Release() noexcept;
        bool IsValid() const noexcept { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }

        // Boolean conversion for convenience
        explicit operator bool() const noexcept { return IsValid(); }
    };

    // Process handle wrapper
    class ProcessHandle : public HandleWrapper {
    public:
        ProcessHandle(DWORD processId, DWORD desiredAccess);
        ProcessHandle(HANDLE handle);

        DWORD GetProcessId() const;
        bool Is64BitProcess() const;
        winrt::Windows::Foundation::IAsyncAction WaitForExitAsync();
    };

    // Thread handle wrapper
    class ThreadHandle : public HandleWrapper {
    public:
        ThreadHandle(DWORD threadId, DWORD desiredAccess);
        ThreadHandle(HANDLE handle);

        DWORD GetThreadId() const;
        winrt::Windows::Foundation::IAsyncAction WaitForExitAsync();
    };

    // String conversion utilities
    class StringUtils {
    public:
        // ANSI to UTF-8
        static std::string AnsiToUtf8(const std::string& ansi);
        static std::string AnsiToUtf8(const char* ansi, size_t length);

        // UTF-8 to ANSI
        static std::string Utf8ToAnsi(const std::string& utf8);
        static std::string Utf8ToAnsi(const char* utf8, size_t length);

        // Wide string to UTF-8
        static std::string WideToUtf8(const std::wstring& wide);
        static std::string WideToUtf8(const wchar_t* wide, size_t length);

        // UTF-8 to wide string
        static std::wstring Utf8ToWide(const std::string& utf8);
        static std::wstring Utf8ToWide(const char* utf8, size_t length);

        // WinRT hstring conversions
        static winrt::hstring Utf8ToHString(const std::string& utf8);
        static std::string HStringToUtf8(const winrt::hstring& hstr);

        // Safe string formatting
        static std::string FormatString(const char* format, ...);
        static std::wstring FormatString(const wchar_t* format, ...);
    };

    // Async operation helpers
    class AsyncHelper {
    public:
        // Convert Windows async to WinRT async
        static winrt::Windows::Foundation::IAsyncAction CreateAsyncAction(
            std::function<void(winrt::Windows::Foundation::IAsyncAction const&)> action);

        static winrt::Windows::Foundation::IAsyncOperation<bool> CreateAsyncOperation(
            std::function<bool()> operation);

        // Run on background thread
        static winrt::Windows::Foundation::IAsyncAction RunOnBackgroundAsync(
            winrt::Windows::System::Threading::WorkItemPriority priority,
            std::function<void()> workItem);

        // Create cancellable operation
        static winrt::Windows::Foundation::IAsyncActionWithProgress<int> CreateCancellableOperation(
            std::function<void(const winrt::Windows::Foundation::IAsyncOperationWithProgress<int>&)> operation);

        // Timeout wrapper
        static winrt::Windows::Foundation::IAsyncAction WithTimeoutAsync(
            winrt::Windows::Foundation::IAsyncAction action,
            winrt::Windows::Foundation::TimeSpan timeout);
    };

    // File I/O utilities
    class FileUtils {
    public:
        // Async file operations
        static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile>
            CreateFileAsync(const std::wstring& path,
                           winrt::Windows::Storage::CreationCollisionOption options =
                           winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);

        static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile>
            GetFileAsync(const std::wstring& path);

        static winrt::Windows::Foundation::IAsyncAction WriteTextAsync(
            const winrt::Windows::Storage::StorageFile& file,
            const std::string& content);

        static winrt::Windows::Foundation::IAsyncOperation<std::string> ReadTextAsync(
            const winrt::Windows::Storage::StorageFile& file);

        static winrt::Windows::Foundation::IAsyncAction WriteBytesAsync(
            const winrt::Windows::Storage::StorageFile& file,
            winrt::array_view<uint8_t> data);

        static winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> ReadBytesAsync(
            const winrt::Windows::Storage::StorageFile& file);

        // File existence and attributes
        static winrt::Windows::Foundation::IAsyncOperation<bool> FileExistsAsync(const std::wstring& path);
        static winrt::Windows::Foundation::IAsyncOperation<uint64_t> GetFileSizeAsync(const std::wstring& path);
        static winrt::Windows::Foundation::IAsyncAction DeleteFileAsync(const std::wstring& path);

        // Directory operations
        static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder>
            CreateDirectoryAsync(const std::wstring& path);

        static winrt::Windows::Foundation::IAsyncOperation<std::vector<winrt::Windows::Storage::StorageFile>>
            GetFilesInDirectoryAsync(const std::wstring& path);
    };

    // Registry utilities
    class RegistryUtils {
    public:
        // Registry key operations
        static winrt::Windows::Foundation::IAsyncOperation<bool> KeyExistsAsync(const std::wstring& keyPath);
        static winrt::Windows::Foundation::IAsyncAction CreateKeyAsync(const std::wstring& keyPath);
        static winrt::Windows::Foundation::IAsyncAction DeleteKeyAsync(const std::wstring& keyPath);

        // Value operations
        static winrt::Windows::Foundation::IAsyncOperation<std::wstring> ReadStringValueAsync(
            const std::wstring& keyPath, const std::wstring& valueName);

        static winrt::Windows::Foundation::IAsyncAction WriteStringValueAsync(
            const std::wstring& keyPath, const std::wstring& valueName, const std::wstring& value);

        static winrt::Windows::Foundation::IAsyncOperation<uint32_t> ReadDWordValueAsync(
            const std::wstring& keyPath, const std::wstring& valueName);

        static winrt::Windows::Foundation::IAsyncAction WriteDWordValueAsync(
            const std::wstring& keyPath, const std::wstring& valueName, uint32_t value);

        // Enumeration
        static winrt::Windows::Foundation::IAsyncOperation<std::vector<std::wstring>>
            EnumerateSubKeysAsync(const std::wstring& keyPath);

        static winrt::Windows::Foundation::IAsyncOperation<std::vector<std::wstring>>
            EnumerateValuesAsync(const std::wstring& keyPath);
    };

    // Process utilities
    class ProcessUtils {
    public:
        // Process enumeration
        static winrt::Windows::Foundation::IAsyncOperation<std::vector<DWORD>> GetProcessIdsAsync();
        static winrt::Windows::Foundation::IAsyncOperation<std::vector<std::pair<DWORD, std::wstring>>>
            GetProcessListAsync();

        // Process information
        static winrt::Windows::Foundation::IAsyncOperation<std::wstring> GetProcessNameAsync(DWORD processId);
        static winrt::Windows::Foundation::IAsyncOperation<std::wstring> GetProcessPathAsync(DWORD processId);
        static winrt::Windows::Foundation::IAsyncOperation<bool> Is64BitProcessAsync(DWORD processId);

        // Process manipulation
        static winrt::Windows::Foundation::IAsyncOperation<bool> TerminateProcessAsync(DWORD processId, uint32_t exitCode);
        static winrt::Windows::Foundation::IAsyncOperation<bool> SuspendProcessAsync(DWORD processId);
        static winrt::Windows::Foundation::IAsyncOperation<bool> ResumeProcessAsync(DWORD processId);

        // Memory information
        static winrt::Windows::Foundation::IAsyncOperation<size_t> GetProcessMemoryUsageAsync(DWORD processId);
        static winrt::Windows::Foundation::IAsyncOperation<size_t> GetProcessWorkingSetAsync(DWORD processId);
    };

    // Security utilities
    class SecurityUtils {
    public:
        // Token operations
        static winrt::Windows::Foundation::IAsyncAction EnablePrivilegeAsync(const std::wstring& privilegeName);
        static winrt::Windows::Foundation::IAsyncAction RevertToSelfAsync();

        // Integrity level
        static winrt::Windows::Foundation::IAsyncOperation<bool> IsRunningAsAdministratorAsync();
        static winrt::Windows::Foundation::IAsyncOperation<bool> IsRunningAsSystemAsync();

        // Process security
        static winrt::Windows::Foundation::IAsyncAction GrantProcessAccessAsync(DWORD processId, DWORD desiredAccess);
        static winrt::Windows::Foundation::IAsyncAction VerifyProcessTrustAsync(DWORD processId);

        // DLL security
        static winrt::Windows::Foundation::IAsyncOperation<bool> VerifyDllSignatureAsync(const std::wstring& dllPath);
        static winrt::Windows::Foundation::IAsyncOperation<std::string> GetDllCertificateInfoAsync(const std::wstring& dllPath);
    };

    // Error handling utilities
    class ErrorUtils {
    public:
        // Error code conversions
        static winrt::hresult HResultFromLastError();
        static winrt::hresult HResultFromWin32(DWORD error);
        static DWORD Win32FromHResult(winrt::hresult hr);

        // Error message formatting
        static std::string FormatErrorMessage(DWORD error);
        static std::string FormatErrorMessage(winrt::hresult hr);
        static winrt::hstring FormatErrorMessageHString(DWORD error);
        static winrt::hstring FormatErrorMessageHString(winrt::hresult hr);

        // Exception handling
        static winrt::Windows::Foundation::IAsyncAction HandleExceptionAsync(
            const std::exception& ex, const std::string& context);

        static winrt::Windows::Foundation::IAsyncAction HandleWin32ErrorAsync(
            DWORD error, const std::string& context);

        static winrt::Windows::Foundation::IAsyncAction HandleHResultErrorAsync(
            winrt::hresult hr, const std::string& context);
    };

    // Timer utilities
    class TimerUtils {
    public:
        // High-resolution timer
        static winrt::Windows::Foundation::IAsyncAction DelayAsync(winrt::Windows::Foundation::TimeSpan delay);
        static winrt::Windows::Foundation::IAsyncAction DelayAsync(std::chrono::milliseconds delay);

        // Periodic timer
        static winrt::Windows::System::Threading::ThreadPoolTimer CreatePeriodicTimer(
            winrt::Windows::Foundation::TimeSpan period,
            winrt::Windows::System::Threading::TimerElapsedHandler const& handler);

        // One-shot timer
        static winrt::Windows::System::Threading::ThreadPoolTimer CreateOneShotTimer(
            winrt::Windows::Foundation::TimeSpan delay,
            winrt::Windows::System::Threading::TimerElapsedHandler const& handler);

        // Performance measurement
        class PerformanceTimer {
        private:
            std::chrono::high_resolution_clock::time_point m_start;
            std::string m_operationName;

        public:
            explicit PerformanceTimer(const std::string& operationName = "");
            ~PerformanceTimer();

            void Reset();
            double GetElapsedMilliseconds() const;
            double GetElapsedMicroseconds() const;
            std::string GetReport() const;
        };
    };

} // namespace MacType::Agent::WinRT
