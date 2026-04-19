#include <filesystem>
#include <windows.h>
#include <string>
#include <iostream>
#include <thread>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "process_guard.h"

void initLogging() {
    try {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "D:/projects/C++/WatchDog/logs/watchDog.log", 1024 * 1024 * 2, 3
        );

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // создаем логгер, который пишет в оба места сразу
        std::vector<spdlog::sink_ptr> sinks {file_sink, console_sink};
        auto logger = std::make_shared<spdlog::logger>("watchdog", sinks.begin(), sinks.end());

        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // 1. Собственный мьютекс вочдога (чтобы не запустить два вочдога)
    HANDLE hSelfMutex = CreateMutexW(NULL, TRUE, L"Global\\WatchDog_Unique_Mutex");
    if (hSelfMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    initLogging();

    spdlog::info("--- Watchdog запущен (режим Mutex-мониторинга) ---");

    std::filesystem::path exeDir = std::filesystem::canonical(argv[0]).parent_path();
    std::wstring appPath = exeDir / "SecurityHealthService.exe";

    ProcessGuard guard;
    guard.StartProtection(appPath);
    std::cout << "Process Guard запущен\n";
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    WaitForSingleObject(hEvent, INFINITE);
}