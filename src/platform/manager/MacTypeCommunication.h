// MacTypeCommunication.h - Communication interface with MacType
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace MacTypeManager
{
    // Process information structure
    struct ProcessInfo
    {
        DWORD processId;
        std::wstring processName;
        std::wstring executablePath;
        bool is64Bit;
        bool isInjected;
        double memoryUsage; // MB
        double cpuUsage;    // %
    };

    // MacType mode enumeration
    enum class MacTypeMode
    {
        Service,
        Tray,
        Manual,
        Safe
    };

    // Style information structure
    struct StyleInfo
    {
        std::string name;
        std::string description;
        std::string category; // Clean, CRT, LCD, Platform, etc.
        bool isBuiltIn;
        std::string previewText;
    };

    // Communication interface class
    class MacTypeCommunicator
    {
    public:
        MacTypeCommunicator();
        ~MacTypeCommunicator();

        // Connection management
        bool ConnectToMacType();
        void Disconnect();
        bool IsConnected() const;

        // Mode management
        bool SetMode(MacTypeMode mode);
        MacTypeMode GetCurrentMode();
        bool IsServiceRunning();

        // Process management
        std::vector<ProcessInfo> GetProcessList();
        bool InjectIntoProcess(DWORD processId);
        bool RemoveInjection(DWORD processId);
        bool BatchInject(const std::vector<DWORD>& processIds);

        // Style management
        std::vector<StyleInfo> GetAvailableStyles();
        bool ApplyStyle(const std::string& styleName);
        bool SaveProfile(const std::string& profileName);
        bool LoadProfile(const std::string& profileName);

        // Monitoring
        void SetProcessListCallback(std::function<void(const std::vector<ProcessInfo>&)> callback);
        void SetModeChangeCallback(std::function<void(MacTypeMode)> callback);

    private:
        // IPC communication with mt64agnt
        bool SendCommand(const std::string& command, const std::string& data = "");
        std::string ReceiveResponse();
        bool EstablishNamedPipeConnection();

        // Internal state
        bool m_isConnected = false;
        HANDLE m_pipeHandle = nullptr;
        MacTypeMode m_currentMode = MacTypeMode::Unknown;

        // Callbacks
        std::function<void(const std::vector<ProcessInfo>&)> m_processCallback;
        std::function<void(MacTypeMode)> m_modeCallback;

        // Helper methods
        ProcessInfo ParseProcessInfo(const std::string& data);
        StyleInfo ParseStyleInfo(const std::string& data);
        std::vector<std::string> SplitString(const std::string& str, char delimiter);
    };

    // Global communicator instance
    extern std::unique_ptr<MacTypeCommunicator> g_communicator;
}
