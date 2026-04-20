#include "monitor.h"
#include "system/utils.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

namespace {
std::map<std::string, std::chrono::seconds> time_spent;
std::map<std::string, int> app_focus_count;
std::map<std::string, std::chrono::seconds> site_time_spent;
std::map<std::string, std::chrono::seconds> site_focus_count;

std::chrono::steady_clock::time_point last_time;
HWND last_hwnd = nullptr;
std::string last_domain;

std::mutex stats_mutex;
std::mutex state_mutex;
std::atomic<bool> monitoring_running{false};
std::thread monitoring_thread;
std::thread pipe_thread;

constexpr int EPS_MILLISECONDS = 300;

void WakePipeServer() {
    HANDLE hPipe = CreateFileW(
        L"\\\\.\\pipe\\MyMonitorDomainPipe",
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipe != INVALID_HANDLE_VALUE) {
        const char wake[] = "{}";
        DWORD bytesWritten = 0;
        WriteFile(hPipe, wake, sizeof(wake) - 1, &bytesWritten, nullptr);
        CloseHandle(hPipe);
    }
}

void PipeServerThread() {
    while (monitoring_running.load()) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;

        HANDLE hPipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\MyMonitorDomainPipe",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            0,
            0,
            0,
            &sa
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            spdlog::error("[Monitor] Failed to create named pipe");
            continue;
        }

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            if (!monitoring_running.load()) {
                CloseHandle(hPipe);
                break;
            }

            char buffer[512];
            DWORD bytesRead = 0;
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';

                auto now = std::chrono::steady_clock::now();
                HWND currentHwnd = GetForegroundWindow();
                DWORD pid = 0;
                GetWindowThreadProcessId(currentHwnd, &pid);
                std::string appName = GetProcessNameUtf8(pid);

                {
                    std::lock_guard<std::mutex> stateLock(state_mutex);
                    if (appName == "chrome.exe" || appName == "msedge.exe") {
                        if (!last_domain.empty()) {
                            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
                            std::lock_guard<std::mutex> statsLock(stats_mutex);
                            site_time_spent[last_domain] += duration;
                            site_focus_count[last_domain]++;
                        }
                        last_time = now;
                    }

                    last_domain = ParseDomain(std::string(buffer));
                    spdlog::trace("[Pipe] New site focused: {}", last_domain);
                }
            }
        }

        CloseHandle(hPipe);
    }
}

void MonitoringThread() {
    {
        std::lock_guard<std::mutex> stateLock(state_mutex);
        last_time = std::chrono::steady_clock::now();
        last_hwnd = nullptr;
        last_domain.clear();
    }

    while (monitoring_running.load()) {
        HWND hwnd = GetForegroundWindow();

        std::lock_guard<std::mutex> stateLock(state_mutex);
        if (hwnd != nullptr && hwnd != last_hwnd) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);

            if (last_hwnd != nullptr) {
                DWORD pid = 0;
                GetWindowThreadProcessId(last_hwnd, &pid);
                std::string name = GetProcessNameUtf8(pid);

                std::lock_guard<std::mutex> statsLock(stats_mutex);
                time_spent[name] += duration;
                app_focus_count[name]++;

                if ((name == "chrome.exe" || name == "msedge.exe") && !last_domain.empty()) {
                    site_time_spent[last_domain] += duration;
                    site_focus_count[last_domain]++;
                    spdlog::trace("Site {} + {}", last_domain, duration.count());
                }

                spdlog::trace("{} time spent + {}", name, duration.count());
            }

            last_time = now;
            last_hwnd = hwnd;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(EPS_MILLISECONDS));
    }
}
} // namespace

void StartMonitoring() {
    bool expected = false;
    if (!monitoring_running.compare_exchange_strong(expected, true)) {
        return;
    }

    monitoring_thread = std::thread(MonitoringThread);
    pipe_thread = std::thread(PipeServerThread);
}

void StopMonitoring() {
    if (!monitoring_running.exchange(false)) {
        return;
    }

    WakePipeServer();

    if (monitoring_thread.joinable()) {
        monitoring_thread.join();
    }
    if (pipe_thread.joinable()) {
        pipe_thread.join();
    }
}

void PrintStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);

    std::cout << "\n=== Applications ===\n";
    for (const auto& [name, sec] : time_spent) {
        std::cout << name << ": " << sec.count() << "\n";
    }

    std::cout << "\n=== Websites ===\n";
    for (const auto& [domain, sec] : site_time_spent) {
        if (!domain.empty()) {
            std::cout << domain << ": " << sec.count() << "\n";
        }
    }
}
