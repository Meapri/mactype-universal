// ModePage.xaml.cpp - Start Mode Selection Page Implementation
#include "pch.h"
#include "ModePage.xaml.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;

namespace winrt::MacTypeManager::implementation
{
    ModePage::ModePage()
    {
        InitializeComponent();

        // Initialize page state
        UpdateStatusDisplay();
        ResetModeBorders();

        // Load current mode
        m_currentMode = GetCurrentMode();
        UpdateModeSelection(m_currentMode);
    }

    void ModePage::ModeBorder_Tapped(IInspectable const& sender, TappedRoutedEventArgs const& e)
    {
        if (auto border = sender.try_as<Border>())
        {
            if (border.Name() == L"ServiceModeBorder")
            {
                UpdateModeSelection(MacTypeMode::Service);
            }
            else if (border.Name() == L"TrayModeBorder")
            {
                UpdateModeSelection(MacTypeMode::Tray);
            }
            else if (border.Name() == L"ManualModeBorder")
            {
                UpdateModeSelection(MacTypeMode::Manual);
            }
        }
    }

    void ModePage::ApplyButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        if (ApplySelectedMode())
        {
            // Show success message
            auto dialog = ContentDialog();
            dialog.Title(box_value(L"Mode Applied"));
            dialog.Content(box_value(L"The selected MacType mode has been applied successfully."));
            dialog.CloseButtonText(L"OK");

            auto result = co_await dialog.ShowAsync();

            // Refresh status
            UpdateStatusDisplay();
        }
        else
        {
            // Show error message
            auto dialog = ContentDialog();
            dialog.Title(box_value(L"Error"));
            dialog.Content(box_value(L"Failed to apply the selected mode. Please try again."));
            dialog.CloseButtonText(L"OK");

            auto result = co_await dialog.ShowAsync();
        }
    }

    void ModePage::RefreshButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        UpdateStatusDisplay();
    }

    void ModePage::UpdateModeSelection(MacTypeMode mode)
    {
        m_selectedMode = mode;
        HighlightModeBorder(mode);
    }

    void ModePage::UpdateStatusDisplay()
    {
        // Check service status
        bool serviceRunning = CheckMacTypeService();
        ServiceStatusRing().IsActive(false);

        if (serviceRunning)
        {
            ServiceStatusText().Text(L"Running");
            ServiceStatusText().Foreground(Media::SolidColorBrush(Colors::Green()));
        }
        else
        {
            ServiceStatusText().Text(L"Stopped");
            ServiceStatusText().Foreground(Media::SolidColorBrush(Colors::Red()));
        }

        // Update current mode
        m_currentMode = GetCurrentMode();
        ActiveModeText().Text(GetModeDisplayName(m_currentMode));

        // Update injected processes count (placeholder)
        InjectedProcessesText().Text(L"0"); // TODO: Get actual count from mt64agnt
    }

    void ModePage::UpdateModeBorders()
    {
        ResetModeBorders();
        HighlightModeBorder(m_selectedMode);
    }

    void ModePage::HighlightModeBorder(MacTypeMode mode)
    {
        ResetModeBorders();

        switch (mode)
        {
        case MacTypeMode::Service:
            ServiceModeBorder().BorderBrush(Media::SolidColorBrush(Colors::DodgerBlue()));
            ServiceModeBorder().BorderThickness(Thickness{ 2 });
            ServiceModeBorder().Background(Media::SolidColorBrush(Colors::AliceBlue()));
            break;

        case MacTypeMode::Tray:
            TrayModeBorder().BorderBrush(Media::SolidColorBrush(Colors::DodgerBlue()));
            TrayModeBorder().BorderThickness(Thickness{ 2 });
            TrayModeBorder().Background(Media::SolidColorBrush(Colors::AliceBlue()));
            break;

        case MacTypeMode::Manual:
            ManualModeBorder().BorderBrush(Media::SolidColorBrush(Colors::DodgerBlue()));
            ManualModeBorder().BorderThickness(Thickness{ 2 });
            ManualModeBorder().Background(Media::SolidColorBrush(Colors::AliceBlue()));
            break;
        }
    }

    void ModePage::ResetModeBorders()
    {
        ServiceModeBorder().BorderBrush(Media::SolidColorBrush(Colors::Transparent()));
        ServiceModeBorder().BorderThickness(Thickness{ 1 });
        ServiceModeBorder().Background(Media::SolidColorBrush(Colors::Transparent()));

        TrayModeBorder().BorderBrush(Media::SolidColorBrush(Colors::Transparent()));
        TrayModeBorder().BorderThickness(Thickness{ 1 });
        TrayModeBorder().Background(Media::SolidColorBrush(Colors::Transparent()));

        ManualModeBorder().BorderBrush(Media::SolidColorBrush(Colors::Transparent()));
        ManualModeBorder().BorderThickness(Thickness{ 1 });
        ManualModeBorder().Background(Media::SolidColorBrush(Colors::Transparent()));
    }

    bool ModePage::CheckMacTypeService()
    {
        // TODO: Check if mt64agnt is running
        // For now, return false as placeholder
        return false;
    }

    bool ModePage::ApplySelectedMode()
    {
        // TODO: Apply selected mode to MacType
        // This involves communicating with mt64agnt to change execution mode

        // Placeholder implementation
        return true;
    }

    ModePage::MacTypeMode ModePage::GetCurrentMode()
    {
        // TODO: Query current mode from mt64agnt
        // For now, return Service as placeholder
        return MacTypeMode::Service;
    }

    winrt::hstring ModePage::GetModeDisplayName(MacTypeMode mode)
    {
        switch (mode)
        {
        case MacTypeMode::Service:
            return L"Service Mode";
        case MacTypeMode::Tray:
            return L"Tray Mode";
        case MacTypeMode::Manual:
            return L"Manual Mode";
        default:
            return L"Unknown";
        }
    }
}
