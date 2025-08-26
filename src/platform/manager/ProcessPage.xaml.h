// ProcessPage.xaml.h - Process Manager Page Header
#pragma once

#include "ProcessPage.xaml.g.h"
#include "MacTypeCommunication.h"

namespace winrt::MacTypeManager::implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage>
    {
        ProcessPage();

        // UI event handlers
        void RefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void InjectSelectedButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void BatchInjectButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void ProcessListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

    private:
        // Communication
        std::shared_ptr<MacTypeManager::MacTypeCommunicator> m_communicator;

        // Process data
        std::vector<MacTypeManager::ProcessInfo> m_processes;
        std::vector<DWORD> m_selectedProcessIds;

        // UI updates
        void UpdateProcessList();
        void UpdateProcessStats();

        // Process management
        winrt::Windows::Foundation::IAsyncAction InitializeCommunicationAsync();
        winrt::Windows::Foundation::IAsyncAction RefreshProcessListAsync();
        winrt::Windows::Foundation::IAsyncAction InjectIntoSelectedProcessesAsync();
        winrt::Windows::Foundation::IAsyncAction InjectIntoProcessAsync(DWORD processId);

        // UI state management
        void SetButtonsEnabled(bool enabled);
        void ShowStatusMessage(winrt::hstring message, bool isError = false);

        // Process filtering and display
        bool ShouldShowProcess(const MacTypeManager::ProcessInfo& process);
        winrt::hstring GetProcessDisplayName(const MacTypeManager::ProcessInfo& process);
        winrt::hstring GetProcessArchitectureText(const MacTypeManager::ProcessInfo& process);
        winrt::Windows::UI::Xaml::Media::SolidColorBrush GetInjectionStatusColor(const MacTypeManager::ProcessInfo& process);

        // Initialization
        winrt::Windows::Foundation::IAsyncAction InitializePageAsync();

        // Timer for auto-refresh
        winrt::Windows::UI::Xaml::DispatcherTimer m_refreshTimer{ nullptr };
        void SetupAutoRefresh();
        void RefreshTimer_Tick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage, implementation::ProcessPage>
    {
    };
}
