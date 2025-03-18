#pragma once

#include "App.xaml.g.h"
#include "winrt/Windows.UI.Xaml.h"

namespace winrt::Personal_Assistant::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
