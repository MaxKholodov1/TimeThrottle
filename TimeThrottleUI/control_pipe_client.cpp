#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <string>

#include "control_pipe_client.h"

namespace {
constexpr wchar_t kControlPipeName[] = L"\\\\.\\pipe\\TimeThrottleControlPipe";
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return L"";
    }

    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size);
    return out;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return "";
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return "";
    }

    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring SendCommandToCore(const std::wstring& command) {
    HANDLE pipe = CreateFileW(
        kControlPipeName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        return L"[UI] core pipe not available";
    }

    std::string utf8 = WideToUtf8(command);
    utf8.push_back('\n');

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe, utf8.data(), static_cast<DWORD>(utf8.size()), &bytesWritten, nullptr)) {
        CloseHandle(pipe);
        return L"[UI] failed to send command";
    }

    std::array<char, 1024> buffer{};
    DWORD bytesRead = 0;
    if (ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size() - 1), &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        CloseHandle(pipe);
        return Utf8ToWide(std::string(buffer.data(), bytesRead));
    }

    CloseHandle(pipe);
    return L"[UI] command sent";
}
