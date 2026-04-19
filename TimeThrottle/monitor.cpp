#include "monitor.h"
#include "utils.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <thread>

std::map<std::string, std::chrono::seconds> time_spent;
std::map<std::string, int> app_focus_count;
std::map<std::string, std::chrono::seconds> site_time_spent;
std::map<std::string, std::chrono::seconds> site_focus_count;

std::chrono::steady_clock::time_point last_time;
HWND last_hwnd = nullptr;
std::string last_domain; // текущий домен из Pipe
bool monitoring_running = true;
int EPS_MILLISECONDS = 300;


void PipeServerThread() {
    while (monitoring_running) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        // разрешаем доступ всем пользователям
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;

        HANDLE hPipe = CreateNamedPipeW(
                L"\\\\.\\pipe\\MyMonitorDomainPipe",
                PIPE_ACCESS_INBOUND,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 0, 0, 0,
                &sa
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            spdlog::error("INVALID HANDLE VALUE");
            continue;
        }

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            char buffer[512];
            DWORD bytesRead;
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';

                // прежде чем сменить домен, начислим время старому (если браузер в фокусе)
                auto now = std::chrono::steady_clock::now();
                HWND currentHwnd = GetForegroundWindow();
                DWORD pid;
                GetWindowThreadProcessId(currentHwnd, &pid);
                std::string appName = GetProcessNameUtf8(pid);

                if (appName == "chrome.exe" || appName == "msedge.exe") {
                    if (!last_domain.empty()) {
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
                        site_time_spent[last_domain] += duration;
                        site_focus_count[last_domain]++;
                    }
                    last_time = now; // сбрасываем таймер для нового домена
                }

                last_domain = ParseDomain(std::string(buffer));
                spdlog::trace("[Pipe] New site focused: {}", last_domain);

            }
        }
        CloseHandle(hPipe);
    }
}

void MonitoringThread() {
    last_time = std::chrono::steady_clock::now();
    while(monitoring_running){
        HWND hwnd = GetForegroundWindow();

        if (hwnd != nullptr && hwnd != last_hwnd){
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);

            if (last_hwnd != nullptr) {
                DWORD pid;
                GetWindowThreadProcessId(last_hwnd, &pid);
                std::string name = GetProcessNameUtf8(pid);

                // считаем время приложения
                time_spent[name] += duration;
                app_focus_count[name]++;

                // считаем время сайта, если закрываемое окно было браузером
                if ((name == "chrome.exe" || name == "msedge.exe") && !last_domain.empty()) {
                    site_time_spent[last_domain] += duration;
                    site_focus_count[last_domain]++;
                    spdlog::trace("Site {} + {}",last_domain,  duration.count());

                }

                spdlog::trace("{} time spent + {}",name,  duration.count());

            }

            last_time = now;
            last_hwnd = hwnd;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(EPS_MILLISECONDS));
    }
}

void StartMonitoring() {
    last_time = std::chrono::steady_clock::now();
    std::thread(MonitoringThread).detach();
    std::thread(PipeServerThread).detach(); // запускаем оба потока
}
void StopMonitoring() {
    monitoring_running = false;
}
void PrintStats() {
    std::cout << "\n=== Applications ===\n";
    for (const auto& [name, sec] : time_spent) {
        std:: cout << name <<  ": " << sec.count() << "\n";

    }
    std::cout << "\n=== Websites ===\n";
    for (const auto& [domain, sec] : site_time_spent) {
        if (!domain.empty())
            std:: cout << domain <<  ": " << sec.count() << "\n";
    }
}