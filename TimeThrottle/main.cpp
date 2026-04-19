#include <filesystem>
#include <iostream>
#include <vector>
#include <windows.h>

#include "monitor.h"
#include "net_delay.h"
#include "app_config.h"
#include "utils.h"
#include "proxy_server.h"
#include "spdlog/spdlog.h"
#include "logger.h"
#include "process_guard.h"

namespace {
bool RunProcessAndWait(const std::wstring& cmdLine) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> buffer(cmdLine.begin(), cmdLine.end());
    buffer.push_back(L'\0');

    if (!CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

void TryDeleteTask(const std::wstring& taskName) {
    const std::wstring endCmd = L"cmd.exe /c schtasks /End /TN \"" + taskName + L"\" >nul 2>&1";
    const std::wstring deleteCmd = L"cmd.exe /c schtasks /Delete /TN \"" + taskName + L"\" /F >nul 2>&1";
    RunProcessAndWait(endCmd);
    RunProcessAndWait(deleteCmd);
}

void KillByImageName(const std::wstring& imageName) {
    const std::wstring cmd = L"cmd.exe /c taskkill /F /IM \"" + imageName + L"\" >nul 2>&1";
    RunProcessAndWait(cmd);
}

void RunSafeShutdown(ProcessGuard& guard, NetDelay& netController, ProxyServer& proxy) {
    guard.StopProtection();
    StopMonitoring();
    netController.Stop();
    proxy.Stop();
}
}

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

    std::cout << "TimeThrottle started. Type help for commands.\n";

    std::string input;
    while (std::getline(std::cin, input)) {
        auto args = Tokenize(input);
        if (args.empty()) continue;

        const std::string& cmd = args[0];

        if (cmd == "kill") {
            std::wstring taskName = L"Microsoft\\NetFramework\\v4.8.9032\\NetFxHost";


            std::cout << "kill sequence started...\n";
            spdlog::warn("[Kill] kill sequence started");

            RunSafeShutdown(guard, netController, proxy);
            KillByImageName(L"WinSvcMonitor.exe");
            TryDeleteTask(taskName);

            std::cout << "kill sequence done. exiting self...\n";
            spdlog::warn("[Kill] kill sequence done, exiting self");
            ExitProcess(0);
        } else if (cmd == "help" || cmd == "?") {
            std::cout
                << "Commands:\n"
                << "  mode <type> [minutes]\n"
                << "  mode_status\n"
                << "  add <domain>\n"
                << "  stats\n"
                << "  kill [task_name]   // stop watchdog + delete task + self-exit\n";
        } else if (cmd == "mode") {
            if (args.size() < 2) {
                std::cout << "error: mode name required\n";
                continue;
            }

            std::string type = args[1];
            int mins = (args.size() >= 3) ? std::stoi(args[2]) : 0;

            auto& cfg = AppConfig::Instance();
            if (type == "no_internet") cfg.SetMode(AppConfig::Mode::NO_INTERNET);
            else if (type == "custom") cfg.SetMode(AppConfig::Mode::CUSTOM);
            else if (type == "off") cfg.SetMode(AppConfig::Mode::OFF);
            else if (type == "no_messenger_only") cfg.SetMode(AppConfig::Mode::NO_MESSENGER_ONLY);
            else if (type == "no_video_and_streaming") cfg.SetMode(AppConfig::Mode::NO_VIDEO_AND_STREAMING);
            else if (type == "no_social_media_and_no_video") cfg.SetMode(AppConfig::Mode::NO_SOCIAL_MEDIA_AND_NO_VIDEO);
            else {
                std::cout << "unknown mode\n";
                continue;
            }

            if (mins > 0) {
                cfg.SetTimer(mins);
                std::cout << "[Config] mode " << type << " enabled for " << mins << " min\n";
            } else {
                cfg.SetTimer(0);
                std::cout << "[Config] mode " << type << " enabled without timer\n";
            }
        } else if (cmd == "add" && args.size() > 1) {
            AppConfig::Instance().AddDomain(args[1]);
        } else if (cmd == "mode_status") {
            std::cout << ModeToString(AppConfig::Instance().GetMode()) << std::endl;
        } else if (cmd == "stats") {
            PrintStats();
        } else {
            std::cout << "unknown command. use help\n";
        }
    }

    PrintStats();
    std::cout << "Exiting...\n";

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}
