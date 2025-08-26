/*
 * MacType Modern Agent Main Entry Point
 *
 * Windows App SDK based implementation of mt64agnt
 * Provides secure cross-architecture DLL injection
 */

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Background.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.Storage.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <filesystem>

#include "core/agent_controller.h"
#include "ipc/secure_channel.h"
#include "utils/logger.h"
#include "utils/winrt_helper.h"

// Forward declarations
namespace MacType::Agent {
    class AgentApplication;
}

using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Foundation;
using namespace MacType::Agent;

// Global application instance
std::unique_ptr<AgentApplication> g_application;

// AgentApplication class
class AgentApplication {
private:
    std::unique_ptr<AgentController> m_controller;
    std::unique_ptr<AgentLogger> m_logger;
    BackgroundTaskDeferral m_deferral{ nullptr };
    bool m_isRunning = false;

public:
    AgentApplication() = default;
    ~AgentApplication() {
        if (m_isRunning) {
            ShutdownAsync().get();
        }
    }

    // Prevent copying
    AgentApplication(const AgentApplication&) = delete;
    AgentApplication& operator=(const AgentApplication&) = delete;

    IAsyncAction InitializeAsync() {
        try {
            // Initialize logger first
            m_logger = LoggerFactory::CreateLogger();
            co_await m_logger->InitializeAsync();

            co_await m_logger->InfoAsync("AgentApplication", "Initializing MacType Modern Agent");

            // Create and initialize controller
            m_controller = std::make_unique<AgentController>();
            co_await m_controller->InitializeAsync();

            co_await m_logger->InfoAsync("AgentApplication", "Agent application initialized successfully");

        } catch (winrt::hresult_error const& ex) {
            if (m_logger) {
                co_await m_logger->CriticalAsync("AgentApplication",
                    "Failed to initialize application: " + winrt::to_string(ex.message()));
            } else {
                std::wcerr << L"Critical error during initialization: " << ex.message().c_str() << std::endl;
            }
            throw;
        }
    }

    IAsyncAction StartAsync() {
        if (!m_controller) {
            throw winrt::hresult_error(E_INVALIDARG, L"Controller not initialized");
        }

        try {
            m_isRunning = true;
            co_await m_controller->StartAsync();

            co_await m_logger->InfoAsync("AgentApplication", "Agent application started successfully");

            // Keep the application running
            co_await RunMessageLoopAsync();

        } catch (winrt::hresult_error const& ex) {
            m_isRunning = false;
            if (m_logger) {
                co_await m_logger->CriticalAsync("AgentApplication",
                    "Failed to start application: " + winrt::to_string(ex.message()));
            }
            throw;
        }
    }

    IAsyncAction StopAsync() {
        if (!m_isRunning) {
            co_return;
        }

        try {
            m_isRunning = false;

            if (m_controller) {
                co_await m_controller->StopAsync();
            }

            co_await m_logger->InfoAsync("AgentApplication", "Agent application stopped");

        } catch (winrt::hresult_error const& ex) {
            if (m_logger) {
                co_await m_logger->WarningAsync("AgentApplication",
                    "Error during application stop: " + winrt::to_string(ex.message()));
            }
        }
    }

    IAsyncAction ShutdownAsync() {
        if (!m_isRunning) {
            co_return;
        }

        try {
            co_await StopAsync();

            if (m_controller) {
                co_await m_controller->ShutdownAsync();
                m_controller.reset();
            }

            if (m_logger) {
                co_await m_logger->ShutdownAsync();
                m_logger.reset();
            }

            // Complete deferral if we have one
            if (m_deferral) {
                m_deferral.Complete();
                m_deferral = nullptr;
            }

        } catch (winrt::hresult_error const& ex) {
            // Log error if logger is still available
            if (m_logger) {
                try {
                    co_await m_logger->CriticalAsync("AgentApplication",
                        "Error during shutdown: " + winrt::to_string(ex.message()));
                } catch (...) {
                    // Ignore logging errors during shutdown
                }
            }
        }
    }

    IAsyncAction RunMessageLoopAsync() {
        // Run the main message loop
        while (m_isRunning) {
            try {
                // Check controller health periodically
                if (m_controller) {
                    co_await m_controller->MonitorSystemHealthAsync();
                }

                // Process Windows messages (simplified)
                MSG msg;
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    if (msg.message == WM_QUIT) {
                        co_await ShutdownAsync();
                        break;
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                // Small delay to prevent busy waiting
                co_await winrt::resume_after(winrt::Windows::Foundation::TimeSpan{ 100'000 }); // 10ms

            } catch (winrt::hresult_error const& ex) {
                if (m_logger) {
                    co_await m_logger->ErrorAsync("AgentApplication",
                        "Error in message loop: " + winrt::to_string(ex.message()));
                }

                // Continue running despite errors
                co_await winrt::resume_after(winrt::Windows::Foundation::TimeSpan{ 1'000'000 }); // 100ms
            }
        }
    }

    void SetBackgroundTaskDeferral(BackgroundTaskDeferral deferral) {
        m_deferral = deferral;
    }

    bool IsRunning() const noexcept {
        return m_isRunning;
    }

    const AgentController* GetController() const noexcept {
        return m_controller.get();
    }
};

// Global functions for Windows integration
IAsyncAction RunAsBackgroundTaskAsync(BackgroundTaskInstance taskInstance) {
    try {
        // Get deferral to keep task alive
        auto deferral = taskInstance.GetDeferral();

        // Create and initialize application
        g_application = std::make_unique<AgentApplication>();
        g_application->SetBackgroundTaskDeferral(deferral);

        co_await g_application->InitializeAsync();
        co_await g_application->StartAsync();

    } catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Failed to run as background task: " << ex.message().c_str() << std::endl;

        // Clean up on failure
        if (g_application) {
            co_await g_application->ShutdownAsync();
            g_application.reset();
        }

        throw;
    }
}

IAsyncAction RunAsConsoleAppAsync() {
    try {
        // Create and initialize application
        g_application = std::make_unique<AgentApplication>();

        co_await g_application->InitializeAsync();
        co_await g_application->StartAsync();

    } catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Failed to run as console app: " << ex.message().c_str() << std::endl;

        // Clean up on failure
        if (g_application) {
            co_await g_application->ShutdownAsync();
            g_application.reset();
        }

        throw;
    }
}

void SignalHandler(int signal) {
    if (g_application && g_application->IsRunning()) {
        // Shutdown gracefully on signal
        g_application->ShutdownAsync().get();
    }
    exit(signal);
}

// Main entry point for Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize Windows Runtime
    winrt::init_apartment();

    try {
        // Parse command line arguments
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        bool runAsService = false;
        bool runAsConsole = true; // Default to console mode

        // Simple argument parsing
        for (int i = 1; i < argc; ++i) {
            std::wstring arg = argv[i];
            if (arg == L"--service" || arg == L"/service") {
                runAsService = true;
                runAsConsole = false;
            } else if (arg == L"--console" || arg == L"/console") {
                runAsConsole = true;
                runAsService = false;
            } else if (arg == L"--help" || arg == L"/help" || arg == L"-h") {
                std::wcout << L"MacType Modern Agent" << std::endl;
                std::wcout << L"Usage: mt64agnt.exe [options]" << std::endl;
                std::wcout << L"Options:" << std::endl;
                std::wcout << L"  --service    Run as Windows service" << std::endl;
                std::wcout << L"  --console    Run as console application" << std::endl;
                std::wcout << L"  --help       Show this help message" << std::endl;
                return 0;
            }
        }

        LocalFree(argv);

        // Setup signal handlers for graceful shutdown
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);

        if (runAsService) {
            // Run as background task
            std::wcout << L"Starting MacType Agent as background task..." << std::endl;
            RunAsBackgroundTaskAsync(nullptr).get();

        } else {
            // Run as console application
            std::wcout << L"Starting MacType Agent as console application..." << std::endl;
            std::wcout << L"Press Ctrl+C to stop." << std::endl;
            RunAsConsoleAppAsync().get();
        }

        // Clean up
        if (g_application) {
            g_application->ShutdownAsync().get();
            g_application.reset();
        }

        std::wcout << L"MacType Agent stopped." << std::endl;
        return 0;

    } catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Critical error: " << ex.message().c_str() << std::endl;
        return 1;

    } catch (std::exception const& ex) {
        std::wcerr << L"Critical error: " << winrt::to_hstring(ex.what()).c_str() << std::endl;
        return 1;
    }
}

// Alternative main for console applications
int main(int argc, char* argv[]) {
    // This is a fallback for non-Windows platforms or when WinMain is not available
    return WinMain(nullptr, nullptr, nullptr, SW_SHOWNORMAL);
}
