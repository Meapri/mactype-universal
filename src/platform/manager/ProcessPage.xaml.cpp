// ProcessPage.xaml.cpp - Process Manager Page Implementation
#include "pch.h"
#include "ProcessPage.xaml.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace winrt::MacTypeManager::implementation
{
    // Process list item data model
    struct ProcessListItem : winrt::implements<ProcessListItem, IInspectable>
    {
        ProcessListItem(const MacTypeManager::ProcessInfo& processInfo)
            : m_processInfo(processInfo)
        {
            m_processName = StringToHString(processInfo.processName);
            m_processId = L"PID: " + std::to_wstring(processInfo.processId);
            m_executablePath = StringToHString(processInfo.executablePath);
            m_architecture = processInfo.is64Bit ? L"x64" : L"x86";
            m_injectionStatus = processInfo.isInjected ? L"Injected" : L"Not Injected";
            m_memoryUsage = L"RAM: " + std::to_wstring(static_cast<int>(processInfo.memoryUsage)) + L" MB";
            m_cpuUsage = L"CPU: " + std::to_wstring(static_cast<int>(processInfo.cpuUsage)) + L" %";

            // Set status color based on injection state
            if (processInfo.isInjected)
            {
                m_injectionStatusColor = Media::SolidColorBrush(Colors::Green());
            }
            else
            {
                m_injectionStatusColor = Media::SolidColorBrush(Colors::Orange());
            }
        }

        winrt::hstring ProcessName() const { return m_processName; }
        winrt::hstring ProcessId() const { return m_processId; }
        winrt::hstring ExecutablePath() const { return m_executablePath; }
        winrt::hstring Architecture() const { return m_architecture; }
        winrt::hstring InjectionStatus() const { return m_injectionStatus; }
        winrt::hstring MemoryUsage() const { return m_memoryUsage; }
        winrt::hstring CpuUsage() const { return m_cpuUsage; }
        Media::SolidColorBrush InjectionStatusColor() const { return m_injectionStatusColor; }

        DWORD GetProcessId() const { return m_processInfo.processId; }
        bool IsInjected() const { return m_processInfo.isInjected; }

    private:
        MacTypeManager::ProcessInfo m_processInfo;
        winrt::hstring m_processName;
        winrt::hstring m_processId;
        winrt::hstring m_executablePath;
        winrt::hstring m_architecture;
        winrt::hstring m_injectionStatus;
        winrt::hstring m_memoryUsage;
        winrt::hstring m_cpuUsage;
        Media::SolidColorBrush m_injectionStatusColor{ nullptr };

        winrt::hstring StringToHString(const std::wstring& str)
        {
            return winrt::hstring(str);
        }
    };

    ProcessPage::ProcessPage()
    {
        InitializeComponent();

        // Start initialization
        InitializePageAsync();
    }

    void ProcessPage::RefreshButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        RefreshProcessListAsync();
    }

    void ProcessPage::InjectSelectedButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        InjectIntoSelectedProcessesAsync();
    }

    void ProcessPage::BatchInjectButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        // Batch inject all non-injected processes
        if (!m_communicator)
        {
            ShowStatusMessage(L"Communication not available", true);
            return;
        }

        std::vector<DWORD> processIdsToInject;
        for (const auto& process : m_processes)
        {
            if (!process.isInjected)
            {
                processIdsToInject.push_back(process.processId);
            }
        }

        if (processIdsToInject.empty())
        {
            ShowStatusMessage(L"No processes need injection", false);
            return;
        }

        // Use batch inject
        if (m_communicator->BatchInject(processIdsToInject))
        {
            ShowStatusMessage(L"Batch injection completed successfully", false);
            RefreshProcessListAsync();
        }
        else
        {
            ShowStatusMessage(L"Batch injection failed", true);
        }
    }

    void ProcessPage::ProcessListView_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        m_selectedProcessIds.clear();

        auto selectedItems = ProcessListView().SelectedItems();
        for (auto&& item : selectedItems)
        {
            auto processItem = item.try_as<ProcessListItem>();
            if (processItem)
            {
                m_selectedProcessIds.push_back(processItem.GetProcessId());
            }
        }

        // Update button states
        InjectSelectedButton().IsEnabled(!m_selectedProcessIds.empty());
    }

    void ProcessPage::UpdateProcessList()
    {
        auto items = single_threaded_observable_vector<IInspectable>();

        for (const auto& process : m_processes)
        {
            if (ShouldShowProcess(process))
            {
                auto item = winrt::make<ProcessListItem>(process);
                items.Append(item);
            }
        }

        ProcessListView().ItemsSource(items);
        UpdateProcessStats();
    }

    void ProcessPage::UpdateProcessStats()
    {
        size_t totalProcesses = m_processes.size();
        size_t injectedProcesses = 0;
        size_t x64Processes = 0;
        size_t x86Processes = 0;

        for (const auto& process : m_processes)
        {
            if (process.isInjected) injectedProcesses++;
            if (process.is64Bit) x64Processes++;
            else x86Processes++;
        }

        TotalProcessesText().Text(std::to_wstring(totalProcesses).c_str());
        InjectedProcessesText().Text(std::to_wstring(injectedProcesses).c_str());
        X64ProcessesText().Text(std::to_wstring(x64Processes).c_str());
        X86ProcessesText().Text(std::to_wstring(x86Processes).c_str());
    }

    IAsyncAction ProcessPage::InitializeCommunicationAsync()
    {
        m_communicator = std::make_shared<MacTypeManager::MacTypeCommunicator>();

        if (!m_communicator->ConnectToMacType())
        {
            throw winrt::hresult_error(E_FAIL, L"Failed to connect to MacType agent");
        }

        co_return;
    }

    IAsyncAction ProcessPage::RefreshProcessListAsync()
    {
        if (!m_communicator)
        {
            co_return;
        }

        try
        {
            RefreshProgressRing().IsActive(true);
            SetButtonsEnabled(false);

            // Get process list
            m_processes = m_communicator->GetProcessList();
            UpdateProcessList();

            ShowStatusMessage(L"Process list updated successfully", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Failed to refresh process list: " + ex.message(), true);
        }

        RefreshProgressRing().IsActive(false);
        SetButtonsEnabled(true);
    }

    IAsyncAction ProcessPage::InjectIntoSelectedProcessesAsync()
    {
        if (!m_communicator || m_selectedProcessIds.empty())
        {
            ShowStatusMessage(L"No processes selected", true);
            co_return;
        }

        try
        {
            SetButtonsEnabled(false);
            ShowStatusMessage(L"Injecting into selected processes...", false);

            size_t successCount = 0;
            for (DWORD processId : m_selectedProcessIds)
            {
                if (m_communicator->InjectIntoProcess(processId))
                {
                    successCount++;
                }
            }

            if (successCount == m_selectedProcessIds.size())
            {
                ShowStatusMessage(L"All injections completed successfully", false);
            }
            else if (successCount > 0)
            {
                ShowStatusMessage(L"Partial success: " + std::to_wstring(successCount) +
                                L" of " + std::to_wstring(m_selectedProcessIds.size()) + L" injections succeeded", false);
            }
            else
            {
                ShowStatusMessage(L"All injections failed", true);
            }

            // Refresh the list to show updated status
            co_await RefreshProcessListAsync();
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Injection failed: " + ex.message(), true);
        }

        SetButtonsEnabled(true);
    }

    IAsyncAction ProcessPage::InjectIntoProcessAsync(DWORD processId)
    {
        if (!m_communicator)
        {
            co_return;
        }

        if (m_communicator->InjectIntoProcess(processId))
        {
            ShowStatusMessage(L"Injection successful", false);
        }
        else
        {
            ShowStatusMessage(L"Injection failed", true);
        }

        co_await RefreshProcessListAsync();
    }

    IAsyncAction ProcessPage::InitializePageAsync()
    {
        try
        {
            // Initialize communication
            co_await InitializeCommunicationAsync();

            // Load initial process list
            co_await RefreshProcessListAsync();

            // Setup auto-refresh timer
            SetupAutoRefresh();

            ShowStatusMessage(L"Process manager initialized successfully", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Failed to initialize: " + ex.message(), true);
        }
    }

    void ProcessPage::SetupAutoRefresh()
    {
        m_refreshTimer = DispatcherTimer();
        m_refreshTimer.Interval(std::chrono::seconds(30)); // Refresh every 30 seconds
        m_refreshTimer.Tick({ this, &ProcessPage::RefreshTimer_Tick });
        m_refreshTimer.Start();
    }

    void ProcessPage::RefreshTimer_Tick(IInspectable const& sender, IInspectable const& e)
    {
        // Auto-refresh process list
        RefreshProcessListAsync();
    }

    void ProcessPage::SetButtonsEnabled(bool enabled)
    {
        RefreshButton().IsEnabled(enabled);
        InjectSelectedButton().IsEnabled(enabled && !m_selectedProcessIds.empty());
        BatchInjectButton().IsEnabled(enabled);
    }

    void ProcessPage::ShowStatusMessage(winrt::hstring message, bool isError)
    {
        // TODO: Implement status message display
        // For now, we could add a status text block to the UI
        OutputDebugStringW((message + L"\n").c_str());
    }

    bool ProcessPage::ShouldShowProcess(const MacTypeManager::ProcessInfo& process)
    {
        // Filter out system processes and our own process
        static const std::vector<std::wstring> filteredProcesses = {
            L"System",
            L"Registry",
            L"smss.exe",
            L"csrss.exe",
            L"wininit.exe",
            L"services.exe",
            L"lsass.exe",
            L"svchost.exe",
            L"explorer.exe", // Usually already injected
            L"MacTypeManager.exe" // Our own process
        };

        for (const auto& filtered : filteredProcesses)
        {
            if (process.processName == filtered)
            {
                return false;
            }
        }

        return true;
    }

    winrt::hstring ProcessPage::GetProcessDisplayName(const MacTypeManager::ProcessInfo& process)
    {
        return StringToHString(process.processName);
    }

    winrt::hstring ProcessPage::GetProcessArchitectureText(const MacTypeManager::ProcessInfo& process)
    {
        return process.is64Bit ? L"x64" : L"x86";
    }

    Media::SolidColorBrush ProcessPage::GetInjectionStatusColor(const MacTypeManager::ProcessInfo& process)
    {
        if (process.isInjected)
        {
            return Media::SolidColorBrush(Colors::Green());
        }
        else
        {
            return Media::SolidColorBrush(Colors::Orange());
        }
    }

    winrt::hstring ProcessPage::StringToHString(const std::string& str)
    {
        return winrt::hstring(std::string_view(str));
    }
}
