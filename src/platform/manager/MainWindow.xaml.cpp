// MainWindow.xaml.cpp - MacType Manager Main Window Implementation
#include "pch.h"
#include "MainWindow.xaml.h"
#include "ModePage.xaml.h"
#include "ProcessPage.xaml.h"
#include "StylePage.xaml.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;

namespace winrt::MacTypeManager::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        SetupWindow();
        LoadSavedSettings();

        // Navigate to default page
        NavigateToPage(L"ModePage");
    }

    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();
    }

    void MainWindow::SetupWindow()
    {
        // Set window properties
        auto window = Window::Current();
        window.Title(L"MacType Manager");

        // Set minimum window size
        auto hwnd = winrt::Microsoft::UI::Win32Interop::GetWindowFromWindowId(
            winrt::Microsoft::UI::GetWindowIdFromWindow(window));

        if (hwnd)
        {
            // Set minimum size (800x600)
            ::SetWindowPos(hwnd, nullptr, 0, 0, 800, 600,
                          SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            ::SetWindowLongPtrW(hwnd, GWL_STYLE,
                              ::GetWindowLongPtrW(hwnd, GWL_STYLE) | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        }

        // Set Mica backdrop if supported
        if (winrt::Microsoft::UI::Composition::SystemBackdrops::MicaController::IsSupported())
        {
            // TODO: Implement Mica backdrop
        }
    }

    void MainWindow::LoadSavedSettings()
    {
        // Load window position and size
        // TODO: Implement settings persistence

        // Load last selected page
        // TODO: Implement page state persistence
    }

    void MainWindow::NavigationView_SelectionChanged(
        NavigationView const& sender,
        NavigationViewSelectionChangedEventArgs const& args)
    {
        if (auto item = args.SelectedItem().try_as<NavigationViewItem>())
        {
            auto tag = winrt::unbox_value<winrt::hstring>(item.Tag());
            NavigateToPage(tag);
        }
    }

    void MainWindow::NavigateToPage(winrt::hstring const& tag)
    {
        m_currentPage = tag;

        if (tag == L"ModePage")
        {
            ContentFrame().Navigate(xaml_typename<MacTypeManager::ModePage>());
        }
        else if (tag == L"ProcessPage")
        {
            ContentFrame().Navigate(xaml_typename<MacTypeManager::ProcessPage>());
        }
        else if (tag == L"StylePage")
        {
            ContentFrame().Navigate(xaml_typename<MacTypeManager::StylePage>());
        }

        // Update window title
        auto window = Window::Current();
        if (tag == L"ModePage")
            window.Title(L"MacType Manager - Start Mode");
        else if (tag == L"ProcessPage")
            window.Title(L"MacType Manager - Process Manager");
        else if (tag == L"StylePage")
            window.Title(L"MacType Manager - Styles & Profiles");
    }

    void MainWindow::SettingsButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        // TODO: Open settings dialog
        auto dialog = ContentDialog();
        dialog.Title(box_value(L"Settings"));
        dialog.Content(box_value(L"Settings dialog will be implemented here."));
        dialog.CloseButtonText(L"Close");

        auto result = co_await dialog.ShowAsync();
    }

    void MainWindow::AboutButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        // TODO: Open about dialog
        auto dialog = ContentDialog();
        dialog.Title(box_value(L"About MacType Manager"));
        dialog.Content(box_value(L"MacType Manager v1.0\n\nA modern management interface for MacType font rendering engine."));
        dialog.CloseButtonText(L"Close");

        auto result = co_await dialog.ShowAsync();
    }

    bool MainWindow::EnsureMacTypeConnection()
    {
        // TODO: Check if mt64agnt is running and establish connection
        // For now, return true as placeholder
        return true;
    }

    void MainWindow::UpdateStatus()
    {
        // TODO: Update status indicators based on MacType state
        // Update tray icon, service status, etc.
    }
}
