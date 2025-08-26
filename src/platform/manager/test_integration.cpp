// test_integration.cpp - Integration test for MacType Manager communication
#include "MacTypeCommunication.h"
#include <iostream>
#include <thread>
#include <chrono>

void TestCommunication()
{
    std::cout << "=== MacType Manager Integration Test ===\n\n";

    // Create communicator
    auto communicator = std::make_shared<MacTypeManager::MacTypeCommunicator>();

    std::cout << "1. Connecting to MacType agent...\n";
    if (communicator->ConnectToMacType()) {
        std::cout << "   ✓ Successfully connected to MacType agent\n\n";

        // Test health check
        std::cout << "2. Testing health check...\n";
        if (communicator->IsServiceRunning()) {
            std::cout << "   ✓ Service is running\n\n";
        } else {
            std::cout << "   ✗ Service is not running (this is expected in test environment)\n\n";
        }

        // Test getting current mode
        std::cout << "3. Testing mode query...\n";
        auto currentMode = communicator->GetCurrentMode();
        std::cout << "   Current mode: " << static_cast<int>(currentMode) << "\n\n";

        // Test getting process list
        std::cout << "4. Testing process list retrieval...\n";
        auto processes = communicator->GetProcessList();
        std::cout << "   Found " << processes.size() << " processes:\n";
        for (const auto& process : processes) {
            std::cout << "   - " << process.processName << " (PID: " << process.processId << ")\n";
        }
        std::cout << "\n";

        // Test getting styles
        std::cout << "5. Testing style list retrieval...\n";
        auto styles = communicator->GetAvailableStyles();
        std::cout << "   Found " << styles.size() << " styles:\n";
        for (const auto& style : styles) {
            std::cout << "   - " << style.name << " (" << style.category << ")\n";
        }
        std::cout << "\n";

        // Test mode change (if processes exist)
        if (!processes.empty()) {
            std::cout << "6. Testing mode change...\n";
            if (communicator->SetMode(MacTypeManager::MacTypeMode::Tray)) {
                std::cout << "   ✓ Mode changed to Tray\n\n";
            } else {
                std::cout << "   ✗ Mode change failed\n\n";
            }

            // Test injection
            std::cout << "7. Testing process injection...\n";
            DWORD testProcessId = processes[0].processId;
            if (communicator->InjectIntoProcess(testProcessId)) {
                std::cout << "   ✓ Injection successful for process " << testProcessId << "\n\n";
            } else {
                std::cout << "   ✗ Injection failed for process " << testProcessId << "\n\n";
            }
        }

        // Test style application
        if (!styles.empty()) {
            std::cout << "8. Testing style application...\n";
            if (communicator->ApplyStyle(styles[0].name)) {
                std::cout << "   ✓ Style '" << styles[0].name << "' applied successfully\n\n";
            } else {
                std::cout << "   ✗ Style application failed\n\n";
            }
        }

        std::cout << "=== Test completed ===\n";

    } else {
        std::cout << "   ✗ Failed to connect to MacType agent\n";
        std::cout << "   (This is expected if mt64agnt is not running)\n\n";
    }

    // Disconnect
    communicator->Disconnect();
}

int main()
{
    std::cout << "MacType Manager Integration Test\n";
    std::cout << "================================\n\n";

    TestCommunication();

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
