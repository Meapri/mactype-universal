// ModePage.xaml.h - Start Mode Selection Page Header
#pragma once

#include "ModePage.xaml.g.h"
#include "MacTypeCommunication.h"

namespace winrt::MacTypeManager::implementation
{
    struct ModePage : ModePageT<ModePage>
    {
        ModePage();

        // UI event handlers
        void ModeBorder_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void ApplyButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void RefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);

    private:
        // Mode management
        MacTypeManager::MacTypeMode m_selectedMode = MacTypeManager::MacTypeMode::Unknown;
        MacTypeManager::MacTypeMode m_currentMode = MacTypeManager::MacTypeMode::Unknown;

        // Communication
        std::shared_ptr<MacTypeManager::MacTypeCommunicator> m_communicator;

        // UI updates
        void UpdateModeSelection(MacTypeManager::MacTypeMode mode);
        void UpdateStatusDisplay();
        void UpdateModeBorders();

        // MacType communication
        winrt::Windows::Foundation::IAsyncAction InitializeCommunicationAsync();
        winrt::Windows::Foundation::IAsyncAction CheckMacTypeServiceAsync();
        winrt::Windows::Foundation::IAsyncAction ApplySelectedModeAsync();
        winrt::Windows::Foundation::IAsyncAction GetCurrentModeAsync();
        winrt::hstring GetModeDisplayName(MacTypeManager::MacTypeMode mode);

        // Visual feedback
        void HighlightModeBorder(MacTypeManager::MacTypeMode mode);
        void ResetModeBorders();
        void SetButtonsEnabled(bool enabled);
        void ShowStatusMessage(winrt::hstring message, bool isError = false);

        // Initialization
        winrt::Windows::Foundation::IAsyncAction InitializePageAsync();
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct ModePage : ModePageT<ModePage, implementation::ModePage>
    {
    };
}
