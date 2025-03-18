#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <WebView2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <winrt/Windows.Data.Json.h> 
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Security.Cryptography.h>
#include "winrt/Windows.Globalization.DateTimeFormatting.h"
#include "winrt/Microsoft.UI.Dispatching.h"
#include <winrt/Windows.Web.Http.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sapi.h>

#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")
#pragma warning(push)
#pragma warning(disable: 4996)
#include <sphelper.h>
#pragma warning(pop)

using namespace winrt;
using namespace winrt::Windows;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::Streams;
using namespace std::chrono;
using namespace Windows::Globalization::DateTimeFormatting;
using json = nlohmann::json;

namespace winrt::Personal_Assistant::implementation
{

    // Structure
    struct User {
        std::wstring username;
        std::wstring password;
        std::wstring name;
        int age;
        bool loggedIn = false;
        int totalexpense = 0;
        std::vector<winrt::hstring> expenseVector;
        std::vector<std::pair<winrt::hstring, hstring>> uncompletedTasks;
        std::vector<std::pair<hstring, hstring>> completedTasks;
    };

    // Global Variables
    std::vector<User> users;
    bool LoggedIn = false;
    std::wstring LoggedInUsername;
    std::wstring LoggedInName;
    int LoggedInAge;
    int totalexpense;
    std::vector<hstring> ExpenseVector;
    std::vector<std::pair<hstring, hstring>> UncompletedTasks;
    std::vector<std::pair<hstring, hstring>> CompletedTasks;

    // String Conversions
    std::string WStringToString(const std::wstring& wstr) {
        return to_string(wstr);
    }

    std::wstring StringToWString(const std::string& str) {
        return to_hstring(str).c_str();
    }

    void MainWindow::RefreshTaskList()
    {
        UncompletedTaskList().Items().Clear();
        for (const auto& task : UncompletedTasks)
        {
            auto taskObject = Collections::PropertySet();
            taskObject.Insert(L"Title", box_value(task.first));
            taskObject.Insert(L"Description", box_value(task.second));

            UncompletedTaskList().Items().Append(taskObject);
        }
        CompletedTaskList().Items().Clear();
        for (const auto& task : CompletedTasks)
        {
            auto taskObject = Collections::PropertySet();
            taskObject.Insert(L"Title", box_value(task.first));
            taskObject.Insert(L"Description", box_value(task.second));

            CompletedTaskList().Items().Append(taskObject);
        }
    }

    void MainWindow::RefreshExpenses() {
        ExpenseList().Items().Clear();
        for (const auto& item : ExpenseVector)
        {
            ExpenseList().Items().Append(box_value(item));
        }
        TotalExpense().Text(std::to_wstring(totalexpense));
    }

    const std::string BASE64_STANDARD = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const std::string BASE64_CUSTOM = "QWERTYUIOPASDFGHJKLZXCVBNMghjklzxcvbnmqwertyuiopasdf5678901234-*_";

    std::string Base64Encode(const std::string& input) {
        DWORD base64Length = 0;
        CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input.data()), input.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &base64Length);

        std::string base64(base64Length, '\0');
        CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input.data()), input.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &base64[0], &base64Length);

        // Replace standard Base64 chars with custom ones
        for (char& c : base64) {
            size_t index = BASE64_STANDARD.find(c);
            if (index != std::string::npos) {
                c = BASE64_CUSTOM[index];
            }
        }

        return base64;
    }

    std::string Base64Decode(const std::string& input) {
        std::string modifiedBase64 = input;

        // Replace custom chars back to standard Base64
        for (char& c : modifiedBase64) {
            size_t index = BASE64_CUSTOM.find(c);
            if (index != std::string::npos) {
                c = BASE64_STANDARD[index];
            }
        }

        DWORD binaryLength = 0;
        CryptStringToBinaryA(modifiedBase64.c_str(), modifiedBase64.size(), CRYPT_STRING_BASE64, NULL, &binaryLength, NULL, NULL);

        std::vector<BYTE> binaryData(binaryLength);
        CryptStringToBinaryA(modifiedBase64.c_str(), modifiedBase64.size(), CRYPT_STRING_BASE64, binaryData.data(), &binaryLength, NULL, NULL);

        return std::string(binaryData.begin(), binaryData.end());
    }

    std::string Base64EncodeInt(int num) {
        return Base64Encode(std::to_string(num));
    }

    int Base64DecodeInt(const std::string& encoded) {
        return std::stoi(Base64Decode(encoded));
    }

    void MainWindow::UpdateGreeting()
    {

        // Get the current time
        auto now = system_clock::to_time_t(system_clock::now());
        struct tm localTime;
        localtime_s(&localTime, &now);

        // Format date separately
        wchar_t dateBuffer[50];
        wcsftime(dateBuffer, sizeof(dateBuffer), L"%A, %B %d, %Y", &localTime);
        std::wstring dateStr = dateBuffer;

        // Format time separately
        wchar_t timeBuffer[20];
        wcsftime(timeBuffer, sizeof(timeBuffer), L"%I:%M %p", &localTime);
        std::wstring timeStr = timeBuffer;

        // Determine greeting
        int hour = localTime.tm_hour;
        std::wstring greeting;
        if (hour < 12){
                greeting = L"Good Morning";
        } else if (hour < 18) {
            greeting = L"Good Afternoon";
        } else {
            greeting = L"Good Evening";
        }

        std::wstring TheName;
        for (const auto& user : users) {
            if (user.username == LoggedInUsername) {
				TheName = user.name;
            }
        }

        // Update TextBlock in UI thread
        auto dispatcher = this->Dispatcher();
        DispatcherQueue().TryEnqueue([this, dateStr, timeStr, TheName, greeting]()
            {
                GreetingTextBlock().Text(greeting + L" " + TheName + L"!");
                DateBlock().Text(L"Date: " + dateStr);
                TimeBlock().Text(L"Time: " + timeStr);
            });
    }

    void MainWindow::StartProgressRingAnimation(Controls::ProgressRing progressRing, int delayMilliseconds, std::function<void()> onComplete)
    {

        progressRing.Visibility(Visibility::Visible);
        progressRing.Value(0);

        std::thread([this, progressRing, delayMilliseconds, onComplete]() {
            for (int i = 0; i <= 100; ++i)
            {
                DispatcherQueue().TryEnqueue([progressRing, i]() {
                    progressRing.Value(i);
                    });
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));
            }

            DispatcherQueue().TryEnqueue([progressRing, onComplete]()
                {
                    progressRing.Visibility(Visibility::Collapsed);
                    if (onComplete)
                    {
                        onComplete();
                    }
                });

            }).detach();

    }
    void ShowAuthError(Controls::InfoBar errorBar, Controls::InfoBarSeverity severity, hstring message)
    {
        errorBar.Severity(severity);
        errorBar.Message(message);
        errorBar.IsOpen(true);
    }
    void MainWindow::ShowErrorMessage(hstring const& message)
    {
        ErrorForMenu().Subtitle(message);
        ErrorForMenu().IsOpen(true);
    }

    int ExtractAmountFromString(const hstring& expenseString)
    {
        std::wstring str(expenseString);

        size_t pos = str.find(L" PKR - ");
        if (pos == std::wstring::npos)
        {
            throw std::invalid_argument("Invalid string format");
        }

        std::wstring amountStr = str.substr(0, pos);

        try
        {
            return std::stoi(amountStr);
        }
        catch (const std::invalid_argument&)
        {
            throw std::invalid_argument("Invalid amount format");
        }
    }

    void MainWindow::OpenLink(hstring url) {
        if (MyWebView2() != nullptr) {
            MyWebView2().Source(Uri(url));
        }
    }


    void MainWindow::Sound_Toggled(IInspectable const& sender, RoutedEventArgs const& e)
    {
        if (Sound().IsOn()) {
            ElementSoundPlayer::State(ElementSoundPlayerState::On);
            ElementSoundPlayer::SpatialAudioMode(ElementSpatialAudioMode::On);
        }
        else
        {
            ElementSoundPlayer::State(ElementSoundPlayerState::Off);
            ElementSoundPlayer::SpatialAudioMode(ElementSpatialAudioMode::Off);
        }
    }

    //File Handling
    void to_json(json& j, const User& user) {
        j = json{
            {"username", Base64Encode(WStringToString(user.username))},
            {"password", Base64Encode(WStringToString(user.password))},
            {"name", Base64Encode(WStringToString(user.name))},
            {"age", Base64EncodeInt(user.age)},
            {"loggedIn", user.loggedIn}, 
            {"totalexpense", Base64EncodeInt(user.totalexpense)},
            {"expenseVector", json::array()},
            {"uncompletedTasks", json::array()},
            {"completedTasks", json::array()}
        };

        for (const auto& exp : user.expenseVector) {
            j["expenseVector"].push_back(Base64Encode(WStringToString(exp.c_str())));
        }

        for (const auto& task : user.uncompletedTasks) {
            j["uncompletedTasks"].push_back({
                Base64Encode(WStringToString(task.first.c_str())),
                Base64Encode(WStringToString(task.second.c_str()))
                });
        }

        for (const auto& task : user.completedTasks) {
            j["completedTasks"].push_back({
                Base64Encode(WStringToString(task.first.c_str())),
                Base64Encode(WStringToString(task.second.c_str()))
                });
        }
    }



    void from_json(const json& j, User& user) {
        user.username = StringToWString(Base64Decode(j["username"].get<std::string>()));
        user.password = StringToWString(Base64Decode(j["password"].get<std::string>()));
        user.name = StringToWString(Base64Decode(j["name"].get<std::string>()));
        user.age = Base64DecodeInt(j["age"].get<std::string>());
        user.loggedIn = j.value("loggedIn", false); 
        user.totalexpense = Base64DecodeInt(j["totalexpense"].get<std::string>());

        for (const auto& expense : j.value("expenseVector", json::array())) {
            user.expenseVector.push_back(to_hstring(Base64Decode(expense.get<std::string>())));
        }

        for (const auto& task : j.value("uncompletedTasks", json::array())) {
            user.uncompletedTasks.emplace_back(
                to_hstring(Base64Decode(task[0].get<std::string>())),
                to_hstring(Base64Decode(task[1].get<std::string>()))
            );
        }

        for (const auto& task : j.value("completedTasks", json::array())) {
            user.completedTasks.emplace_back(
                to_hstring(Base64Decode(task[0].get<std::string>())),
                to_hstring(Base64Decode(task[1].get<std::string>()))
            );
        }
    }


    void SaveUsers() {
        OutputDebugString(L"\n===== Saving Users =====\n");

        for (auto& user : users) {
            if (user.username == LoggedInUsername) {
                user.loggedIn = LoggedIn;
                user.totalexpense = totalexpense;
                user.expenseVector = ExpenseVector;
                user.uncompletedTasks = UncompletedTasks;
                user.completedTasks = CompletedTasks;

                std::wstring debugInfo = L" Saving User Data: \n";
                debugInfo += L"Username: " + user.username + L"\n";
                debugInfo += L"Name: " + user.name + L"\n";
                debugInfo += L"Age: " + std::to_wstring(user.age) + L"\n";
                debugInfo += L"Total Expense: " + std::to_wstring(user.totalexpense) + L"\n";
                debugInfo += L"Uncompleted Tasks Count: " + std::to_wstring(user.uncompletedTasks.size()) + L"\n";
                debugInfo += L"Completed Tasks Count: " + std::to_wstring(user.completedTasks.size()) + L"\n";

                OutputDebugString(debugInfo.c_str());
                break;
            }
        }

        std::ofstream file("C:\\Users\\rocky\\Documents\\users.json");
        if (!file) {
            OutputDebugString(L" Error: Unable to open users.json for writing!\n");
            return;
        }

        try {
            json j = users;
            file << j.dump(4);
            OutputDebugString(L" Users saved successfully!\n");
        }
        catch (const std::exception& e) {
            std::wstring errorMsg = L" JSON Write Error: " + std::wstring(e.what(), e.what() + strlen(e.what())) + L"\n";
            OutputDebugString(errorMsg.c_str());
        }
    }

    void LoadUsers() {
        OutputDebugString(L"\n===== Loading Users =====\n");

        std::ifstream file("C:\\Users\\rocky\\Documents\\users.json");
        if (!file) {
            OutputDebugString(L" Error: users.json not found or unable to open!\n");
            return;
        }

        json j;
        try {
            file >> j;
            OutputDebugString(L" JSON file loaded successfully!\n");
        }
        catch (const std::exception& e) {
            OutputDebugString(L" JSON Read Error! File might be corrupted.\n");
            return;
        }

        try {
            users = j.get<std::vector<User>>();
            OutputDebugString(L" Users parsed successfully from JSON!\n");
        }
        catch (const std::exception& e) {
            OutputDebugString(L" User Parse Error!\n");
            return;
        }

        bool foundLoggedInUser = false;
        for (auto& user : users) {
                LoggedIn = true;
                LoggedInUsername = user.username;
                LoggedInName = user.name;
                LoggedInAge = user.age;
                totalexpense = user.totalexpense;
                ExpenseVector = user.expenseVector;
                UncompletedTasks = user.uncompletedTasks;
                CompletedTasks = user.completedTasks;

                std::wstring debugInfo = L" Loaded User Data: \n";
                debugInfo += L"Username: " + user.username + L"\n";
                debugInfo += L"Name: " + user.name + L"\n";
                debugInfo += L"Age: " + std::to_wstring(user.age) + L"\n";
                debugInfo += L"Total Expense: " + std::to_wstring(user.totalexpense) + L"\n";
                debugInfo += L"Uncompleted Tasks Count: " + std::to_wstring(user.uncompletedTasks.size()) + L"\n";
                debugInfo += L"Completed Tasks Count: " + std::to_wstring(user.completedTasks.size()) + L"\n";

                OutputDebugString(debugInfo.c_str());
                foundLoggedInUser = true;
                break;
        }

        if (!foundLoggedInUser) {
            OutputDebugString(L"⚠ No logged-in user found in JSON!\n");
        }
    }



    // Visibility Functions
    void MainWindow::Auth_Menu_SelectionChanged(IInspectable const&, Controls::NavigationViewSelectionChangedEventArgs const& args)
    {
        auto selectedItem = args.SelectedItem().try_as<Controls::NavigationViewItem>();
        if (selectedItem)
        {
            auto tag = unbox_value_or<hstring>(selectedItem.Tag(), L"");
            SignIn().Visibility(Visibility::Collapsed);
            Registration().Visibility(Visibility::Collapsed);
            ElementSoundPlayer::Play(ElementSoundKind::Show);
            if (tag == L"SignIn")
            {
                ErrorForAuth().IsOpen(false);
                SignInHeading().Margin(Thickness{ 0, 55, 107, 0 });
                SignIn().Visibility(Visibility::Visible);
                AuthMenuBackground().Height(380);
            }
            else if (tag == L"SignUp")
            {
                ErrorForAuth().IsOpen(false);
                RegistrationHeading().Margin(Thickness{ 0, 57 ,105,0 });
                Registration().Visibility(Visibility::Visible);
                AuthMenuBackground().Height(425);
            }
        };
    }


    void MainWindow::MainMenu_SelectionChanged(IInspectable const&, Controls::NavigationViewSelectionChangedEventArgs const& args)
    {
        UpdateGreeting();
        auto selectedItem = args.SelectedItem().try_as<Controls::NavigationViewItem>();
        if (selectedItem)
        {
            auto tag = unbox_value_or<hstring>(selectedItem.Tag(), L"");
            // Hide all pages first
            Home().Visibility(Visibility::Collapsed);
            LogOut().Visibility(Visibility::Collapsed);
            Browser().Visibility(Visibility::Collapsed);
            Chatbot().Visibility(Visibility::Collapsed);
            ExpenseCalculator().Visibility(Visibility::Collapsed);
            ToDoList().Visibility(Visibility::Collapsed);

            ElementSoundPlayer::Play(ElementSoundKind::Show);

            if (tag == L"HomePage")
            {
                Home().Visibility(Visibility::Visible);

            }
            else if (tag == L"AuthSettingsPage")
            {
                LogOut().Visibility(Visibility::Visible);
            }
            else if (tag == L"BrowserPage")
            {
                Browser().Visibility(Visibility::Visible);
            }
            else if (tag == L"ChatBotPage")
            {
                Chatbot().Visibility(Visibility::Visible);
            }
            else if (tag == L"ExpenseCalculatorPage")
            {
                ExpenseCalculator().Visibility(Visibility::Visible);
            }
            else if (tag == L"ToDoListPage")
            {
                ToDoList().Visibility(Visibility::Visible);
            }
        }
    }


    //Auth Functions
    void MainWindow::SignInButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        std::wstring enteredUsername = SignInUsername().Text().c_str();
        std::wstring enteredPassword = SignInPassword().Password().c_str();

        ErrorForAuth().IsOpen(false);

        if (enteredUsername.empty() || enteredPassword.empty())
        {
            ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"Please Fill All The Fields!");
            SignInHeading().Margin(Thickness{ 0, 4, 107, 0 });
            return;
        }

        // Debug 
        OutputDebugString((L"Entered Username: " + enteredUsername + L"\n").c_str());
        OutputDebugString((L"Entered Password: " + enteredPassword + L"\n").c_str());


        bool userFound = false;
        for (const auto& user : users)
        {
            // Debug
            OutputDebugString((L"Checking User: " + user.username + L" | Password: " + user.password + L"\n").c_str());

            if (user.username == enteredUsername)
            {
                userFound = true;
                if (user.password == enteredPassword)
                {
                    LoggedInUsername = enteredUsername;
                    UpdateGreeting();
                    ProgressRingBackground().Visibility(Visibility::Visible);
                    Auth().Visibility(Visibility::Collapsed);
                    StartProgressRingAnimation(ProgressRingAuth(), 20, [this]() {

                        // Successful Login
                        ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Success, L"Login Success!");
                        MainMenu().Visibility(Visibility::Visible);
                        LoggedIn - true;

                        if (!RememberMe().IsOn()) {
                            SignInUsername().Text(L"");
                            SignInPassword().Password(L"");
                        }

                        MenuBG().Visibility(Visibility::Visible);
                        ProgressRingBackground().Visibility(Visibility::Collapsed);
                        RefreshTaskList();
                        RefreshExpenses();
                        return;
                        });

                }

                else
                {
                    // Incorrect Password
                    ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"Invalid Password!");
                    SignInHeading().Margin(Thickness{ 0, 4, 107, 0 });
                    return;
                }
            }
        }

        // User not found
        if (!userFound)
        {
            ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"No User Found!");
            SignInHeading().Margin(Thickness{ 0, 4, 107, 0 });
        }
    }

    void MainWindow::RegistrationButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        std::wstring username = RegistrationUsername().Text().c_str();
        std::wstring password = RegistrationPassword().Password().c_str();
        std::wstring fullname = RegistrationName().Text().c_str();
        int age = 25;

        ErrorForAuth().IsOpen(false);

        //Already Exists Error
        for (const auto& user : users)
        {
            if (user.username == username)
            {

                RegistrationHeading().Margin(Thickness{ 0, 6 ,105,0 });
                ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"Username Already Exists!");
                return;
            }
        }

        //Empty Error
        if (username.empty() || password.empty() || fullname.empty() || age <= 0)
        {

            RegistrationHeading().Margin(Thickness{ 0, 6 ,105,0 });
            ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"All fields must be filled correctly!");
            return;

        }
        else if (password.length() < 7)
        {

            RegistrationHeading().Margin(Thickness{ 0, 6 ,105,0 });
            ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Error, L"Password must be at least 7 characters!");
            return;
        }


        // Saving The User
        User newUser = { username, password, fullname, age };
        users.push_back(newUser);
        SaveUsers();

        RegistrationUsername().Text(L"");
        RegistrationName().Text(L"");
        RegistrationPassword().Password(L"");

        // Show success message
        RegistrationHeading().Margin(Thickness{ 0, 6 ,105,0 });
        ShowAuthError(ErrorForAuth(), Controls::InfoBarSeverity::Success, L"Registration Successful!");
    }



    void MainWindow::LogOutButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {

        LoggedIn = false;
        SaveUsers();
        LoggedInUsername = L"";
        MainMenu().Visibility(Visibility::Collapsed);
        ErrorForAuth().IsOpen(false);
        ProgressRingBackground().Visibility(Visibility::Visible);
        MyWebView2().Source(Uri(L"https://www.google.com"));

        auto flyout = LogOutButton().Flyout().as<Controls::Flyout>();
        if (flyout)
        {
            flyout.Hide();
        }

        StartProgressRingAnimation(ProgressRingAuth(), 20, [this]() {
            MenuBG().Visibility(Visibility::Collapsed);
            Auth().Visibility(Visibility::Visible);
            ProgressRingBackground().Visibility(Visibility::Collapsed);
            });
    }



    // Created For Both Assistant And UI
    void MainWindow::AddExpense(double amount, hstring reason) {
        int expenseamount = amount;
        hstring NewExpense = std::to_wstring(expenseamount) + L" PKR - " + reason;

        ExpenseVector.insert(ExpenseVector.begin(), NewExpense);

        // Update total expense
        totalexpense += expenseamount;
        RefreshExpenses();
        SaveUsers();
    }

    //Expense Calculator Functions
    void MainWindow::AddExpense_Click(IInspectable const&, RoutedEventArgs const&)
    {
        int expenseamount = ExpenseAmount().Value();

        if (expenseamount <= 0 || std::isnan(expenseamount) || ExpenseReason().Text().empty()) {
            ShowErrorMessage(L"Please Fill All The Fields Valid!");
            return;
        }

        AddExpense(ExpenseAmount().Value(), ExpenseReason().Text());
        ExpenseAmount().Value(0);
        ExpenseReason().Text(L"");
    }

    void MainWindow::DeleteExpense_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Controls::Button>();
        if (!button) return;

        auto itemTag = button.Tag().try_as<hstring>();
        if (!itemTag || itemTag->empty()) return;

        auto it = std::find(ExpenseVector.begin(), ExpenseVector.end(), *itemTag);
        if (it != ExpenseVector.end())
        {
            // Update total expense
            int amount = ExtractAmountFromString(*it);
            totalexpense -= amount;
            ExpenseVector.erase(it);
            RefreshExpenses();
        }
    }

    // Created For Both Assistant And UI
    void MainWindow::CreateTask(hstring taskName, hstring taskDesc) {
        hstring Task_name = taskName;
        hstring Task_Description = taskDesc;

        UncompletedTasks.emplace_back(Task_name, Task_Description);
        RefreshTaskList();
        SaveUsers();
    }

    void MainWindow::AddTaskButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        hstring Task_name = TaskName().Text();
        hstring Task_Description = TaskDescription().Text();
        if (Task_name.empty() || Task_Description.empty()) {
            ShowErrorMessage(L"Please Fill All The Fields Valid!");
            return;
        }
        CreateTask(Task_name, Task_Description);
        TaskDescription().Text(L"");
        TaskName().Text(L"");
    }


    // Complete Button Click Handler
    void MainWindow::CompleteTask_Click(
        IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        auto button = sender.try_as<Microsoft::UI::Xaml::Controls::AppBarButton>();
        if (button)
        {
            auto taskTag = button.Tag().try_as<hstring>();
            if (taskTag)
            {
                for (auto it = UncompletedTasks.begin(); it != UncompletedTasks.end(); ++it)
                {
                    if (it->first == *taskTag)
                    {
                        CompletedTasks.push_back(*it);
                        UncompletedTasks.erase(it);
                        break;
                    }
                }
                RefreshTaskList();
            }
        }
    }


    void MainWindow::DeleteCompletedTask_Click(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.try_as<Controls::Button>();
        if (!button) return;

        auto itemTag = button.Tag().try_as<hstring>();
        if (!itemTag || itemTag->empty()) return;

        auto it = std::find_if(CompletedTasks.begin(), CompletedTasks.end(),
            [&itemTag](const std::pair<hstring, hstring>& task)
            {
                return task.first == *itemTag;
            });

        if (it != CompletedTasks.end())
        {
            CompletedTasks.erase(it); 
            RefreshTaskList();
        }
    }


    fire_and_forget MainWindow::SendMessageToGemini(hstring userMessage) {
        auto lifetime = get_strong();
        try {
            HttpClient httpClient;
            HttpRequestMessage request(
                HttpMethod::Post(),
                Uri(L"https://generativelanguage.googleapis.com/v1beta/tunedModels/YOUR_MODEL:generateContent?key=YOUR_API_KEY")
            );

            std::wstring jsonBody = LR"({"contents":[{"parts":[{"text": ")" +
                std::wstring(userMessage.c_str()) + LR"("}]}]})";

            HttpStringContent jsonContent(
                hstring(jsonBody),
                Windows::Storage::Streams::UnicodeEncoding::Utf8,
                L"application/json"
            );
            request.Content(jsonContent);

            HttpResponseMessage response = co_await httpClient.SendRequestAsync(request);

            if (response.StatusCode() == HttpStatusCode::Ok) {
                hstring responseText = co_await response.Content().ReadAsStringAsync();
                OutputDebugStringW((L"Raw API Response: " + std::wstring(responseText.c_str())).c_str());

                Windows::Data::Json::JsonObject jsonResponse;
                if (Windows::Data::Json::JsonObject::TryParse(responseText, jsonResponse)) {
                    if (jsonResponse.HasKey(L"candidates")) {
                        auto candidatesArray = jsonResponse.GetNamedArray(L"candidates");
                        if (candidatesArray.Size() > 0) {
                            auto firstCandidate = candidatesArray.GetAt(0).GetObject();
                            if (firstCandidate.HasKey(L"content")) {
                                auto contentObject = firstCandidate.GetNamedObject(L"content");
                                if (contentObject.HasKey(L"parts")) {
                                    auto partsArray = contentObject.GetNamedArray(L"parts");
                                    if (partsArray.Size() > 0) {
                                        auto textResponse = partsArray.GetAt(0).GetObject().GetNamedString(L"text");

                                        std::wstring textStd = textResponse.c_str();

                                        // **Extract text before JSON if any**
                                        size_t jsonStart = textStd.find(L"{");
                                        size_t jsonEnd = textStd.rfind(L"}");

                                        std::wstring preJsonText;
                                        std::wstring validJson;
                                        preJsonText = textStd.substr(0, jsonStart);

                                        validJson = textStd.substr(jsonStart, jsonEnd - jsonStart + 1);

                                        // **Sanitize JSON by removing newlines and carriage returns**
                                        std::wstring sanitizedJson;
                                        for (wchar_t c : validJson) {
                                            if (c != L'\n' && c != L'\r') {
                                                sanitizedJson += c;
                                            }
                                        }

                                        std::wstring sanitizedpreJsonText;
                                        for (wchar_t c : preJsonText) {
                                            if (c != L'```') {
                                                sanitizedpreJsonText += c;
                                            }
                                            else {
                                                break;
                                            }
                                        }
                                        // **Debug Output: Print extracted pre-JSON text**
                                        OutputDebugStringW((L"Pre-JSON Text: " + preJsonText).c_str());

                                        // **Debug Output: Print sanitized JSON**
                                        OutputDebugStringW((L"Sanitized Extracted JSON: " + sanitizedJson).c_str());
                                        if (jsonStart != std::wstring::npos && jsonEnd != std::wstring::npos) {

                                            Windows::Data::Json::JsonObject parsedTextJson;
                                            if (Windows::Data::Json::JsonObject::TryParse(hstring(sanitizedJson), parsedTextJson)) {
                                                hstring contentText = parsedTextJson.GetNamedString(L"content", L"");
                                                hstring speakText = parsedTextJson.GetNamedString(L"speak", L"");

                                                // Store the pre-JSON text if needed for later use
                                                ChatbotResponseText().Text(contentText);

                                                // **Process Actions**
                                                if (parsedTextJson.HasKey(L"actionType")) {
                                                    hstring actionType = parsedTextJson.GetNamedString(L"actionType");
                                                    OutputDebugStringW((L"Action Type: " + actionType).c_str());

                                                    if (parsedTextJson.HasKey(L"parameters")) {
                                                        Windows::Data::Json::JsonObject parameters = parsedTextJson.GetNamedObject(L"parameters");

                                                        if (actionType == L"createTask") {
                                                            hstring taskName = parameters.GetNamedString(L"taskName", L"");
                                                            hstring taskDesc = parameters.GetNamedString(L"taskDesc", L"");
                                                            OutputDebugStringW((L"Task Name: " + taskName + L", Task Desc: " + taskDesc).c_str());

                                                            ChatbotResponseText().Text(L"Task Created: " + taskName + L" - " + taskDesc);
                                                            CreateTask(taskName, taskDesc);
                                                            auto ManuItem = MainMenu().MenuItems().GetAt(2).try_as<Controls::NavigationViewItem>();
                                                            MainMenu().SelectedItem(ManuItem);
                                                        }
                                                        else if (actionType == L"addExpense") {
                                                            double expenseAmount = parameters.GetNamedNumber(L"expenseAmount", 0);
                                                            hstring expenseReason = parameters.GetNamedString(L"expenseReason", L"");

                                                            if (!expenseReason.empty()) {
                                                                OutputDebugStringW((L"Expense Amount: $" + to_hstring(expenseAmount) + L", Reason: " + expenseReason).c_str());

                                                                ChatbotResponseText().Text(L"Expense Added: $" + to_hstring(expenseAmount) + L" for " + expenseReason);
                                                                AddExpense(expenseAmount, expenseReason);

                                                            } else {

                                                                hstring expenseCategory = parameters.GetNamedString(L"expenseCategory", L"");
                                                                OutputDebugStringW((L"Expense Amount: $" + to_hstring(expenseAmount) + L", Reason: " + expenseCategory).c_str());
                                                                ChatbotResponseText().Text(L"Expense Added: $" + to_hstring(expenseAmount) + L" for " + expenseCategory);
                                                                AddExpense(expenseAmount, expenseCategory);

                                                            }
                                                            auto ManuItem = MainMenu().MenuItems().GetAt(1).try_as<Controls::NavigationViewItem>();
                                                            MainMenu().SelectedItem(ManuItem);
                                                        }
                                                        else if (actionType == L"openLink") {
                                                            hstring url = parameters.GetNamedString(L"url", L"");
                                                            OutputDebugStringW((L"Opening Link: " + url).c_str());

                                                            ChatbotResponseText().Text(L"Opening: " + url);
                                                            OpenLink(url);
                                                            auto ManuItem = MainMenu().MenuItems().GetAt(3).try_as<Controls::NavigationViewItem>();
                                                            MainMenu().SelectedItem(ManuItem);
                                                        }
                                                    }
                                                    else {
                                                        OutputDebugStringW(L"Error: 'parameters' key missing in response.");
                                                    }
                                                }
                                                co_return;
                                            }
                                            else {
                                                OutputDebugStringW(L"Error: Extracted JSON is invalid.");
                                            }
                                        }
                                        else {
                                            // **Handle plain text response**
                                            OutputDebugStringW(L"Plain text response detected.");
                                            ChatbotResponseText().Text(sanitizedpreJsonText);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else {
                    // **If JSON parsing fails, assume it's plain text**
                    OutputDebugStringW(L"Response is not JSON. Assuming plain text.");
                    ChatbotResponseText().Text(responseText);
                }
            }
            else {
                ChatbotResponseText().Text(L"Error: HTTP Request Failed!");
            }
        }
        catch (const hresult_error& ex) {
            ChatbotResponseText().Text(L"Critical Error: " + ex.message());
        }
        catch (const std::exception& ex) {
            ChatbotResponseText().Text(L"Exception: " + to_hstring(ex.what()));
        }
    }




    void MainWindow::SendButton_Click(const IInspectable&, const Microsoft::UI::Xaml::RoutedEventArgs&) {
        hstring userInput = UserInputTextBox().Text();
        if (userInput.empty()) {
            ChatbotResponseText().Text(L"Please enter a message.");
            return;
        }
        SendMessageToGemini(userInput);
    }

    //on close
    void MainWindow::OnCloseRequested()
    {

        if (MyWebView2() != nullptr)
        {
            auto coreWebView2 = MyWebView2();
            if (coreWebView2)
            {
                coreWebView2.Close();
            }
            MyWebView2(nullptr);
        }
        SaveUsers();

    }

    // On start
    MainWindow::MainWindow()
    {
        InitializeComponent();

        // Disable the title bar
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(nullptr);

        // Sound
        if (Sound().IsOn()) {
            ElementSoundPlayer::State(ElementSoundPlayerState::On);
            ElementSoundPlayer::SpatialAudioMode(ElementSpatialAudioMode::On);

        }

        // Set default navigation to "Sign Up"
        auto AuthItem = AuthMenu().MenuItems().GetAt(0).try_as<Controls::NavigationViewItem>();
        auto ManuItem = MainMenu().MenuItems().GetAt(0).try_as<Controls::NavigationViewItem>();

        AuthMenu().SelectedItem(AuthItem);
        MainMenu().SelectedItem(ManuItem);


        // Load Users
        LoadUsers();
        this->Closed([this](auto&&, auto&&) { OnCloseRequested(); });


    }

}
