// ModePage.xaml.h - Start Mode Selection Page Header
#pragma once

#include "ModePage.xaml.g.h"

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
        enum class MacTypeMode
        {
            Service,
            Tray,
            Manual,
            Unknown
        };

        MacTypeMode m_selectedMode = MacTypeMode::Unknown;
        MacTypeMode m_currentMode = MacTypeMode::Unknown;

        // UI updates
        void UpdateModeSelection(MacTypeMode mode);
        void UpdateStatusDisplay();
        void UpdateModeBorders();

        // MacType communication
        bool CheckMacTypeService();
        bool ApplySelectedMode();
        MacTypeMode GetCurrentMode();
        winrt::hstring GetModeDisplayName(MacTypeMode mode);

        // Visual feedback
        void HighlightModeBorder(MacTypeMode mode);
        void ResetModeBorders();
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct ModePage : ModePageT<ModePage, implementation::ModePage>
    {
    };
}
