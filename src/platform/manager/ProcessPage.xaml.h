// ProcessPage.xaml.h - Process Manager Page Header
#pragma once

#include "ProcessPage.xaml.g.h"

namespace winrt::MacTypeManager::implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage>
    {
        ProcessPage();

        // TODO: Add process management methods
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage, implementation::ProcessPage>
    {
    };
}
