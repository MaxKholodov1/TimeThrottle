#include "control_pipe_server.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <iostream>
#include <string>
#include <utility>

#include "spdlog/spdlog.h"

namespace {
constexpr wchar_t kControlPipeName[] = L"\\\\.\\pipe\\TimeThrottleControlPipe";
}

ControlPipeServer::ControlPipeServer(CommandHandler handler)
    : handler(std::move(handler)) {}

ControlPipeServer::~ControlPipeServer() {
    Stop();
}

void ControlPipeServer::Start() {
    std::cout << "Starting control pipe server..." << std::endl;
    if (running.exchange(true)) {
        return;
    }
    worker = std::thread(&ControlPipeServer::Run, this);
}

void ControlPipeServer::Stop() {
    if (!running.exchange(false)) {
        return;
    }

    WakeServer();
    if (worker.joinable()) {
        worker.join();
    }
}

void ControlPipeServer::WakeServer() const {
    HANDLE pipe = CreateFileW(
        kControlPipeName,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (pipe != INVALID_HANDLE_VALUE) {
        const char wake[] = "noop\n";
        DWORD written = 0;
        WriteFile(pipe, wake, static_cast<DWORD>(sizeof(wake) - 1), &written, nullptr);
        CloseHandle(pipe);
    }
}

void ControlPipeServer::Run() {
    while (running.load()) {
        HANDLE pipe = CreateNamedPipeW(
            kControlPipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            4096,
            4096,
            0,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            spdlog::error("[ControlPipe] Failed to create pipe: {}", GetLastError());
            continue;
        }

        const bool connected = ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
        if (!connected) {
            CloseHandle(pipe);
            continue;
        }

        if (!running.load()) {
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            break;
        }

        std::array<char, 2048> buffer{};
        DWORD bytesRead = 0;
        if (ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size() - 1), &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string command(buffer.data(), bytesRead);
            while (!command.empty() && (command.back() == '\n' || command.back() == '\r' || command.back() == '\0')) {
                command.pop_back();
            }

            auto result = handler ? handler(command) : ControlCommandResult{};
            std::string response = result.response.empty() ? "ok\n" : result.response;
            if (response.back() != '\n') {
                response.push_back('\n');
            }

            DWORD bytesWritten = 0;
            WriteFile(pipe, response.data(), static_cast<DWORD>(response.size()), &bytesWritten, nullptr);

            if (result.exitRequested) {
                running.store(false);
            }
        }

        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}
