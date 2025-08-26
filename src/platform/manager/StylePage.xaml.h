// StylePage.xaml.h - Styles & Profiles Page Header
#pragma once

#include "StylePage.xaml.g.h"
#include "MacTypeCommunication.h"

namespace winrt::MacTypeManager::implementation
{
    struct StylePage : StylePageT<StylePage>
    {
        StylePage();

        // UI event handlers
        void ApplyStyleButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void SaveProfileButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void StyleGridView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void CategoryFilterComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void PreviewToggleButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void RefreshPreviewButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);

    private:
        // Communication
        std::shared_ptr<MacTypeManager::MacTypeCommunicator> m_communicator;

        // Style data
        std::vector<MacTypeManager::StyleInfo> m_availableStyles;
        std::string m_selectedStyleName;
        bool m_isPreviewMode = false;

        // UI updates
        void UpdateStyleGallery();
        void UpdatePreview();
        void UpdateButtons();

        // Style management
        winrt::Windows::Foundation::IAsyncAction InitializeCommunicationAsync();
        winrt::Windows::Foundation::IAsyncAction LoadAvailableStylesAsync();
        winrt::Windows::Foundation::IAsyncAction ApplySelectedStyleAsync();
        winrt::Windows::Foundation::IAsyncAction SaveCurrentProfileAsync();

        // Preview management
        void ShowPreviewText(const std::string& styleName);
        void ClearPreview();
        void TogglePreviewMode();

        // UI state management
        void SetButtonsEnabled(bool enabled);
        void ShowStatusMessage(winrt::hstring message, bool isError = false);

        // Style filtering and categorization
        std::vector<MacTypeManager::StyleInfo> GetStylesByCategory(const std::string& category);
        std::vector<std::string> GetAvailableCategories();
        winrt::hstring GetStyleDisplayName(const MacTypeManager::StyleInfo& style);
        winrt::hstring GetStyleDescription(const MacTypeManager::StyleInfo& style);

        // Initialization
        winrt::Windows::Foundation::IAsyncAction InitializePageAsync();
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct StylePage : StylePageT<StylePage, implementation::StylePage>
    {
    };
}
