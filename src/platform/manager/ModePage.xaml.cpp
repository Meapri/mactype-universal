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
        ResetModeBorders();
        SetButtonsEnabled(false);

        // Start initialization
        InitializePageAsync();
    }

    void ModePage::ModeBorder_Tapped(IInspectable const& sender, TappedRoutedEventArgs const& e)
    {
        if (auto border = sender.try_as<Border>())
        {
            if (border.Name() == L"ServiceModeBorder")
            {
                UpdateModeSelection(MacTypeManager::MacTypeMode::Service);
            }
            else if (border.Name() == L"TrayModeBorder")
            {
                UpdateModeSelection(MacTypeManager::MacTypeMode::Tray);
            }
            else if (border.Name() == L"ManualModeBorder")
            {
                UpdateModeSelection(MacTypeManager::MacTypeMode::Manual);
            }
        }
    }

    void ModePage::ApplyButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        ApplySelectedModeAsync();
    }

    void ModePage::RefreshButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        CheckMacTypeServiceAsync();
    }

    // Async implementation methods

    IAsyncAction ModePage::InitializePageAsync()
    {
        try
        {
            // Initialize communication
            co_await InitializeCommunicationAsync();

            // Load current status
            co_await CheckMacTypeServiceAsync();

            // Enable UI
            SetButtonsEnabled(true);

            ShowStatusMessage(L"Ready to manage MacType modes", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Failed to initialize: " + ex.message(), true);
            SetButtonsEnabled(false);
        }
    }

    IAsyncAction ModePage::InitializeCommunicationAsync()
    {
        m_communicator = std::make_shared<MacTypeManager::MacTypeCommunicator>();

        if (!m_communicator->ConnectToMacType())
        {
            throw winrt::hresult_error(E_FAIL, L"Failed to connect to MacType agent");
        }

        co_return;
    }

    void ModePage::UpdateModeSelection(MacTypeManager::MacTypeMode mode)
    {
        m_selectedMode = mode;
        HighlightModeBorder(mode);
    }

    void ModePage::UpdateStatusDisplay()
    {
        // Update current mode display
        ActiveModeText().Text(GetModeDisplayName(m_currentMode));

        // Update injected processes count (placeholder)
        InjectedProcessesText().Text(L"0"); // TODO: Get actual count from communicator
    }

    void ModePage::UpdateModeBorders()
    {
        ResetModeBorders();
        HighlightModeBorder(m_selectedMode);
    }

    void ModePage::HighlightModeBorder(MacTypeManager::MacTypeMode mode)
    {
        ResetModeBorders();

        switch (mode)
        {
        case MacTypeManager::MacTypeMode::Service:
            ServiceModeBorder().BorderBrush(Media::SolidColorBrush(Colors::DodgerBlue()));
            ServiceModeBorder().BorderThickness(Thickness{ 2 });
            ServiceModeBorder().Background(Media::SolidColorBrush(Colors::AliceBlue()));
            break;

        case MacTypeManager::MacTypeMode::Tray:
            TrayModeBorder().BorderBrush(Media::SolidColorBrush(Colors::DodgerBlue()));
            TrayModeBorder().BorderThickness(Thickness{ 2 });
            TrayModeBorder().Background(Media::SolidColorBrush(Colors::AliceBlue()));
            break;

        case MacTypeManager::MacTypeMode::Manual:
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

    IAsyncAction ModePage::CheckMacTypeServiceAsync()
    {
        if (!m_communicator)
        {
            co_return;
        }

        try
        {
            ServiceStatusRing().IsActive(true);
            ServiceStatusText().Text(L"Checking...");

            // Check service status
            bool serviceRunning = m_communicator->IsServiceRunning();
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

            // Get current mode
            co_await GetCurrentModeAsync();

            ShowStatusMessage(L"Status updated successfully", false);
        }
        catch (winrt::hresult_error const& ex)
        {
            ServiceStatusRing().IsActive(false);
            ServiceStatusText().Text(L"Error");
            ServiceStatusText().Foreground(Media::SolidColorBrush(Colors::Red()));

            ShowStatusMessage(L"Failed to check service status: " + ex.message(), true);
        }
    }

    IAsyncAction ModePage::ApplySelectedModeAsync()
    {
        if (!m_communicator || m_selectedMode == MacTypeManager::MacTypeMode::Unknown)
        {
            ShowStatusMessage(L"Please select a mode first", true);
            co_return;
        }

        try
        {
            SetButtonsEnabled(false);
            ShowStatusMessage(L"Applying mode...", false);

            // Apply selected mode
            if (m_communicator->SetMode(m_selectedMode))
            {
                m_currentMode = m_selectedMode;
                UpdateStatusDisplay();

                // Show success message
                auto dialog = ContentDialog();
                dialog.Title(box_value(L"Mode Applied"));
                dialog.Content(box_value(L"The selected MacType mode has been applied successfully."));
                dialog.CloseButtonText(L"OK");

                co_await dialog.ShowAsync();

                ShowStatusMessage(L"Mode applied successfully", false);
            }
            else
            {
                // Show error message
                auto dialog = ContentDialog();
                dialog.Title(box_value(L"Error"));
                dialog.Content(box_value(L"Failed to apply the selected mode. Please try again."));
                dialog.CloseButtonText(L"OK");

                co_await dialog.ShowAsync();

                ShowStatusMessage(L"Failed to apply mode", true);
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            ShowStatusMessage(L"Error applying mode: " + ex.message(), true);
        }

        SetButtonsEnabled(true);
    }

    IAsyncAction ModePage::GetCurrentModeAsync()
    {
        if (!m_communicator)
        {
            co_return;
        }

        try
        {
            m_currentMode = m_communicator->GetCurrentMode();
            ActiveModeText().Text(GetModeDisplayName(m_currentMode));

            // Update selection if not already selected
            if (m_selectedMode == MacTypeManager::MacTypeMode::Unknown)
            {
                UpdateModeSelection(m_currentMode);
            }
        }
        catch (winrt::hresult_error const&)
        {
            ActiveModeText().Text(L"Unknown");
        }
    }

    winrt::hstring ModePage::GetModeDisplayName(MacTypeManager::MacTypeMode mode)
    {
        switch (mode)
        {
        case MacTypeManager::MacTypeMode::Service:
            return L"Service Mode";
        case MacTypeManager::MacTypeMode::Tray:
            return L"Tray Mode";
        case MacTypeManager::MacTypeMode::Manual:
            return L"Manual Mode";
        default:
            return L"Unknown";
        }
    }

    void ModePage::SetButtonsEnabled(bool enabled)
    {
        ApplyButton().IsEnabled(enabled);
        RefreshButton().IsEnabled(enabled);

        ServiceModeBorder().IsHitTestVisible(enabled);
        TrayModeBorder().IsHitTestVisible(enabled);
        ManualModeBorder().IsHitTestVisible(enabled);
    }

    void ModePage::ShowStatusMessage(winrt::hstring message, bool isError)
    {
        // TODO: Implement status message display
        // For now, we could add a status text block to the UI
    }
}
