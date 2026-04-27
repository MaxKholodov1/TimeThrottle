#ifndef TIMETHROTTLE_UI_IPC_H
#define TIMETHROTTLE_UI_IPC_H

#include <string>

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);

std::wstring SendCommandToCore(const std::wstring& command);

#endif // TIMETHROTTLE_UI_IPC_H
