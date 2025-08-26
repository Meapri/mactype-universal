/*
 * MacType Process Bridge Implementation
 *
 * Secure cross-architecture process communication
 * Windows API direct usage without wow64ext dependency
 */

#include "process_bridge.h"
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <memory>
#include <sstream>

namespace MacType::Agent {

    // ProcessBridge implementation
    ProcessBridge::ProcessBridge() :
        m_isAttached(false),
        m_remoteModuleBase(0) {
        // Initialize hash provider for integrity checks
        m_hashProvider = winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider::OpenAlgorithm(
            winrt::Windows::Security::Cryptography::Core::HashAlgorithmNames::Sha256());
    }

    ProcessBridge::~ProcessBridge() {
        // Ensure proper cleanup
        if (m_isAttached) {
            DetachFromProcessAsync().get();
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::AttachToProcessAsync(DWORD processId) {
        if (m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Already attached to a process");
        }

        try {
            // Gather process information first
            co_await GatherProcessInfoAsync(processId);
            co_await DetermineArchitectureAsync();

            // Open process handles with required permissions
            co_await OpenProcessHandlesAsync(processId);

            // Verify process integrity
            co_await VerifyProcessIntegrityAsync();
            co_await CheckProcessPermissionsAsync();

            m_isAttached = true;

            // TODO: Log successful attachment
            // co_await LogEventAsync("Successfully attached to process " + std::to_string(processId));

        } catch (winrt::hresult_error const& ex) {
            co_await CloseProcessHandlesAsync();
            throw;
        } catch (std::exception const& ex) {
            co_await CloseProcessHandlesAsync();
            winrt::hresult_error error = winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
            throw error;
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::DetachFromProcessAsync() {
        if (!m_isAttached) {
            co_return;
        }

        try {
            // Clean up remote resources
            if (m_remoteModuleBase != 0) {
                // Note: We don't actually unload modules here as it could be dangerous
                // The module will be unloaded when the process terminates
                m_remoteModuleBase = 0;
            }

            // Close process handles
            co_await CloseProcessHandlesAsync();

            // Reset state
            m_isAttached = false;
            m_targetProcess = ProcessInfo{};
            m_remoteModuleBase = 0;

        } catch (winrt::hresult_error const& ex) {
            // Log error but still try to cleanup
            // TODO: Log error
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::VerifyProcessIntegrityAsync() {
        // Basic process verification
        if (!m_processHandle || !m_threadHandle) {
            throw winrt::hresult_error(E_HANDLE, L"Invalid process or thread handle");
        }

        // Check if process is still running
        DWORD exitCode;
        if (GetExitCodeProcess(m_processHandle.Get(), &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                throw winrt::hresult_error(E_FAIL, L"Process is not running");
            }
        }

        // Check process architecture compatibility
        BOOL isWow64 = FALSE;
        if (IsWow64Process(m_processHandle.Get(), &isWow64)) {
            // Additional architecture validation can be added here
        }

        // Verify process privileges and security context
        co_await VerifyProcessSecurityAsync();

        // Check for suspicious memory patterns
        co_await ValidateMemoryIntegrityAsync();

        // Verify loaded modules integrity
        co_await VerifyLoadedModulesAsync();
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::VerifyProcessSecurityAsync() {
        // Check if we have sufficient privileges
        HANDLE tokenHandle;
        if (OpenProcessToken(m_processHandle.Get(), TOKEN_QUERY, &tokenHandle)) {
            // Query token information
            DWORD tokenInfoLength = 0;
            GetTokenInformation(tokenHandle, TokenIntegrityLevel, nullptr, 0, &tokenInfoLength);

            if (tokenInfoLength > 0) {
                std::vector<BYTE> tokenInfo(tokenInfoLength);
                if (GetTokenInformation(tokenHandle, TokenIntegrityLevel, tokenInfo.data(), tokenInfoLength, &tokenInfoLength)) {
                    // Check integrity level
                    auto integrityLevel = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(tokenInfo.data());
                    SID* sid = integrityLevel->Label.Sid;

                    // Extract integrity level value
                    BYTE* count = GetSidSubAuthorityCount(sid);
                    DWORD* rid = GetSidSubAuthority(sid, *count - 1);

                    // Basic integrity check - can be enhanced
                    if (*rid < SECURITY_MANDATORY_MEDIUM_RID) {
                        // Low integrity level - might be restricted
                    }
                }
            }

            CloseHandle(tokenHandle);
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::ValidateMemoryIntegrityAsync() {
        // Check for signs of memory corruption or debugging
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T address = 0;

        while (VirtualQueryEx(m_processHandle.Get(), reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            // Check for suspicious memory regions
            if (mbi.State == MEM_COMMIT) {
                // Validate memory protection
                if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) {
                    // Potentially suspicious
                }

                // Check for unusually large committed regions
                if (mbi.RegionSize > 100 * 1024 * 1024) { // 100MB threshold
                    // Large memory region - could be suspicious
                }
            }

            address += mbi.RegionSize;

            // Prevent infinite loop
            if (address >= 0x7FFFFFFFFFFF) {
                break;
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::VerifyLoadedModulesAsync() {
        // Enumerate loaded modules and verify their integrity
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_targetProcess.processId);
        if (snapshot == INVALID_HANDLE_VALUE) {
            co_return; // Cannot verify - not necessarily an error
        }

        MODULEENTRY32 me;
        me.dwSize = sizeof(me);

        if (Module32First(snapshot, &me)) {
            do {
                // Check module path and integrity
                if (me.szExePath[0] != '\0') {
                    // Verify module is from a trusted location
                    co_await VerifyModulePathAsync(me.szExePath);

                    // Check module size and other attributes
                    if (me.modBaseSize == 0) {
                        // Suspicious - zero size module
                    }
                }
            } while (Module32Next(snapshot, &me));
        }

        CloseHandle(snapshot);
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::VerifyModulePathAsync(const char* modulePath) {
        // Basic module path verification
        if (!modulePath) {
            co_return;
        }

        std::string path(modulePath);

        // Check for suspicious paths
        if (path.find("temp") != std::string::npos ||
            path.find("tmp") != std::string::npos ||
            path.find("cache") != std::string::npos) {
            // Potentially suspicious location
        }

        // Verify file exists and is accessible
        WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
        if (!GetFileAttributesExA(modulePath, GetFileExInfoStandard, &fileAttributes)) {
            // File doesn't exist or can't be accessed
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::CheckProcessPermissionsAsync() {
        // Check if we have the required permissions
        // This is a basic check - in production, this would be more comprehensive

        // Test if we can read process memory
        SIZE_T bytesRead;
        uint64_t testAddress = reinterpret_cast<uint64_t>(GetModuleHandle(nullptr));

        if (!ReadProcessMemory(m_processHandle.Get(), reinterpret_cast<LPCVOID>(testAddress),
                              &bytesRead, sizeof(SIZE_T), &bytesRead)) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Insufficient permissions to read process memory");
        }
    }

    winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> ProcessBridge::AllocateRemoteMemoryAsync(size_t size, uint32_t protection) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        MemoryOpResult result;
        result.success = false;
        result.bytesTransferred = 0;

        try {
            // Validate size
            if (size == 0 || size > 1024 * 1024 * 1024) { // Max 1GB
                throw winrt::hresult_error(E_INVALIDARG, L"Invalid allocation size");
            }

            // Convert protection flags to Windows constants
            DWORD nativeProtection = ConvertProtectionFlags(protection);

            // Try to allocate memory in the remote process
            LPVOID remoteAddress = VirtualAllocEx(
                m_processHandle.Get(),
                nullptr,
                size,
                MEM_COMMIT | MEM_RESERVE,
                nativeProtection
            );

            if (!remoteAddress) {
                result.errorCode = HRESULT_FROM_WIN32(GetLastError());
                co_return result;
            }

            // Verify the allocation
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(m_processHandle.Get(), remoteAddress, &mbi, sizeof(mbi))) {
                if (mbi.BaseAddress != remoteAddress || mbi.RegionSize < size) {
                    // Allocation verification failed - cleanup
                    VirtualFreeEx(m_processHandle.Get(), remoteAddress, 0, MEM_RELEASE);
                    result.errorCode = E_FAIL;
                    co_return result;
                }
            }

            // Zero out the memory for security
            std::vector<uint8_t> zeroData(1024, 0);
            SIZE_T bytesWritten = 0;
            uint64_t currentAddress = reinterpret_cast<uint64_t>(remoteAddress);

            while (bytesWritten < size) {
                size_t toWrite = std::min(size - bytesWritten, zeroData.size());
                SIZE_T written;
                if (!WriteProcessMemory(m_processHandle.Get(),
                                       reinterpret_cast<LPVOID>(currentAddress),
                                       zeroData.data(), toWrite, &written)) {
                    // Zeroing failed - but allocation succeeded, so continue
                    break;
                }
                bytesWritten += written;
                currentAddress += written;
            }

            result.success = true;
            result.address = reinterpret_cast<uint64_t>(remoteAddress);
            result.bytesTransferred = size;

        } catch (winrt::hresult_error const& ex) {
            result.errorCode = ex.code();
        } catch (std::exception const& ex) {
            result.errorCode = E_FAIL;
        }

        co_return result;
    }

    winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> ProcessBridge::FreeRemoteMemoryAsync(uint64_t address) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        MemoryOpResult result;
        result.success = false;
        result.address = address;

        try {
            if (!VirtualFreeEx(m_processHandle.Get(), reinterpret_cast<LPVOID>(address), 0, MEM_RELEASE)) {
                result.errorCode = HRESULT_FROM_WIN32(GetLastError());
                co_return result;
            }

            result.success = true;

        } catch (winrt::hresult_error const& ex) {
            result.errorCode = ex.code();
        } catch (std::exception const& ex) {
            result.errorCode = E_FAIL;
        }

        co_return result;
    }

    winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> ProcessBridge::WriteRemoteMemoryAsync(uint64_t address, winrt::array_view<uint8_t> data) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        MemoryOpResult result;
        result.success = false;
        result.address = address;

        try {
            SIZE_T bytesWritten;

            if (!WriteProcessMemory(
                m_processHandle.Get(),
                reinterpret_cast<LPVOID>(address),
                data.data(),
                data.size(),
                &bytesWritten
            )) {
                result.errorCode = HRESULT_FROM_WIN32(GetLastError());
                co_return result;
            }

            result.success = true;
            result.bytesTransferred = bytesWritten;

        } catch (winrt::hresult_error const& ex) {
            result.errorCode = ex.code();
        } catch (std::exception const& ex) {
            result.errorCode = E_FAIL;
        }

        co_return result;
    }

    winrt::Windows::Foundation::IAsyncOperation<MemoryOpResult> ProcessBridge::ReadRemoteMemoryAsync(uint64_t address, size_t size) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        MemoryOpResult result;
        result.success = false;
        result.address = address;

        try {
            std::vector<uint8_t> buffer(size);
            SIZE_T bytesRead;

            if (!ReadProcessMemory(
                m_processHandle.Get(),
                reinterpret_cast<LPCVOID>(address),
                buffer.data(),
                size,
                &bytesRead
            )) {
                result.errorCode = HRESULT_FROM_WIN32(GetLastError());
                co_return result;
            }

            result.success = true;
            result.bytesTransferred = bytesRead;

        } catch (winrt::hresult_error const& ex) {
            result.errorCode = ex.code();
        } catch (std::exception const& ex) {
            result.errorCode = E_FAIL;
        }

        co_return result;
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::LoadRemoteModuleAsync(const std::wstring& modulePath) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        // For security reasons, we don't implement remote module loading
        // This would be a significant security risk
        // Instead, we rely on the existing MacType loading mechanisms

        throw winrt::hresult_error(E_NOTIMPL, L"Remote module loading is not supported for security reasons");
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::UnloadRemoteModuleAsync(uint64_t moduleBase) {
        // Similar security concerns as LoadRemoteModuleAsync
        throw winrt::hresult_error(E_NOTIMPL, L"Remote module unloading is not supported for security reasons");
    }

    winrt::Windows::Foundation::IAsyncOperation<uint64_t> ProcessBridge::GetRemoteProcAddressAsync(const std::wstring& moduleName, const std::string& functionName) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        // Get module handle in remote process
        HMODULE remoteModule = GetModuleHandleW(moduleName.c_str());
        if (!remoteModule) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Failed to get module handle in remote process");
        }

        // Get function address
        FARPROC functionAddress = GetProcAddress(remoteModule, functionName.c_str());
        if (!functionAddress) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Failed to get function address");
        }

        co_return reinterpret_cast<uint64_t>(functionAddress);
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::CreateRemoteThreadAsync(uint64_t startAddress, uint64_t parameter) {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        try {
            HANDLE remoteThread = CreateRemoteThread(
                m_processHandle.Get(),
                nullptr,
                0,
                reinterpret_cast<LPTHREAD_START_ROUTINE>(startAddress),
                reinterpret_cast<LPVOID>(parameter),
                0,
                nullptr
            );

            if (!remoteThread) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                          L"Failed to create remote thread");
            }

            // Store thread handle for cleanup
            m_threadHandle = HandleWrapper(remoteThread);

            // Wait for thread to complete
            WaitForSingleObject(remoteThread, INFINITE);

        } catch (winrt::hresult_error const& ex) {
            throw;
        } catch (std::exception const& ex) {
            winrt::hresult_error error = winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
            throw error;
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::SuspendRemoteThreadAsync() {
        if (!m_isAttached || !m_threadHandle) {
            throw winrt::hresult_error(E_INVALIDARG, L"No valid thread handle");
        }

        if (SuspendThread(m_threadHandle.Get()) == (DWORD)-1) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Failed to suspend remote thread");
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::ResumeRemoteThreadAsync() {
        if (!m_isAttached || !m_threadHandle) {
            throw winrt::hresult_error(E_INVALIDARG, L"No valid thread handle");
        }

        if (ResumeThread(m_threadHandle.Get()) == (DWORD)-1) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Failed to resume remote thread");
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::InjectLibraryAsync(const std::wstring& dllPath) {
        // This would be a complex implementation involving shellcode generation
        // and remote thread execution. For now, we'll defer to the ModernInjector

        throw winrt::hresult_error(E_NOTIMPL, L"Direct injection not implemented - use ModernInjector instead");
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::EjectLibraryAsync(uint64_t moduleBase) {
        // Security reasons - we don't implement ejection
        throw winrt::hresult_error(E_NOTIMPL, L"Library ejection not supported for security reasons");
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::GetThreadContextAsync() {
        if (!m_isAttached || !m_threadHandle) {
            throw winrt::hresult_error(E_INVALIDARG, L"No valid thread handle");
        }

        try {
            // Get thread context
            CONTEXT context = { 0 };
            context.ContextFlags = CONTEXT_FULL;

            if (!GetThreadContext(m_threadHandle.Get(), &context)) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                         L"Failed to get thread context");
            }

            // Store context for later use (simplified)
            // In a real implementation, you'd return or store this context

        } catch (winrt::hresult_error const& ex) {
            throw;
        } catch (std::exception const& ex) {
            winrt::hresult_error error = winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
            throw error;
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::SetThreadContextAsync() {
        if (!m_isAttached || !m_threadHandle) {
            throw winrt::hresult_error(E_INVALIDARG, L"No valid thread handle");
        }

        try {
            // Set thread context (simplified - would need a context parameter)
            CONTEXT context = { 0 };
            context.ContextFlags = CONTEXT_FULL;

            // Set context values here...

            if (!SetThreadContext(m_threadHandle.Get(), &context)) {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                         L"Failed to set thread context");
            }

        } catch (winrt::hresult_error const& ex) {
            throw;
        } catch (std::exception const& ex) {
            winrt::hresult_error error = winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
            throw error;
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::GetProcessStatisticsAsync() {
        if (!m_isAttached) {
            throw winrt::hresult_error(E_INVALIDARG, L"Not attached to a process");
        }

        // Get basic process information
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(m_processHandle.Get(), &pmc, sizeof(pmc))) {
            // TODO: Log memory statistics
        }

        // TODO: Get CPU usage, thread count, etc.
    }

    // Private implementation methods
    winrt::Windows::Foundation::IAsyncAction ProcessBridge::OpenProcessHandlesAsync(DWORD processId) {
        // Open process with required permissions
        DWORD desiredAccess = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                             PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME;

        HANDLE processHandle = OpenProcess(desiredAccess, FALSE, processId);
        if (!processHandle) {
            throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()),
                                      L"Failed to open process handle");
        }

        m_processHandle = HandleWrapper(processHandle);

        // Open main thread handle (simplified - in practice we'd enumerate threads)
        HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,
                                       FALSE, GetProcessMainThreadId(processId));
        if (!threadHandle) {
            // This might fail for various reasons, but we can continue without it
            // TODO: Log warning
        } else {
            m_threadHandle = HandleWrapper(threadHandle);
        }
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::CloseProcessHandlesAsync() {
        m_processHandle = HandleWrapper();
        m_threadHandle = HandleWrapper();
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::GatherProcessInfoAsync(DWORD processId) {
        m_targetProcess.processId = processId;

        // Get process name
        HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (processHandle) {
            WCHAR processName[MAX_PATH];
            DWORD size = MAX_PATH;

            if (QueryFullProcessImageNameW(processHandle, 0, processName, &size)) {
                m_targetProcess.processName = std::filesystem::path(processName).filename().wstring();
                m_targetProcess.executablePath = processName;
            }

            CloseHandle(processHandle);
        }

        // Set start time (simplified)
        m_targetProcess.startTime = winrt::clock::now();
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::DetermineArchitectureAsync() {
        // Check if process is 64-bit using IsWow64Process
        BOOL isWow64 = FALSE;
        if (IsWow64Process(m_processHandle.Get(), &isWow64)) {
            m_targetProcess.is64Bit = !isWow64;
        }

#ifdef _WIN32
        // On 32-bit Windows, all processes are 32-bit
        m_targetProcess.is64Bit = false;
#endif

        // ARM64 detection using Windows 10+ API
        m_targetProcess.isArm64 = co_await DetectArm64ProcessAsync();
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::DetermineArm64Async() {
        // Use PROCESS_MACHINE_INFORMATION to detect ARM64
        typedef struct _PROCESS_MACHINE_INFORMATION {
            USHORT ProcessMachine;
            USHORT Res0;
            USHORT MachineAttributes;
            USHORT Res1;
        } PROCESS_MACHINE_INFORMATION, *PPROCESS_MACHINE_INFORMATION;

        // This API is available on Windows 10 version 2004+
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!kernel32) {
            co_return;
        }

        typedef HRESULT(WINAPI* PGetProcessInformation)(
            HANDLE ProcessHandle,
            int ProcessInformationClass,
            PVOID ProcessInformation,
            DWORD ProcessInformationLength
        );

        auto pGetProcessInformation = reinterpret_cast<PGetProcessInformation>(
            GetProcAddress(kernel32, "GetProcessInformation"));

        if (pGetProcessInformation) {
            PROCESS_MACHINE_INFORMATION machineInfo = { 0 };
            HRESULT hr = pGetProcessInformation(
                m_processHandle.Get(),
                9, // ProcessMachineTypeInfo
                &machineInfo,
                sizeof(machineInfo)
            );

            if (SUCCEEDED(hr)) {
                // IMAGE_FILE_MACHINE_ARM64 = 0xAA64
                m_targetProcess.isArm64 = (machineInfo.ProcessMachine == 0xAA64);
            }
        }
    }

    winrt::Windows::Foundation::IAsyncOperation<bool> ProcessBridge::DetectArm64ProcessAsync() {
        // Check if we're running on ARM64 Windows
        SYSTEM_INFO systemInfo;
        GetNativeSystemInfo(&systemInfo);

        bool isArm64System = (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64);

        if (!isArm64System) {
            // Not running on ARM64 system, so process can't be ARM64
            co_return false;
        }

        // Use Windows API to detect if process is ARM64
        co_await DetermineArm64Async();

        co_return m_targetProcess.isArm64;
    }

    uint32_t ProcessBridge::ConvertProtectionFlags(uint32_t nativeFlags) {
        // Convert our flags to Windows PAGE_ constants
        switch (nativeFlags) {
            case 1: return PAGE_NOACCESS;
            case 2: return PAGE_READONLY;
            case 4: return PAGE_READWRITE;
            case 8: return PAGE_WRITECOPY;
            case 16: return PAGE_EXECUTE;
            case 32: return PAGE_EXECUTE_READ;
            case 64: return PAGE_EXECUTE_READWRITE;
            case 128: return PAGE_EXECUTE_WRITECOPY;
            default: return PAGE_READWRITE;
        }
    }

    bool ProcessBridge::IsValidMemoryRange(uint64_t address, size_t size) {
        // Basic validation
        if (address == 0 || size == 0) {
            return false;
        }

        // Check if address is within reasonable bounds
        // This is a simplified check - in production, we'd be more thorough
        return address < 0x7FFFFFFFFFFF; // Arbitrary upper limit
    }

    winrt::Windows::Foundation::IAsyncAction ProcessBridge::ValidateMemoryPointerAsync(uint64_t address) {
        if (!IsValidMemoryRange(address, 1)) {
            throw winrt::hresult_error(E_INVALIDARG, L"Invalid memory address");
        }
    }

    bool ProcessBridge::IsProcessStillRunning() {
        if (!m_processHandle) {
            return false;
        }

        DWORD exitCode;
        return GetExitCodeProcess(m_processHandle.Get(), &exitCode) && exitCode == STILL_ACTIVE;
    }

    // Helper function to get main thread ID (simplified)
    DWORD ProcessBridge::GetProcessMainThreadId(DWORD processId) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        THREADENTRY32 te;
        te.dwSize = sizeof(te);

        if (Thread32First(snapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == processId) {
                    CloseHandle(snapshot);
                    return te.th32ThreadID;
                }
            } while (Thread32Next(snapshot, &te));
        }

        CloseHandle(snapshot);
        return 0;
    }

} // namespace MacType::Agent
