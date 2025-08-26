/*
 * MacType Modern Agent Controller
 *
 * Modernized 64-bit agent controller using Windows App SDK
 * - Secure cross-architecture communication
 * - Process integrity verification
 * - Modern Windows Runtime integration
 */

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Background.h>
#include <winrt/Windows.System.Threading.h>
#include <memory>
#include <string>

namespace MacType::Agent {

    // Forward declarations
    class ProcessBridge;
    class SecureChannel;
    class ModernInjector;
    class DiagnosticsLogger;

    // Agent configuration
    struct AgentConfig {
        std::wstring serviceName = L"MacType.Agent";
        std::wstring displayName = L"MacType Modern Agent";
        std::wstring description = L"Secure cross-architecture agent for MacType";
        bool enableEncryption = true;
        bool enableDiagnostics = true;
        uint32_t maxConcurrentConnections = 10;
        uint32_t connectionTimeoutMs = 5000;
    };

    // Agent status enumeration
    enum class AgentStatus {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error
    };

    // Agent controller main class
    class AgentController {
    private:
        // Core components
        std::unique_ptr<ProcessBridge> m_processBridge;
        std::unique_ptr<SecureChannel> m_secureChannel;
        std::unique_ptr<ModernInjector> m_injector;
        std::unique_ptr<DiagnosticsLogger> m_logger;

        // Windows Runtime components
        winrt::Windows::System::Threading::ThreadPoolTimer m_healthTimer{ nullptr };
        winrt::Windows::ApplicationModel::Background::BackgroundTaskDeferral m_deferral{ nullptr };

        // State management
        AgentStatus m_status = AgentStatus::Stopped;
        AgentConfig m_config;

        // Event handlers
        winrt::event_token m_shutdownToken;

    public:
        AgentController();
        ~AgentController();

        // Prevent copying
        AgentController(const AgentController&) = delete;
        AgentController& operator=(const AgentController&) = delete;

        // Main lifecycle methods
        winrt::Windows::Foundation::IAsyncAction InitializeAsync();
        winrt::Windows::Foundation::IAsyncAction StartAsync();
        winrt::Windows::Foundation::IAsyncAction StopAsync();
        winrt::Windows::Foundation::IAsyncAction ShutdownAsync();

        // Core functionality
        winrt::Windows::Foundation::IAsyncAction ProcessInjectionRequestAsync(DWORD processId);
        winrt::Windows::Foundation::IAsyncAction HandleConfigurationUpdateAsync();
        winrt::Windows::Foundation::IAsyncAction MonitorSystemHealthAsync();

        // Status and diagnostics
        AgentStatus GetStatus() const noexcept { return m_status; }
        winrt::Windows::Foundation::IAsyncAction GetDiagnosticsReportAsync();

        // Configuration
        void SetConfig(const AgentConfig& config) { m_config = config; }
        const AgentConfig& GetConfig() const noexcept { return m_config; }

    private:
        // Initialization helpers
        winrt::Windows::Foundation::IAsyncAction InitializeComponentsAsync();
        winrt::Windows::Foundation::IAsyncAction SetupBackgroundTaskAsync();
        winrt::Windows::Foundation::IAsyncAction RegisterEventHandlersAsync();

        // Health monitoring
        winrt::Windows::Foundation::IAsyncAction HealthCheckTimerAsync();
        winrt::Windows::Foundation::IAsyncAction ReportHealthStatusAsync();

        // Error handling
        winrt::Windows::Foundation::IAsyncAction HandleCriticalErrorAsync(winrt::hresult_error error);
        winrt::Windows::Foundation::IAsyncAction AttemptRecoveryAsync();

        // Utility methods
        winrt::Windows::Foundation::IAsyncAction LogEventAsync(std::string_view message, uint32_t level = 1);
        bool IsProcessTrusted(DWORD processId);
    };

    // Service entry point for Windows
    class AgentService {
    public:
        static winrt::Windows::Foundation::IAsyncAction RunServiceAsync();
        static winrt::Windows::Foundation::IAsyncAction StopServiceAsync();

    private:
        static std::unique_ptr<AgentController> s_controller;
        static winrt::Windows::ApplicationModel::Background::BackgroundTaskDeferral s_serviceDeferral;
    };

} // namespace MacType::Agent
