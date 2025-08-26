// App.xaml.cpp - MacType Manager Application Implementation
#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;

namespace winrt::MacTypeManager::implementation
{
    App::App()
    {
        InitializeComponent();

        // Initialize application resources
        Initialize();

        // Handle unhandled exceptions
        UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            // Log the exception
            auto errorMessage = std::wstring(L"Unhandled exception: ") + e.Message().c_str();

            // TODO: Add proper error handling and logging
            ::OutputDebugStringW(errorMessage.c_str());

            // Prevent the app from terminating
            e.Handled(true);
        });
    }

    void App::OnLaunched(LaunchActivatedEventArgs const& e)
    {
        Frame rootFrame{ nullptr };
        auto content = Window::Current().Content();

        if (content)
        {
            rootFrame = content.try_as<Frame>();
        }

        if (rootFrame == nullptr)
        {
            // Create a Frame to act as the navigation context
            rootFrame = Frame();
            rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });

            // Place the frame in the current Window
            Window::Current().Content(rootFrame);
        }

        if (e.PrelaunchActivated() == false)
        {
            if (rootFrame.Content() == nullptr)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame.Navigate(xaml_typename<MacTypeManager::MainWindow>(), box_value(e.Arguments()));
            }

            // Ensure the current window is active
            Window::Current().Activate();
        }

        // Set the window icon and title
        auto hwnd = winrt::Microsoft::UI::Win32Interop::GetWindowFromWindowId(
            winrt::Microsoft::UI::GetWindowIdFromWindow(Window::Current()));

        // TODO: Set window icon and properties
        // SetWindowTextW(hwnd, L"MacType Manager");
    }

    void App::OnSuspending(IInspectable const& sender, Windows::ApplicationModel::SuspendingEventArgs const& e)
    {
        // Save application state and stop any background activity
        auto deferral = e.SuspendingOperation().GetDeferral();

        // TODO: Save application state and stop any background activity

        deferral.Complete();
    }

    void App::OnNavigationFailed(IInspectable const& sender, NavigationFailedEventArgs const& e)
    {
        throw hresult_error(E_FAIL, hstring(L"Failed to load Page: ") + e.SourcePageType().Name);
    }
}
