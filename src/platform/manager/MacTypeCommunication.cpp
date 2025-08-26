// MacTypeCommunication.cpp - Communication implementation with MacType
#include "pch.h"
#include "MacTypeCommunication.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <codecvt>
#include <locale>

// Named Pipe constants
#define MT_AGENT_PIPE_NAME L"\\\\.\\pipe\\MacTypeAgent"
#define PIPE_TIMEOUT 5000  // 5 seconds

namespace MacTypeManager
{
    // Global communicator instance
    std::unique_ptr<MacTypeCommunicator> g_communicator;

    // Message types for communication with mt64agnt
    enum class AgentMessageType
    {
        GetStatus = 1,
        SetMode = 2,
        GetProcessList = 3,
        InjectProcess = 4,
        RemoveInjection = 5,
        GetStyles = 6,
        ApplyStyle = 7,
        HealthCheck = 8
    };

    // Message structure for named pipe communication
    struct PipeMessage
    {
        AgentMessageType type;
        DWORD dataSize;
        // Followed by data[dataSize]
    };

    MacTypeCommunicator::MacTypeCommunicator()
        : m_isConnected(false)
        , m_pipeHandle(nullptr)
        , m_currentMode(MacTypeMode::Unknown)
    {
    }

    MacTypeCommunicator::~MacTypeCommunicator()
    {
        Disconnect();
    }

    bool MacTypeCommunicator::ConnectToMacType()
    {
        if (m_isConnected)
        {
            return true;
        }

        return EstablishNamedPipeConnection();
    }

    void MacTypeCommunicator::Disconnect()
    {
        if (m_pipeHandle && m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_pipeHandle);
            m_pipeHandle = nullptr;
        }
        m_isConnected = false;
    }

    bool MacTypeCommunicator::IsConnected() const
    {
        return m_isConnected && m_pipeHandle != nullptr;
    }

    bool MacTypeCommunicator::SetMode(MacTypeMode mode)
    {
        if (!m_isConnected)
        {
            return false;
        }

        // Send mode change command
        std::string data = std::to_string(static_cast<int>(mode));
        auto response = SendCommand(AgentMessageType::SetMode, data);

        if (!response.empty())
        {
            // Update local state on success
            m_currentMode = mode;
            return true;
        }

        return false;
    }

    MacTypeMode MacTypeCommunicator::GetCurrentMode()
    {
        if (!m_isConnected)
        {
            return MacTypeMode::Unknown;
        }

        // Query current mode from mt64agnt
        auto response = SendCommand(AgentMessageType::GetStatus, "");

        if (!response.empty())
        {
            try
            {
                int modeInt = std::stoi(response);
                return static_cast<MacTypeMode>(modeInt);
            }
            catch (const std::exception&)
            {
                return MacTypeMode::Unknown;
            }
        }

        return MacTypeMode::Unknown;
    }

    bool MacTypeCommunicator::IsServiceRunning()
    {
        if (!m_isConnected)
        {
            return false;
        }

        // Send health check
        auto response = SendCommand(AgentMessageType::HealthCheck, "");

        return !response.empty() && response == "OK";
    }

    std::vector<ProcessInfo> MacTypeCommunicator::GetProcessList()
    {
        std::vector<ProcessInfo> processes;

        if (!m_isConnected)
        {
            return processes;
        }

        // Get process list from mt64agnt
        auto response = SendCommand(AgentMessageType::GetProcessList, "");

        if (!response.empty())
        {
            processes = ParseProcessList(response);
        }

        return processes;
    }

    bool MacTypeCommunicator::InjectIntoProcess(DWORD processId)
    {
        if (!m_isConnected)
        {
            return false;
        }

        std::string data = std::to_string(processId);
        auto response = SendCommand(AgentMessageType::InjectProcess, data);

        return !response.empty() && response == "SUCCESS";
    }

    bool MacTypeCommunicator::RemoveInjection(DWORD processId)
    {
        if (!m_isConnected)
        {
            return false;
        }

        std::string data = std::to_string(processId);
        auto response = SendCommand(AgentMessageType::RemoveInjection, data);

        return !response.empty() && response == "SUCCESS";
    }

    bool MacTypeCommunicator::BatchInject(const std::vector<DWORD>& processIds)
    {
        if (!m_isConnected || processIds.empty())
        {
            return false;
        }

        // Convert process IDs to comma-separated string
        std::stringstream ss;
        for (size_t i = 0; i < processIds.size(); ++i)
        {
            if (i > 0) ss << ",";
            ss << processIds[i];
        }

        auto response = SendCommand(AgentMessageType::InjectProcess, ss.str());
        return !response.empty() && response == "SUCCESS";
    }

    std::vector<StyleInfo> MacTypeCommunicator::GetAvailableStyles()
    {
        std::vector<StyleInfo> styles;

        // Get style list from MacType
        auto response = SendCommand(AgentMessageType::GetStyles, "");

        if (!response.empty())
        {
            styles = ParseStyleList(response);
        }
        else
        {
            // Return default styles if communication fails
            styles = GetDefaultStyles();
        }

        return styles;
    }

    bool MacTypeCommunicator::ApplyStyle(const std::string& styleName)
    {
        if (!m_isConnected)
        {
            return false;
        }

        auto response = SendCommand(AgentMessageType::ApplyStyle, styleName);
        return !response.empty() && response == "SUCCESS";
    }

    bool MacTypeCommunicator::SaveProfile(const std::string& profileName)
    {
        // TODO: Implement profile saving
        return true;
    }

    bool MacTypeCommunicator::LoadProfile(const std::string& profileName)
    {
        // TODO: Implement profile loading
        return true;
    }

    void MacTypeCommunicator::SetProcessListCallback(std::function<void(const std::vector<ProcessInfo>&)> callback)
    {
        m_processCallback = callback;
    }

    void MacTypeCommunicator::SetModeChangeCallback(std::function<void(MacTypeMode)> callback)
    {
        m_modeCallback = callback;
    }

    std::string MacTypeCommunicator::SendCommand(AgentMessageType type, const std::string& data)
    {
        if (!m_isConnected || !m_pipeHandle)
        {
            return "";
        }

        try
        {
            // Prepare message
            PipeMessage message;
            message.type = type;
            message.dataSize = static_cast<DWORD>(data.size());

            // Send message header
            DWORD bytesWritten;
            if (!WriteFile(m_pipeHandle, &message, sizeof(message), &bytesWritten, nullptr))
            {
                return "";
            }

            // Send message data if any
            if (message.dataSize > 0)
            {
                if (!WriteFile(m_pipeHandle, data.c_str(), message.dataSize, &bytesWritten, nullptr))
                {
                    return "";
                }
            }

            // Read response
            return ReceiveResponse();
        }
        catch (const std::exception&)
        {
            return "";
        }
    }

    std::string MacTypeCommunicator::ReceiveResponse()
    {
        if (!m_pipeHandle)
        {
            return "";
        }

        try
        {
            // Read response header
            PipeMessage responseHeader;
            DWORD bytesRead;
            if (!ReadFile(m_pipeHandle, &responseHeader, sizeof(responseHeader), &bytesRead, nullptr))
            {
                return "";
            }

            // Read response data
            if (responseHeader.dataSize > 0)
            {
                std::vector<char> buffer(responseHeader.dataSize + 1, 0);
                if (!ReadFile(m_pipeHandle, buffer.data(), responseHeader.dataSize, &bytesRead, nullptr))
                {
                    return "";
                }
                return std::string(buffer.data());
            }

            return "";
        }
        catch (const std::exception&)
        {
            return "";
        }
    }

    bool MacTypeCommunicator::EstablishNamedPipeConnection()
    {
        try
        {
            // Try to connect to mt64agnt's named pipe
            m_pipeHandle = CreateFileW(
                MT_AGENT_PIPE_NAME,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (m_pipeHandle == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            // Set pipe mode to message mode
            DWORD mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
            if (!SetNamedPipeHandleState(m_pipeHandle, &mode, nullptr, nullptr))
            {
                CloseHandle(m_pipeHandle);
                m_pipeHandle = nullptr;
                return false;
            }

            // Set read timeout
            COMMTIMEOUTS timeouts;
            timeouts.ReadIntervalTimeout = PIPE_TIMEOUT;
            timeouts.ReadTotalTimeoutMultiplier = 0;
            timeouts.ReadTotalTimeoutConstant = PIPE_TIMEOUT;
            timeouts.WriteTotalTimeoutMultiplier = 0;
            timeouts.WriteTotalTimeoutConstant = PIPE_TIMEOUT;

            if (!SetCommTimeouts(m_pipeHandle, &timeouts))
            {
                CloseHandle(m_pipeHandle);
                m_pipeHandle = nullptr;
                return false;
            }

            m_isConnected = true;
            return true;
        }
        catch (const std::exception&)
        {
            if (m_pipeHandle)
            {
                CloseHandle(m_pipeHandle);
                m_pipeHandle = nullptr;
            }
            return false;
        }
    }

    std::vector<ProcessInfo> MacTypeCommunicator::ParseProcessList(const std::string& data)
    {
        std::vector<ProcessInfo> processes;
        std::vector<std::string> lines = SplitString(data, '\n');

        for (const auto& line : lines)
        {
            if (line.empty()) continue;

            std::vector<std::string> fields = SplitString(line, '|');
            if (fields.size() >= 6)
            {
                ProcessInfo info;
                try
                {
                    info.processId = std::stoul(fields[0]);
                    info.processName = StringToWString(fields[1]);
                    info.executablePath = StringToWString(fields[2]);
                    info.is64Bit = fields[3] == "1";
                    info.isInjected = fields[4] == "1";
                    info.memoryUsage = std::stod(fields[5]);
                    if (fields.size() >= 7)
                    {
                        info.cpuUsage = std::stod(fields[6]);
                    }
                    processes.push_back(info);
                }
                catch (const std::exception&)
                {
                    // Skip malformed entries
                    continue;
                }
            }
        }

        return processes;
    }

    std::vector<StyleInfo> MacTypeCommunicator::ParseStyleList(const std::string& data)
    {
        std::vector<StyleInfo> styles;
        std::vector<std::string> lines = SplitString(data, '\n');

        for (const auto& line : lines)
        {
            if (line.empty()) continue;

            std::vector<std::string> fields = SplitString(line, '|');
            if (fields.size() >= 5)
            {
                StyleInfo info;
                info.name = fields[0];
                info.description = fields[1];
                info.category = fields[2];
                info.isBuiltIn = fields[3] == "1";
                info.previewText = fields[4];
                styles.push_back(info);
            }
        }

        return styles;
    }

    std::vector<StyleInfo> MacTypeCommunicator::GetDefaultStyles()
    {
        std::vector<StyleInfo> styles;

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

    std::vector<std::string> MacTypeCommunicator::SplitString(const std::string& str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::wstring MacTypeCommunicator::StringToWString(const std::string& str)
    {
        try
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(str);
        }
        catch (const std::exception&)
        {
            // Fallback for non-UTF8 strings
            std::wstring result;
            result.reserve(str.size());
            for (char c : str)
            {
                result.push_back(static_cast<wchar_t>(c));
            }
            return result;
        }
    }
}
