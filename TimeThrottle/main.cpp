#include <filesystem>
#include <iostream>
#include <windows.h>

#include "monitor/monitor.h"
#include "network/net_delay.h"
#include "network/proxy_server.h"
#include "core/logger.h"
#include "core/console_commands.h"
#include "core/app_config.h"
#include "security/process_guard.h"

int main(int argc, char* argv[]) {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\TimeThrottle_Unique_Mutex");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    InitLogging();

    SetConsoleTitleW(L"TimeThrottle");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    AppConfig::Instance().LoadFromFile();

    NetDelay netController;
    StartMonitoring();
    netController.Start();

    ProcessGuard guard;
    std::filesystem::path exeDir = std::filesystem::canonical(argv[0]).parent_path();
    std::wstring watchdogPath = (exeDir / "WinSvcMonitor.exe").wstring();
    guard.StartProtection(watchdogPath);

    ProxyServer proxy(&netController, 8899);
    proxy.Start();
    ConsoleCommands commands(guard, netController, proxy);

    std::cout << "TimeThrottle started. Type help for commands.\n";

    bool stopped = false;
    const auto shutdownAll = [&]() {
        if (stopped) return;
        guard.StopProtection();
        StopMonitoring();
        netController.Stop();
        proxy.Stop();
        stopped = true;
    };

    std::string input;
    while (std::getline(std::cin, input)) {
        auto commandResult = commands.HandleLine(input);
        if (commandResult == ConsoleCommands::Result::ExitRequested) {
            shutdownAll();
            break;
        }
    }

    shutdownAll();
    PrintStats();
    std::cout << "Exiting...\n";

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}
