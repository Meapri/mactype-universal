/*
 * MacType Process Bridge
 *
 * Secure cross-architecture process communication
 * - Windows API direct usage (no wow64ext dependency)
 * - Process integrity verification
 * - Memory-safe operations
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace MacType::Agent {

    // Forward declarations
    class SecureChannel;

    // Process information structure
    struct ProcessInfo {
        DWORD processId;
        std::wstring processName;
        std::wstring executablePath;
        bool is64Bit;
        bool isArm64;
        winrt::Windows::Foundation::DateTime startTime;
    };

    // Memory operation result
    struct MemoryOpResult {
        bool success;
        uint64_t address;
        size_t bytesTransferred;
        winrt::hresult errorCode;
    };

    // DLL injection context
    struct InjectionContext {
        DWORD processId;
        std::wstring dllPath;
        uint64_t entryPointAddress;
        std::vector<uint8_t> shellcode;
        bool useRemoteThread;
    };

    // Process bridge main class
    class ProcessBridge {
    private:
        // Windows API handles
        winrt::handle m_processHandle;
        winrt::handle m_threadHandle;

        // Process information
        ProcessInfo m_targetProcess;

        // Security and verification
        std::unique_ptr<SecureChannel> m_secureChannel;
        winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider m_hashProvider{ nullptr };

        // State tracking
        bool m_isAttached = false;
        uint64_t m_remoteModuleBase = 0;

        // Event handlers
        winrt::event_token m_processExitToken;

    public:
        ProcessBridge();
        ~ProcessBridge();

        // Prevent copying
        ProcessBridge(const ProcessBridge&) = delete;
        ProcessBridge& operator=(const ProcessBridge&) = delete;

        // Process attachment and detachment
        winrt::Windows::Foundation::IAsyncAction AttachToProcessAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction DetachFromProcessAsync();

        // Process verification
        winrt::Windows::Foundation::IAsyncAction VerifyProcessIntegrityAsync();
        winrt::Windows::Foundation::IAsyncAction CheckProcessPermissionsAsync();

        // Memory operations
        winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> AllocateRemoteMemoryAsync(size_t size, uint32_t protection);
        winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> FreeRemoteMemoryAsync(uint64_t address);
        winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> WriteRemoteMemoryAsync(uint64_t address, winrt::array_view<uint8_t> data);
        winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> ReadRemoteMemoryAsync(uint64_t address, size_t size);

        // Module operations
        winrt::Windows::Foundation::IAsyncAction LoadRemoteModuleAsync(const std::wstring& modulePath);
        winrt::Windows::Foundation::IAsyncAction UnloadRemoteModuleAsync(uint64_t moduleBase);
        winrt::Windows::Foundation::IAsyncOperation<uint64_t> GetRemoteProcAddressAsync(const std::wstring& moduleName, const std::string& functionName);

        // Thread operations
        winrt::Windows::Foundation::IAsyncAction CreateRemoteThreadAsync(uint64_t startAddress, uint64_t parameter);
        winrt::Windows::Foundation::IAsyncAction SuspendRemoteThreadAsync();
        winrt::Windows::Foundation::IAsyncAction ResumeRemoteThreadAsync();

        // DLL injection
        winrt::Windows::Foundation::IAsyncAction InjectLibraryAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction EjectLibraryAsync(uint64_t moduleBase);

        // Context operations (for x64)
        winrt::Windows::Foundation::IAsyncAction GetThreadContextAsync();
        winrt::Windows::Foundation::IAsyncAction SetThreadContextAsync();

        // Status and information
        bool IsAttached() const noexcept { return m_isAttached; }
        const ProcessInfo& GetProcessInfo() const noexcept { return m_targetProcess; }
        winrt::Windows::Foundation::IAsyncAction GetProcessStatisticsAsync();

    private:
        // Initialization helpers
        winrt::Windows::Foundation::IAsyncAction OpenProcessHandlesAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction CloseProcessHandlesAsync();

        // Process information gathering
        winrt::Windows::Foundation::IAsyncAction GatherProcessInfoAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction DetermineArchitectureAsync();

        // Memory protection helpers
        uint32_t ConvertProtectionFlags(uint32_t nativeFlags);
        bool IsValidMemoryRange(uint64_t address, size_t size);

        // Thread safety
        winrt::Windows::Foundation::IAsyncAction AcquireProcessLockAsync();
        winrt::Windows::Foundation::IAsyncAction ReleaseProcessLockAsync();

        // Error handling and recovery
        winrt::Windows::Foundation::IAsyncAction HandleMemoryErrorAsync(winrt::hresult error);
        winrt::Windows::Foundation::IAsyncAction AttemptMemoryRecoveryAsync();

        // Event handlers
        winrt::Windows::Foundation::IAsyncAction OnProcessExitAsync();
        winrt::Windows::Foundation::IAsyncAction OnThreadExitAsync();

        // Utility methods
        bool IsProcessStillRunning();
        winrt::Windows::Foundation::IAsyncAction ValidateMemoryPointerAsync(uint64_t address);
    };

    // Cross-architecture helper class
    class CrossArchitectureHelper {
    public:
        // Architecture detection
        static winrt::Windows::Foundation::IAsyncOperation<bool> Is64BitProcessAsync(DWORD processId);
        static winrt::Windows::Foundation::IAsyncOperation<bool> IsArm64ProcessAsync(DWORD processId);

        // Context conversion
        static winrt::Windows::Foundation::IAsyncAction ConvertContext32To64Async(PVOID context32, PVOID context64);
        static winrt::Windows::Foundation::IAsyncAction ConvertContext64To32Async(PVOID context64, PVOID context32);

        // Address translation
        static uint64_t TranslateAddress32To64(uint32_t address32);
        static uint32_t TranslateAddress64To32(uint64_t address64);

        // Shellcode generation for different architectures
        static winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateInjectionShellcodeAsync(bool is64Bit);
        static winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateArm64ShellcodeAsync();
    };

} // namespace MacType::Agent
