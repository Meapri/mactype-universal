// StylePage.xaml.h - Styles & Profiles Page Header
#pragma once

#include "StylePage.xaml.g.h"

namespace winrt::MacTypeManager::implementation
{
    struct StylePage : StylePageT<StylePage>
    {
        StylePage();

        // TODO: Add style management methods
    };
}

namespace winrt::MacTypeManager::factory_implementation
{
    struct StylePage : StylePageT<StylePage, implementation::StylePage>
    {
    };
}
