/*
 * MacType Named Pipe Server Implementation
 *
 * Handles communication with MacType Manager
 * Named Pipe server with message processing
 */

#include "named_pipe_server.h"
#include "secure_channel.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <psapi.h>
#include <tlhelp32.h>

namespace MacType::Agent {

    // Global server instance
    std::unique_ptr<NamedPipeServer> g_namedPipeServer;

    NamedPipeServer::NamedPipeServer(AgentController* controller)
        : m_agentController(controller)
        , m_isRunning(false)
        , m_pipeHandle(INVALID_HANDLE_VALUE)
    {
    }

    NamedPipeServer::~NamedPipeServer()
    {
        if (m_isRunning) {
            StopAsync().get();
        }

        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }

        ClosePipeHandle(m_pipeHandle);
    }

    winrt::Windows::Foundation::IAsyncAction NamedPipeServer::StartAsync()
    {
        if (m_isRunning) {
            co_return;
        }

        try {
            m_isRunning = true;

            // Start server thread
            m_serverThread = std::thread([this]() {
                ServerThreadProc();
            });

            co_return;
        }
        catch (const std::exception& ex) {
            m_isRunning = false;
            throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to start named pipe server: ") +
                                      winrt::hstring(std::string(ex.what())));
        }
    }

    winrt::Windows::Foundation::IAsyncAction NamedPipeServer::StopAsync()
    {
        if (!m_isRunning) {
            co_return;
        }

        m_isRunning = false;

        // Wait for server thread to finish
        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }

        ClosePipeHandle(m_pipeHandle);

        co_return;
    }

    void NamedPipeServer::ServerThreadProc()
    {
        while (m_isRunning) {
            try {
                // Create named pipe server instance
                m_pipeHandle = CreateNamedPipeServer();
                if (m_pipeHandle == INVALID_HANDLE_VALUE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    continue;
                }

                // Wait for client connection
                if (ConnectToClient(m_pipeHandle)) {
                    // Handle client connection asynchronously
                    HandleClientConnectionAsync(m_pipeHandle).get();
                }

                // Clean up pipe handle
                ClosePipeHandle(m_pipeHandle);
            }
            catch (const std::exception& ex) {
                // Log error and continue
                OutputDebugStringA(("Named pipe server error: " + std::string(ex.what()) + "\n").c_str());
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction NamedPipeServer::HandleClientConnectionAsync(HANDLE clientPipe)
    {
        try {
            co_await ProcessClientMessagesAsync(clientPipe);
        }
        catch (const std::exception& ex) {
            OutputDebugStringA(("Client connection error: " + std::string(ex.what()) + "\n").c_str());
        }

        // Disconnect client
        if (clientPipe != INVALID_HANDLE_VALUE) {
            DisconnectNamedPipe(clientPipe);
        }

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction NamedPipeServer::ProcessClientMessagesAsync(HANDLE clientPipe)
    {
        std::vector<char> messageBuffer;

        while (m_isRunning) {
            // Read message from client
            if (!ReadMessage(clientPipe, messageBuffer)) {
                break; // Client disconnected or error
            }

            if (messageBuffer.empty()) {
                continue;
            }

            // Process message and generate response
            auto response = ProcessMessage(messageBuffer);

            // Send response back to client
            if (!WriteMessage(clientPipe, response)) {
                break; // Failed to send response
            }
        }

        co_return;
    }

    std::vector<char> NamedPipeServer::ProcessMessage(const std::vector<char>& messageData)
    {
        try {
            if (messageData.size() < sizeof(ManagerMessage)) {
                return CreateErrorResponse("Invalid message format");
            }

            // Parse message header
            ManagerMessage message;
            memcpy(&message, messageData.data(), sizeof(ManagerMessage));

            // Extract message data
            std::string data;
            if (message.dataSize > 0) {
                size_t dataOffset = sizeof(ManagerMessage);
                if (dataOffset + message.dataSize <= messageData.size()) {
                    data.assign(messageData.data() + dataOffset, message.dataSize);
                }
            }

            // Process message based on type
            switch (message.type) {
            case ManagerMessageType::GetStatus:
                return HandleGetStatus();
            case ManagerMessageType::SetMode:
                return HandleSetMode(data);
            case ManagerMessageType::GetProcessList:
                return HandleGetProcessList();
            case ManagerMessageType::InjectProcess:
                return HandleInjectProcess(data);
            case ManagerMessageType::RemoveInjection:
                return HandleRemoveInjection(data);
            case ManagerMessageType::GetStyles:
                return HandleGetStyles();
            case ManagerMessageType::ApplyStyle:
                return HandleApplyStyle(data);
            case ManagerMessageType::HealthCheck:
                return HandleHealthCheck();
            default:
                return CreateErrorResponse("Unknown message type");
            }
        }
        catch (const std::exception& ex) {
            return CreateErrorResponse(std::string("Message processing error: ") + ex.what());
        }
    }

    std::vector<char> NamedPipeServer::HandleGetStatus()
    {
        try {
            // Get current agent status
            auto status = m_agentController ? m_agentController->GetStatus() : AgentStatus::Stopped;

            // Convert status to string (for simplicity, return mode as string)
            std::string statusStr = "Service"; // Default to service mode

            return CreateResponse(statusStr);
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Failed to get status");
        }
    }

    std::vector<char> NamedPipeServer::HandleSetMode(const std::string& modeData)
    {
        try {
            // Parse mode from string
            auto mode = StringToMode(modeData);
            if (mode == MacTypeMode::Unknown) {
                return CreateErrorResponse("Invalid mode");
            }

            // TODO: Apply mode change through agent controller
            // For now, just acknowledge
            return CreateResponse("SUCCESS");
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Failed to set mode");
        }
    }

    std::vector<char> NamedPipeServer::HandleGetProcessList()
    {
        try {
            auto processes = GetCurrentProcessList();
            auto processData = SerializeProcessList(processes);
            return CreateResponse(processData);
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Failed to get process list");
        }
    }

    std::vector<char> NamedPipeServer::HandleInjectProcess(const std::string& processData)
    {
        try {
            DWORD processId = std::stoul(processData);
            if (InjectIntoProcess(processId)) {
                return CreateResponse("SUCCESS");
            } else {
                return CreateErrorResponse("Injection failed");
            }
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Invalid process ID");
        }
    }

    std::vector<char> NamedPipeServer::HandleRemoveInjection(const std::string& processData)
    {
        try {
            DWORD processId = std::stoul(processData);
            if (RemoveInjectionFromProcess(processId)) {
                return CreateResponse("SUCCESS");
            } else {
                return CreateErrorResponse("Remove injection failed");
            }
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Invalid process ID");
        }
    }

    std::vector<char> NamedPipeServer::HandleGetStyles()
    {
        try {
            auto styles = GetAvailableStyles();
            auto styleData = SerializeStyleList(styles);
            return CreateResponse(styleData);
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Failed to get styles");
        }
    }

    std::vector<char> NamedPipeServer::HandleApplyStyle(const std::string& styleData)
    {
        try {
            if (ApplyStyle(styleData)) {
                return CreateResponse("SUCCESS");
            } else {
                return CreateErrorResponse("Style application failed");
            }
        }
        catch (const std::exception&) {
            return CreateErrorResponse("Invalid style name");
        }
    }

    std::vector<char> NamedPipeServer::HandleHealthCheck()
    {
        try {
            // Simple health check
            bool isHealthy = m_agentController && m_agentController->GetStatus() == AgentStatus::Running;
            return CreateResponse(isHealthy ? "OK" : "UNHEALTHY");
        }
        catch (const std::exception&) {
            return CreateResponse("ERROR");
        }
    }

    // Helper method implementations
    MacTypeMode NamedPipeServer::StringToMode(const std::string& modeStr)
    {
        if (modeStr == "Service") return MacTypeMode::Service;
        if (modeStr == "Tray") return MacTypeMode::Tray;
        if (modeStr == "Manual") return MacTypeMode::Manual;
        return MacTypeMode::Unknown;
    }

    std::string NamedPipeServer::ModeToString(MacTypeMode mode)
    {
        switch (mode) {
        case MacTypeMode::Service: return "Service";
        case MacTypeMode::Tray: return "Tray";
        case MacTypeMode::Manual: return "Manual";
        default: return "Unknown";
        }
    }

    std::vector<ProcessInfo> NamedPipeServer::GetCurrentProcessList()
    {
        std::vector<ProcessInfo> processes;

        // For testing on non-Windows platforms, return mock data
#ifndef _WIN32
        // Mock process data for testing
        ProcessInfo mock1;
        mock1.processId = 1234;
        mock1.processName = "notepad.exe";
        mock1.executablePath = "C:\\Windows\\System32\\notepad.exe";
        mock1.is64Bit = true;
        mock1.isInjected = false;
        mock1.memoryUsage = 15.2;
        mock1.cpuUsage = 0.1;
        processes.push_back(mock1);

        ProcessInfo mock2;
        mock2.processId = 5678;
        mock2.processName = "chrome.exe";
        mock2.executablePath = "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe";
        mock2.is64Bit = true;
        mock2.isInjected = true;
        mock2.memoryUsage = 120.5;
        mock2.cpuUsage = 2.3;
        processes.push_back(mock2);

        ProcessInfo mock3;
        mock3.processId = 4321;
        mock3.processName = "mspaint.exe";
        mock3.executablePath = "C:\\Windows\\System32\\mspaint.exe";
        mock3.is64Bit = false; // 32-bit process
        mock3.isInjected = false;
        mock3.memoryUsage = 8.7;
        mock3.cpuUsage = 0.5;
        processes.push_back(mock3);
#else
        try {
            // Get snapshot of all processes
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE) {
                return processes;
            }

            PROCESSENTRY32W processEntry = { sizeof(PROCESSENTRY32W) };
            if (Process32FirstW(snapshot, &processEntry)) {
                do {
                    ProcessInfo info;
                    info.processId = processEntry.th32ProcessID;

                    // Convert process name from wide string
                    std::wstring processNameW(processEntry.szExeFile);
                    info.processName = std::string(processNameW.begin(), processNameW.end());

                    // Get executable path
                    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.processId);
                    if (processHandle) {
                        WCHAR pathBuffer[MAX_PATH];
                        DWORD pathSize = MAX_PATH;
                        if (QueryFullProcessImageNameW(processHandle, 0, pathBuffer, &pathSize)) {
                            std::wstring pathW(pathBuffer);
                            info.executablePath = std::string(pathW.begin(), pathW.end());
                        }
                        CloseHandle(processHandle);
                    }

                    // Determine if 64-bit process (simplified check)
                    info.is64Bit = true; // Assume 64-bit on 64-bit Windows

                    // TODO: Check if process is injected
                    info.isInjected = false; // Placeholder

                    // TODO: Get memory and CPU usage
                    info.memoryUsage = 0.0; // Placeholder
                    info.cpuUsage = 0.0;    // Placeholder

                    // Filter out system processes
                    if (ShouldIncludeProcess(info)) {
                        processes.push_back(info);
                    }

                } while (Process32NextW(snapshot, &processEntry));
            }

            CloseHandle(snapshot);
        }
        catch (const std::exception& ex) {
            OutputDebugStringA(("Error getting process list: " + std::string(ex.what()) + "\n").c_str());
        }
#endif

        return processes;
    }

    std::vector<StyleInfo> NamedPipeServer::GetAvailableStyles()
    {
        std::vector<StyleInfo> styles;

        // Return default styles for now
        StyleInfo cleanStyle;
        cleanStyle.name = "Clean";
        cleanStyle.description = "Clean and sharp font rendering";
        cleanStyle.category = "Built-in";
        cleanStyle.isBuiltIn = true;
        cleanStyle.previewText = "The quick brown fox jumps over the lazy dog";
        styles.push_back(cleanStyle);

        StyleInfo crtStyle;
        crtStyle.name = "CRT";
        crtStyle.description = "Cathode Ray Tube style with scanlines";
        crtStyle.category = "Built-in";
        crtStyle.isBuiltIn = true;
        crtStyle.previewText = "Retro CRT display simulation";
        styles.push_back(crtStyle);

        StyleInfo lcdStyle;
        lcdStyle.name = "LCD";
        lcdStyle.description = "Subpixel rendering for LCD displays";
        lcdStyle.category = "Built-in";
        lcdStyle.isBuiltIn = true;
        lcdStyle.previewText = "Enhanced LCD display optimization";
        styles.push_back(lcdStyle);

        return styles;
    }

    bool NamedPipeServer::InjectIntoProcess(DWORD processId)
    {
        try {
            // TODO: Implement actual injection using ModernInjector
            // For now, just return success
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    bool NamedPipeServer::RemoveInjectionFromProcess(DWORD processId)
    {
        try {
            // TODO: Implement actual injection removal
            // For now, just return success
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    bool NamedPipeServer::ApplyStyle(const std::string& styleName)
    {
        try {
            // TODO: Implement actual style application
            // For now, just return success
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    std::string NamedPipeServer::SerializeProcessList(const std::vector<ProcessInfo>& processes)
    {
        std::stringstream ss;
        for (size_t i = 0; i < processes.size(); ++i) {
            if (i > 0) ss << "\n";
            const auto& proc = processes[i];
            ss << proc.processId << "|"
               << proc.processName << "|"
               << proc.executablePath << "|"
               << (proc.is64Bit ? "1" : "0") << "|"
               << (proc.isInjected ? "1" : "0") << "|"
               << proc.memoryUsage << "|"
               << proc.cpuUsage;
        }
        return ss.str();
    }

    std::string NamedPipeServer::SerializeStyleList(const std::vector<StyleInfo>& styles)
    {
        std::stringstream ss;
        for (size_t i = 0; i < styles.size(); ++i) {
            if (i > 0) ss << "\n";
            const auto& style = styles[i];
            ss << style.name << "|"
               << style.description << "|"
               << style.category << "|"
               << (style.isBuiltIn ? "1" : "0") << "|"
               << style.previewText;
        }
        return ss.str();
    }

    std::vector<char> NamedPipeServer::CreateResponse(const std::string& data)
    {
        std::vector<char> response;
        response.reserve(sizeof(ManagerMessage) + data.size());

        ManagerMessage header;
        header.type = ManagerMessageType::GetStatus; // Response type (can be any for now)
        header.dataSize = static_cast<DWORD>(data.size());

        // Add header
        response.insert(response.end(), reinterpret_cast<char*>(&header), reinterpret_cast<char*>(&header) + sizeof(header));

        // Add data
        if (!data.empty()) {
            response.insert(response.end(), data.begin(), data.end());
        }

        return response;
    }

    std::vector<char> NamedPipeServer::CreateErrorResponse(const std::string& error)
    {
        return CreateResponse("ERROR: " + error);
    }

    bool NamedPipeServer::ShouldIncludeProcess(const ProcessInfo& process)
    {
        // Filter out system processes
        static const std::vector<std::string> filteredProcesses = {
            "System", "Registry", "smss.exe", "csrss.exe", "wininit.exe",
            "services.exe", "lsass.exe", "svchost.exe", "explorer.exe"
        };

        for (const auto& filtered : filteredProcesses) {
            if (process.processName == filtered) {
                return false;
            }
        }

        return true;
    }

    HANDLE NamedPipeServer::CreateNamedPipeServer()
    {
        return CreateNamedPipeA(
            PIPE_NAME.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            PIPE_BUFFER_SIZE,
            PIPE_BUFFER_SIZE,
            PIPE_TIMEOUT,
            nullptr
        );
    }

    bool NamedPipeServer::ConnectToClient(HANDLE pipeHandle)
    {
        return ConnectNamedPipe(pipeHandle, nullptr) ||
               GetLastError() == ERROR_PIPE_CONNECTED;
    }

    bool NamedPipeServer::ReadMessage(HANDLE pipeHandle, std::vector<char>& buffer)
    {
        buffer.clear();
        char tempBuffer[PIPE_BUFFER_SIZE];
        DWORD bytesRead;

        if (!ReadFile(pipeHandle, tempBuffer, sizeof(tempBuffer), &bytesRead, nullptr)) {
            return false;
        }

        buffer.assign(tempBuffer, tempBuffer + bytesRead);
        return true;
    }

    bool NamedPipeServer::WriteMessage(HANDLE pipeHandle, const std::vector<char>& buffer)
    {
        DWORD bytesWritten;
        return WriteFile(pipeHandle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesWritten, nullptr);
    }

    void NamedPipeServer::ClosePipeHandle(HANDLE& pipeHandle)
    {
        if (pipeHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(pipeHandle);
            pipeHandle = INVALID_HANDLE_VALUE;
        }
    }

}
