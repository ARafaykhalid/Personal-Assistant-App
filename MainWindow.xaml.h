#pragma once
#include "MainWindow.g.h"
#include <functional>


using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows;

namespace winrt::Personal_Assistant::implementation
{
    struct MainWindow : MainWindowT<MainWindow> {

        MainWindow(); // Constructor declaration
        void RefreshTaskList();
        void AddTask(hstring const& title, hstring const& description);
        void CompleteTask_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SignInButton_Click(Foundation::IInspectable const& sender, RoutedEventArgs const& e);
        void ShowErrorMessage(hstring const& message);
		void OnCloseRequested();
        void RegistrationButton_Click(Foundation::IInspectable const& sender, RoutedEventArgs const& e);
        void StartProgressRingAnimation(Controls::ProgressRing progressRing, int delayMilliseconds, std::function<void()> onComplete);
        void Auth_Menu_SelectionChanged(Foundation::IInspectable const& sender, Controls::NavigationViewSelectionChangedEventArgs const& args);
        void MainMenu_SelectionChanged(Foundation::IInspectable const&, Controls::NavigationViewSelectionChangedEventArgs const& args);
        void LogOutButton_Click(Foundation::IInspectable const& sender, RoutedEventArgs const& e);
        void RefreshExpenses();
        void UpdateGreeting();
        void Sound_Toggled(Foundation::IInspectable const& sender, RoutedEventArgs const& e);
        void AddExpense_Click(Foundation::IInspectable const& sender, RoutedEventArgs const& e);
        void DeleteExpense_Click(Foundation::IInspectable const& sender, RoutedEventArgs const& e);

        void AddTaskButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void DeleteCompletedTask_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget SendMessageToGemini(winrt::hstring userMessage);
        void CreateTask(winrt::hstring taskName, winrt::hstring taskDesc);
        void AddExpense(double amount, winrt::hstring reason);
        void OpenLink(winrt::hstring url);
        void SendButton_Click(const winrt::Windows::Foundation::IInspectable&, const winrt::Microsoft::UI::Xaml::RoutedEventArgs&);
    };
}


namespace winrt::Personal_Assistant::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {

    };
}
