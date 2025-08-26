/*
 * MacType Named Pipe Server
 *
 * Handles communication with MacType Manager
 * - Named Pipe server for IPC with MacType Manager
 * - Message parsing and response generation
 * - Integration with agent controller
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

// Windows API for Named Pipes
#include <windows.h>

// Forward declarations
namespace MacType::Agent {
    class AgentController;
}

namespace MacType::Agent {

    // Manager communication message types (matching MacTypeCommunication.h)
    enum class ManagerMessageType {
        GetStatus = 1,
        SetMode = 2,
        GetProcessList = 3,
        InjectProcess = 4,
        RemoveInjection = 5,
        GetStyles = 6,
        ApplyStyle = 7,
        HealthCheck = 8
    };

    // Manager communication message structure
    struct ManagerMessage {
        ManagerMessageType type;
        DWORD dataSize;
        // Followed by data[dataSize]
    };

    // Process information for manager communication
    struct ProcessInfo {
        DWORD processId;
        std::string processName;
        std::string executablePath;
        bool is64Bit;
        bool isInjected;
        double memoryUsage;
        double cpuUsage;
    };

    // Style information for manager communication
    struct StyleInfo {
        std::string name;
        std::string description;
        std::string category;
        bool isBuiltIn;
        std::string previewText;
    };

    // MacType execution modes
    enum class MacTypeMode {
        Service,
        Tray,
        Manual,
        Unknown
    };

    // Named Pipe Server class
    class NamedPipeServer {
    private:
        // Core components
        AgentController* m_agentController;

        // Server state
        HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
        std::atomic<bool> m_isRunning = false;
        std::thread m_serverThread;

        // Pipe configuration
        static const inline std::string PIPE_NAME = "\\\\.\\pipe\\MacTypeAgent";
        static const inline DWORD PIPE_BUFFER_SIZE = 4096;
        static const inline DWORD PIPE_TIMEOUT = 5000; // 5 seconds

        // Message processing
        std::mutex m_messageMutex;
        std::queue<std::pair<HANDLE, std::vector<char>>> m_messageQueue;

    public:
        NamedPipeServer(AgentController* controller);
        ~NamedPipeServer();

        // Prevent copying
        NamedPipeServer(const NamedPipeServer&) = delete;
        NamedPipeServer& operator=(const NamedPipeServer&) = delete;

        // Server lifecycle
        winrt::Windows::Foundation::IAsyncAction StartAsync();
        winrt::Windows::Foundation::IAsyncAction StopAsync();

        // Status
        bool IsRunning() const noexcept { return m_isRunning; }

    private:
        // Server implementation
        void ServerThreadProc();
        winrt::Windows::Foundation::IAsyncAction HandleClientConnectionAsync(HANDLE clientPipe);
        winrt::Windows::Foundation::IAsyncAction ProcessClientMessagesAsync(HANDLE clientPipe);

        // Message processing
        std::vector<char> ProcessMessage(const std::vector<char>& messageData);
        std::vector<char> HandleGetStatus();
        std::vector<char> HandleSetMode(const std::string& modeData);
        std::vector<char> HandleGetProcessList();
        std::vector<char> HandleInjectProcess(const std::string& processData);
        std::vector<char> HandleRemoveInjection(const std::string& processData);
        std::vector<char> HandleGetStyles();
        std::vector<char> HandleApplyStyle(const std::string& styleData);
        std::vector<char> HandleHealthCheck();

        // Helper methods
        MacTypeMode StringToMode(const std::string& modeStr);
        std::string ModeToString(MacTypeMode mode);
        std::vector<ProcessInfo> GetCurrentProcessList();
        std::vector<StyleInfo> GetAvailableStyles();
        bool InjectIntoProcess(DWORD processId);
        bool RemoveInjectionFromProcess(DWORD processId);
        bool ApplyStyle(const std::string& styleName);

        // Data serialization
        std::string SerializeProcessList(const std::vector<ProcessInfo>& processes);
        std::string SerializeStyleList(const std::vector<StyleInfo>& styles);
        std::vector<char> CreateResponse(const std::string& data);
        std::vector<char> CreateErrorResponse(const std::string& error);

        // Pipe utilities
        HANDLE CreateNamedPipeServer();
        bool ConnectToClient(HANDLE pipeHandle);
        bool ReadMessage(HANDLE pipeHandle, std::vector<char>& buffer);
        bool WriteMessage(HANDLE pipeHandle, const std::vector<char>& buffer);
        void ClosePipeHandle(HANDLE& pipeHandle);
    };

    // Global server instance
    extern std::unique_ptr<NamedPipeServer> g_namedPipeServer;

}
