/*
 * MacType Modern Injector
 *
 * Safe DLL injection without wow64ext dependency
 * - Windows API direct usage
 * - Process integrity verification
 * - Architecture-aware injection
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Security.Cryptography.Core.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace MacType::Agent {

    // Forward declarations
    struct InjectionContext;
    class ProcessBridge;

    // Injection method enumeration
    enum class InjectionMethod {
        LoadLibrary,           // Standard LoadLibrary
        ManualMap,            // Manual DLL mapping
        Reflective,           // Reflective DLL injection
        ThreadHijack,        // Thread execution hijacking
        APCInjection,        // APC queue injection
        EarlyBirdAPC        // Early bird APC injection
    };

    // Architecture-specific information
    struct ArchitectureInfo {
        bool is64Bit;
        bool isArm64;
        std::string architectureName;
        uint32_t pointerSize; // 4 or 8 bytes
        uint64_t kernel32Base;
        uint64_t loadLibraryAddress;
        std::vector<uint8_t> loaderStub;
    };

    // DLL verification result
    struct DllVerificationResult {
        bool isValid;
        bool isSigned;
        std::string certificateIssuer;
        std::string certificateSubject;
        winrt::Windows::Foundation::DateTime certificateExpiry;
        std::vector<std::string> dependencies;
        uint64_t entryPointAddress;
        std::string hashSHA256;
    };

    // Injection result
    struct InjectionResult {
        bool success;
        uint64_t moduleBase;
        uint32_t moduleSize;
        winrt::hresult errorCode;
        std::string errorMessage;
        winrt::Windows::Foundation::DateTime injectionTime;
        double injectionDurationMs;
    };

    // Modern injector main class
    class ModernInjector {
    private:
        // Core components
        std::unique_ptr<ProcessBridge> m_processBridge;
        winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider m_hashProvider{ nullptr };

        // Architecture information
        ArchitectureInfo m_architectureInfo;

        // Injection settings
        InjectionMethod m_preferredMethod = InjectionMethod::LoadLibrary;
        bool m_verifyDllSignature = true;
        bool m_enableAntiAntiDebug = true;
        uint32_t m_injectionTimeoutMs = 10000; // 10 seconds

        // State tracking
        bool m_isInitialized = false;
        std::atomic<bool> m_injectionInProgress = false;

        // Event handlers
        winrt::event_token m_injectionCompletedToken;

    public:
        ModernInjector();
        ~ModernInjector();

        // Prevent copying
        ModernInjector(const ModernInjector&) = delete;
        ModernInjector& operator=(const ModernInjector&) = delete;

        // Initialization
        winrt::Windows::Foundation::IAsyncAction InitializeAsync();
        winrt::Windows::Foundation::IAsyncAction InitializeForProcessAsync(DWORD processId);

        // DLL verification
        winrt::Windows::Foundation::IAsyncOperation<DllVerificationResult> VerifyDllAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction ValidateDllDependenciesAsync(const std::wstring& dllPath);

        // Injection methods
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> InjectViaLoadLibraryAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> InjectViaManualMapAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> InjectViaReflectiveAsync(DWORD processId, const std::wstring& dllPath);

        // High-level injection
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> InjectLibraryAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> InjectWithOptionsAsync(DWORD processId, const std::wstring& dllPath, const InjectionContext& context);

        // Ejection methods
        winrt::Windows::Foundation::IAsyncAction EjectLibraryAsync(DWORD processId, uint64_t moduleBase);
        winrt::Windows::Foundation::IAsyncAction EjectByPathAsync(DWORD processId, const std::wstring& dllPath);

        // Module enumeration
        winrt::Windows::Foundation::IAsyncOperation<std::vector<std::pair<uint64_t, std::wstring>>> EnumerateModulesAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncOperation<bool> IsModuleLoadedAsync(DWORD processId, const std::wstring& moduleName);

        // Configuration
        void SetPreferredMethod(InjectionMethod method) { m_preferredMethod = method; }
        void SetVerificationEnabled(bool enable) { m_verifyDllSignature = enable; }
        void SetTimeout(uint32_t timeoutMs) { m_injectionTimeoutMs = timeoutMs; }

        // Status
        bool IsInitialized() const noexcept { return m_isInitialized; }
        bool IsInjectionInProgress() const noexcept { return m_injectionInProgress; }
        const ArchitectureInfo& GetArchitectureInfo() const noexcept { return m_architectureInfo; }

    private:
        // Architecture detection and setup
        winrt::Windows::Foundation::IAsyncAction DetectArchitectureAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction SetupArchitectureInfoAsync();
        winrt::Windows::Foundation::IAsyncAction FindSystemAddressesAsync();

        // DLL analysis
        winrt::Windows::Foundation::IAsyncAction AnalyzeDllHeadersAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction ExtractExportTableAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction CalculateDllHashAsync(const std::wstring& dllPath);

        // Injection implementations
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> PerformLoadLibraryInjectionAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> PerformManualMapInjectionAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<InjectionResult> PerformReflectiveInjectionAsync(DWORD processId, const std::wstring& dllPath);

        // Shellcode generation
        winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateLoaderShellcodeAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateArm64ShellcodeAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateX64ShellcodeAsync(const std::wstring& dllPath);

        // Memory operations
        winrt::Windows::Foundation::IAsyncAction SetupInjectionMemoryAsync(DWORD processId, const std::vector<uint8_t>& shellcode);
        winrt::Windows::Foundation::IAsyncAction CleanupInjectionMemoryAsync(DWORD processId, uint64_t memoryAddress);
        winrt::Windows::Foundation::IAsyncAction RelocateShellcodeAsync(std::vector<uint8_t>& shellcode, uint64_t targetAddress);

        // Thread operations
        winrt::Windows::Foundation::IAsyncAction CreateInjectionThreadAsync(DWORD processId, uint64_t startAddress, uint64_t parameter);
        winrt::Windows::Foundation::IAsyncAction WaitForInjectionCompleteAsync(DWORD processId, uint64_t moduleBase);

        // Verification and validation
        winrt::Windows::Foundation::IAsyncAction VerifyInjectionSuccessAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction ValidateProcessStateAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction CheckInjectionCompatibilityAsync(DWORD processId, const std::wstring& dllPath);

        // Error handling and recovery
        winrt::Windows::Foundation::IAsyncAction HandleInjectionErrorAsync(DWORD processId, winrt::hresult error);
        winrt::Windows::Foundation::IAsyncAction AttemptInjectionRecoveryAsync(DWORD processId, const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction RollbackFailedInjectionAsync(DWORD processId);

        // Utility methods
        winrt::Windows::Foundation::IAsyncAction LogInjectionAttemptAsync(DWORD processId, const std::wstring& dllPath, InjectionMethod method);
        winrt::Windows::Foundation::IAsyncAction LogInjectionResultAsync(const InjectionResult& result);
        bool IsValidProcessId(DWORD processId);
        bool IsValidDllPath(const std::wstring& dllPath);
    };

    // Injection context builder
    class InjectionContextBuilder {
    private:
        InjectionContext m_context;

    public:
        InjectionContextBuilder& SetProcessId(DWORD processId);
        InjectionContextBuilder& SetDllPath(const std::wstring& dllPath);
        InjectionContextBuilder& SetMethod(InjectionMethod method);
        InjectionContextBuilder& SetTimeout(uint32_t timeoutMs);
        InjectionContextBuilder& SetParameters(const std::vector<std::string>& parameters);
        InjectionContextBuilder& EnableSignatureVerification(bool enable);
        InjectionContextBuilder& EnableAntiDebug(bool enable);

        InjectionContext Build();
        winrt::Windows::Foundation::IAsyncAction ValidateAsync();
    };

    // Injection statistics and monitoring
    class InjectionMonitor {
    private:
        std::vector<InjectionResult> m_injectionHistory;
        winrt::Windows::Foundation::Collections::IVector<winrt::hstring> m_activeInjections;
        std::mutex m_historyMutex;

    public:
        void RecordInjection(const InjectionResult& result);
        winrt::Windows::Foundation::IAsyncAction GetInjectionStatisticsAsync();
        winrt::Windows::Foundation::IAsyncAction ClearHistoryAsync();
        winrt::Windows::Foundation::IAsyncAction ExportHistoryAsync(const std::wstring& filePath);

        // Real-time monitoring
        winrt::Windows::Foundation::IAsyncAction StartMonitoringAsync();
        winrt::Windows::Foundation::IAsyncAction StopMonitoringAsync();
        winrt::Windows::Foundation::IAsyncAction GetActiveInjectionsAsync();
    };

    // ARM64 specific injector
    class Arm64Injector : public ModernInjector {
    public:
        // ARM64 specific overrides
        winrt::Windows::Foundation::IAsyncAction InitializeForArm64Async();
        winrt::Windows::Foundation::IAsyncOperation<std::vector<uint8_t>> GenerateArm64LoaderShellcodeAsync(const std::wstring& dllPath);
        winrt::Windows::Foundation::IAsyncAction HandleArm64MemoryLayoutAsync();

    private:
        // ARM64 specific helper methods
        uint64_t TranslateArm64Address(uint64_t x64Address);
        winrt::Windows::Foundation::IAsyncAction SetupArm64ExecutionContextAsync();
        bool IsArm64CompatibleDll(const std::wstring& dllPath);
    };

} // namespace MacType::Agent
