// StylePage.xaml.cpp - Styles & Profiles Page Implementation
#include "pch.h"
#include "StylePage.xaml.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace winrt::MacTypeManager::implementation
{
    // Style item data model for GridView
    struct StyleGridItem : winrt::implements<StyleGridItem, IInspectable>
    {
        StyleGridItem(const MacTypeManager::StyleInfo& styleInfo)
            : m_styleInfo(styleInfo)
        {
            m_name = StringToHString(styleInfo.name);
            m_description = StringToHString(styleInfo.description);
            m_category = StringToHString(styleInfo.category);
            m_previewText = StringToHString(styleInfo.previewText);
        }

        winrt::hstring Name() const { return m_name; }
        winrt::hstring Description() const { return m_description; }
        winrt::hstring Category() const { return m_category; }
        winrt::hstring PreviewText() const { return m_previewText; }
        bool IsBuiltIn() const { return m_styleInfo.isBuiltIn; }

        std::string GetStyleName() const { return m_styleInfo.name; }

    private:
        MacTypeManager::StyleInfo m_styleInfo;
        winrt::hstring m_name;
        winrt::hstring m_description;
        winrt::hstring m_category;
        winrt::hstring m_previewText;

        winrt::hstring StringToHString(const std::string& str)
        {
            return winrt::hstring(std::string_view(str));
        }
    };

    StylePage::StylePage()
    {
        InitializeComponent();

        // Start initialization
        InitializePageAsync();
    }

    void StylePage::ApplyStyleButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        ApplySelectedStyleAsync();
    }

    void StylePage::SaveProfileButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        SaveCurrentProfileAsync();
    }

    void StylePage::StyleGridView_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        auto selectedItem = StyleGridView().SelectedItem();
        if (selectedItem)
        {
            auto styleItem = selectedItem.try_as<StyleGridItem>();
            if (styleItem)
            {
                m_selectedStyleName = styleItem.GetStyleName();
                UpdateSelectedStyleInfo(styleItem);
                UpdatePreview();
            }
        }
        else
        {
            m_selectedStyleName.clear();
            SelectedStyleInfoBorder().Visibility(Visibility::Collapsed);
        }

        UpdateButtons();
    }

    void StylePage::CategoryFilterComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        UpdateStyleGallery();
    }

    void StylePage::PreviewToggleButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        TogglePreviewMode();
    }

    void StylePage::RefreshPreviewButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        UpdatePreview();
    }

    void StylePage::UpdateStyleGallery()
    {
        auto items = single_threaded_observable_vector<IInspectable>();

        // Get selected category
        std::string selectedCategory = "all";
        auto selectedItem = CategoryFilterComboBox().SelectedItem();
        if (selectedItem)
        {
            auto comboBoxItem = selectedItem.try_as<ComboBoxItem>();
            if (comboBoxItem)
            {
                auto tag = winrt::unbox_value<winrt::hstring>(comboBoxItem.Tag());
                selectedCategory = std::string(tag.c_str());
            }
        }

        // Filter styles by category
        auto filteredStyles = (selectedCategory == "all") ?
            m_availableStyles :
            GetStylesByCategory(selectedCategory);

        // Create grid items
        for (const auto& style : filteredStyles)
        {
            auto item = winrt::make<StyleGridItem>(style);
            items.Append(item);
        }

        StyleGridView().ItemsSource(items);
    }

    void StylePage::UpdateSelectedStyleInfo(StyleGridItem const& styleItem)
    {
        SelectedStyleNameText().Text(styleItem.Name());
        SelectedStyleDescriptionText().Text(styleItem.Description());
        SelectedStyleCategoryText().Text(styleItem.Category());
        SelectedStyleInfoBorder().Visibility(Visibility::Visible);
    }

    void StylePage::UpdatePreview()
    {
        if (m_selectedStyleName.empty())
        {
            ClearPreview();
            return;
        }

        // Show preview for selected style
        ShowPreviewText(m_selectedStyleName);
    }

    void StylePage::UpdateButtons()
    {
        ApplyStyleButton().IsEnabled(!m_selectedStyleName.empty());
        SaveProfileButton().IsEnabled(!m_availableStyles.empty());
    }

    IAsyncAction StylePage::InitializeCommunicationAsync()
    {
        m_communicator = std::make_shared<MacTypeManager::MacTypeCommunicator>();

        if (!m_communicator->ConnectToMacType())
        {
            throw winrt::hresult_error(E_FAIL, L"Failed to connect to MacType agent");
        }

        co_return;
    }

    IAsyncAction StylePage::LoadAvailableStylesAsync()
    {
        if (!m_communicator)
        {
            co_return;
        }

        try
        {
            StyleLoadingRing().IsActive(true);
            SetButtonsEnabled(false);

            // Get available styles
            m_availableStyles = m_communicator->GetAvailableStyles();
            UpdateStyleGallery();

            ShowStatusMessage(L"Styles loaded successfully", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Failed to load styles: " + ex.message(), true);
        }

        StyleLoadingRing().IsActive(false);
        SetButtonsEnabled(true);
    }

    IAsyncAction StylePage::ApplySelectedStyleAsync()
    {
        if (!m_communicator || m_selectedStyleName.empty())
        {
            ShowStatusMessage(L"No style selected", true);
            co_return;
        }

        try
        {
            SetButtonsEnabled(false);
            ShowStatusMessage(L"Applying style...", false);

            // Apply selected style
            if (m_communicator->ApplyStyle(m_selectedStyleName))
            {
                // Show success message
                auto dialog = ContentDialog();
                dialog.Title(box_value(L"Style Applied"));
                dialog.Content(box_value(L"The selected style has been applied successfully."));
                dialog.CloseButtonText(L"OK");

                co_await dialog.ShowAsync();

                ShowStatusMessage(L"Style applied successfully", false);
            }
            else
            {
                // Show error message
                auto dialog = ContentDialog();
                dialog.Title(box_value(L"Error"));
                dialog.Content(box_value(L"Failed to apply the selected style. Please try again."));
                dialog.CloseButtonText(L"OK");

                co_await dialog.ShowAsync();

                ShowStatusMessage(L"Failed to apply style", true);
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Error applying style: " + ex.message(), true);
        }

        SetButtonsEnabled(true);
    }

    IAsyncAction StylePage::SaveCurrentProfileAsync()
    {
        try
        {
            SetButtonsEnabled(false);
            ShowStatusMessage(L"Saving profile...", false);

            // Generate profile name based on current timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "Profile_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
            std::string profileName = ss.str();

            // Save profile
            if (m_communicator->SaveProfile(profileName))
            {
                // Show success message
                auto dialog = ContentDialog();
                dialog.Title(box_value(L"Profile Saved"));
                dialog.Content(box_value(L"Current settings have been saved as profile: " + StringToHString(profileName)));
                dialog.CloseButtonText(L"OK");

                co_await dialog.ShowAsync();

                ShowStatusMessage(L"Profile saved successfully", false);
            }
            else
            {
                ShowStatusMessage(L"Failed to save profile", true);
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Error saving profile: " + ex.message(), true);
        }

        SetButtonsEnabled(true);
    }

    IAsyncAction StylePage::InitializePageAsync()
    {
        try
        {
            // Initialize communication
            co_await InitializeCommunicationAsync();

            // Load available styles
            co_await LoadAvailableStylesAsync();

            // Update UI
            UpdateButtons();

            ShowStatusMessage(L"Style manager initialized successfully", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Failed to initialize: " + ex.message(), true);
        }
    }

    void StylePage::ShowPreviewText(const std::string& styleName)
    {
        // Find the style
        auto it = std::find_if(m_availableStyles.begin(), m_availableStyles.end(),
            [styleName](const MacTypeManager::StyleInfo& style) {
                return style.name == styleName;
            });

        if (it != m_availableStyles.end())
        {
            PreviewStyleTextBlock().Text(StringToHString(it->previewText));
        }
    }

    void StylePage::ClearPreview()
    {
        PreviewStyleTextBlock().Text(L"No style selected");
        CurrentStyleTextBlock().Text(L"Current style");
    }

    void StylePage::TogglePreviewMode()
    {
        m_isPreviewMode = PreviewToggleButton().IsChecked().Value();
        // TODO: Implement preview mode toggle functionality
    }

    void StylePage::SetButtonsEnabled(bool enabled)
    {
        ApplyStyleButton().IsEnabled(enabled && !m_selectedStyleName.empty());
        SaveProfileButton().IsEnabled(enabled && !m_availableStyles.empty());
        RefreshPreviewButton().IsEnabled(enabled);
        CategoryFilterComboBox().IsEnabled(enabled);
    }

    void StylePage::ShowStatusMessage(winrt::hstring message, bool isError)
    {
        // TODO: Implement status message display
        // For now, we could add a status text block to the UI
        OutputDebugStringW((message + L"\n").c_str());
    }

    std::vector<MacTypeManager::StyleInfo> StylePage::GetStylesByCategory(const std::string& category)
    {
        std::vector<MacTypeManager::StyleInfo> filtered;

        for (const auto& style : m_availableStyles)
        {
            if (category == "builtin" && style.isBuiltIn)
            {
                filtered.push_back(style);
            }
            else if (style.category == category)
            {
                filtered.push_back(style);
            }
        }

        return filtered;
    }

    std::vector<std::string> StylePage::GetAvailableCategories()
    {
        std::vector<std::string> categories;

        for (const auto& style : m_availableStyles)
        {
            if (std::find(categories.begin(), categories.end(), style.category) == categories.end())
            {
                categories.push_back(style.category);
            }
        }

        return categories;
    }

    winrt::hstring StylePage::GetStyleDisplayName(const MacTypeManager::StyleInfo& style)
    {
        return StringToHString(style.name);
    }

    winrt::hstring StylePage::GetStyleDescription(const MacTypeManager::StyleInfo& style)
    {
        return StringToHString(style.description);
    }

    winrt::hstring StylePage::StringToHString(const std::string& str)
    {
        return winrt::hstring(std::string_view(str));
    }
}
