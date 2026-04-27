#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <windows.h>

#include "monitor/monitor.h"
#include "network/net_delay.h"
#include "network/proxy_server.h"
#include "core/logger.h"
#include "core/console_commands.h"
#include "core/control_pipe_server.h"
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
    std::mutex commandsMutex;

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

    const auto executeCommand = [&](const std::string& command, bool captureOutput) -> ControlCommandResult {
        std::lock_guard<std::mutex> lock(commandsMutex);

        ControlCommandResult out{};
        if (captureOutput) {
            std::ostringstream buffer;
            auto* oldBuf = std::cout.rdbuf(buffer.rdbuf());
            const auto result = commands.HandleLine(command);
            std::cout.rdbuf(oldBuf);
            out.exitRequested = (result == ConsoleCommands::Result::ExitRequested);
            out.response = buffer.str();
            return out;
        }

        const auto result = commands.HandleLine(command);
        out.exitRequested = (result == ConsoleCommands::Result::ExitRequested);
        return out;
    };

    ControlPipeServer controlPipe([&](const std::string& command) -> ControlCommandResult {
        auto result = executeCommand(command, true);
        if (result.exitRequested) {
            shutdownAll();
            result.response += "core exiting";
            std::thread([] {
                Sleep(120);
                ExitProcess(0);
            }).detach();
        }
        return result;
    });
    controlPipe.Start();

    std::string input;
    while (std::getline(std::cin, input)) {
        const auto commandResult = executeCommand(input, false);
        if (commandResult.exitRequested) {
            shutdownAll();
            break;
        }
    }

    controlPipe.Stop();
    shutdownAll();
    PrintStats();
    std::cout << "Exiting...\n";

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}
