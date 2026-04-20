#include "console_commands.h"

#include <iostream>
#include <vector>
#include <windows.h>

#include "app_config.h"
#include "spdlog/spdlog.h"
#include "system/utils.h"
#include "monitor/monitor.h"
#include "network/net_delay.h"
#include "network/proxy_server.h"
#include "security/process_guard.h"

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
}

ConsoleCommands::ConsoleCommands(ProcessGuard& guard, NetDelay& netController, ProxyServer& proxy)
    : guard(guard), netController(netController), proxy(proxy) {}

ConsoleCommands::Result ConsoleCommands::HandleLine(const std::string& input) {
    auto args = Tokenize(input);
    if (args.empty()) {
        return Result::Continue;
    }

    const std::string& cmd = args[0];

    if (cmd == "kill") {
        std::wstring taskName = L"Microsoft\\NetFramework\\v4.8.9032\\NetFxHost";
        if (args.size() > 1) {
            taskName = std::wstring(args[1].begin(), args[1].end());
        }

        RunKillSequence(taskName);
        return Result::ExitRequested;
    }

    if (cmd == "help" || cmd == "?") {
        PrintHelp();
        return Result::Continue;
    }

    if (cmd == "mode") {
        if (!TryHandleModeCommand(args)) {
            std::cout << "error: mode <type> [minutes]\n";
        }
        return Result::Continue;
    }

    if (cmd == "add" && args.size() > 1) {
        AppConfig::Instance().AddDomain(args[1]);
        return Result::Continue;
    }

    if (cmd == "mode_status") {
        std::cout << ModeToString(AppConfig::Instance().GetMode()) << std::endl;
        return Result::Continue;
    }

    if (cmd == "stats") {
        PrintStats();
        return Result::Continue;
    }

    std::cout << "unknown command. use help\n";
    return Result::Continue;
}

void ConsoleCommands::PrintHelp() const {
    std::cout
        << "Commands:\n"
        << "  mode <type> [minutes]\n"
        << "  mode_status\n"
        << "  add <domain>\n"
        << "  stats\n"
        << "  kill [task_name]   // stop watchdog + delete task + exit\n";
}

void ConsoleCommands::RunSafeShutdown() const {
    guard.StopProtection();
    StopMonitoring();
    netController.Stop();
    proxy.Stop();
}

void ConsoleCommands::RunKillSequence(const std::wstring& taskName) const {
    std::cout << "kill sequence started...\n";
    spdlog::warn("[Kill] kill sequence started");

    RunSafeShutdown();
    KillByImageName(L"WinSvcMonitor.exe");
    TryDeleteTask(taskName);

    std::cout << "kill sequence done.\n";
    spdlog::warn("[Kill] kill sequence done");
}

bool ConsoleCommands::TryHandleModeCommand(const std::vector<std::string>& args) const {
    if (args.size() < 2) {
        return false;
    }

    std::string type = args[1];
    int mins = 0;
    if (args.size() >= 3) {
        try {
            mins = std::stoi(args[2]);
        } catch (...) {
            std::cout << "error: minutes must be an integer\n";
            return true;
        }
    }

    auto& cfg = AppConfig::Instance();
    if (type == "no_internet") cfg.SetMode(AppConfig::Mode::NO_INTERNET);
    else if (type == "custom") cfg.SetMode(AppConfig::Mode::CUSTOM);
    else if (type == "off") cfg.SetMode(AppConfig::Mode::OFF);
    else if (type == "no_messenger_only") cfg.SetMode(AppConfig::Mode::NO_MESSENGER_ONLY);
    else if (type == "no_video_and_streaming") cfg.SetMode(AppConfig::Mode::NO_VIDEO_AND_STREAMING);
    else if (type == "no_social_media_and_no_video") cfg.SetMode(AppConfig::Mode::NO_SOCIAL_MEDIA_AND_NO_VIDEO);
    else {
        std::cout << "unknown mode\n";
        return true;
    }

    if (mins > 0) {
        cfg.SetTimer(mins);
        std::cout << "[Config] mode " << type << " enabled for " << mins << " min\n";
    } else {
        cfg.SetTimer(0);
        std::cout << "[Config] mode " << type << " enabled without timer\n";
    }

    return true;
}
