// MainWindow.xaml.h - MacType Manager Main Window Header
#pragma once

#include "MainWindow.xaml.g.h"

namespace winrt::MacTypeManager::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void InitializeComponent();

        // Navigation handlers
        void NavigationView_SelectionChanged(
            winrt::Windows::UI::Xaml::Controls::NavigationView const& sender,
            winrt::Windows::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);

        // Button handlers
        void SettingsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void AboutButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);

    private:
        // Page management
        void NavigateToPage(winrt::hstring const& tag);

        // Window management
        void SetupWindow();
        void LoadSavedSettings();

        // MacType communication
        bool EnsureMacTypeConnection();
        void UpdateStatus();

        // UI state
        winrt::hstring m_currentPage;
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
