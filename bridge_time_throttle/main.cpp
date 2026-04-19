#include <iostream>
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

void InitBridgeLogging() {
    try {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "D:/projects/C++/bridge_time_throttle/logs/bridge.log", 1024 * 1024 * 2, 3
        );
        file_sink->set_pattern("[%H:%M:%S.%e] [%l] %v");
        auto logger = std::make_shared<spdlog::logger>("bridge", file_sink);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::err);
    } catch (...) {
    }
}

int main() {
    InitBridgeLogging();
    spdlog::info("--- Bridge Instance Started ---");

    //  бинарный режим а не текстовый
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    unsigned int length = 0;

    // цикл чтения из Chrome
    while (std::cin.read(reinterpret_cast<char*>(&length), 4)) {

        if (length == 0) continue;
        if (length > 1024 * 1024) { // Лимит 1МБ
            spdlog::critical("Message too large: {} bytes. Potential protocol error.", length);
            break;
        }

        // 2. ЧТЕНИЕ ТЕЛА СООБЩЕНИЯ
        std::string jsonStr(length, '\0');
        std::cin.read(&jsonStr[0], length);

        if (!std::cin) {
            spdlog::error("Incomplete message body received");
            break;
        }

        spdlog::debug("Received: {}", jsonStr);

        // передача через pipe
        HANDLE hPipe = CreateFileW(
                L"\\\\.\\pipe\\MyMonitorDomainPipe",
                GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten;
            if (WriteFile(hPipe, jsonStr.c_str(), (DWORD)jsonStr.length(), &bytesWritten, NULL)) {
                spdlog::trace("Sent to Pipe: {} bytes", bytesWritten);
            } else {
                spdlog::error("Pipe Write Error: {}", GetLastError());
            }
            CloseHandle(hPipe);
        } else {
            // Если Pipe не найден (TimeThrottle выключен), просто логируем это как trace
            spdlog::trace("Main app Pipe not available");
        }
    }

    spdlog::warn("Bridge loop terminated. Stdin closed or error occurred.");
    return 0;
}