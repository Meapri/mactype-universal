// App.xaml.h - MacType Manager Application Header
#pragma once

#include "App.xaml.g.h"

namespace winrt::MacTypeManager::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);

        void OnSuspending(IInspectable const&, Windows::ApplicationModel::SuspendingEventArgs const&);

        void OnNavigationFailed(IInspectable const&, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);

    private:
        winrt::Windows::UI::Xaml::Window m_window{ nullptr };
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
