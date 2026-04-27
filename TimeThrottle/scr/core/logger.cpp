#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <filesystem>
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <string>

std::string GetDesktopPath() {
    PWSTR path = nullptr;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &path))) {
        std::wstring ws(path);
        CoTaskMemFree(path);

        return std::string(ws.begin(), ws.end());
    }

    return "";
}
void InitLogging() {
    try {
        std::string logDir = "D:/projects/C++/TimeThrottle/logs";
        std::filesystem::create_directories(logDir);
        std::string logPath = logDir + "/debug_history.log";

        auto file_logger = spdlog::rotating_logger_mt("file_logger", logPath, 1024 * 1024 * 5, 3);

        file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        spdlog::set_default_logger(file_logger);

        spdlog::set_level(spdlog::level::trace);

        spdlog::flush_on(spdlog::level::err);
        spdlog::flush_every(std::chrono::seconds(3));

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}
/*void InitLogging() {
    try {
        std::string desktop = GetDesktopPath();
        std::string logDir = desktop + "\\logs";

        std::filesystem::create_directories(logDir);
        std::string logPath = logDir + "\\debug_history.log";

        auto file_logger = spdlog::rotating_logger_mt(
            "file_logger",
            logPath,
            1024 * 1024 * 5,
            3
        );

        file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::set_default_logger(file_logger);
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::err);
        spdlog::flush_every(std::chrono::seconds(3));
        std::cerr << "сука:" << std::endl;


    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}*/