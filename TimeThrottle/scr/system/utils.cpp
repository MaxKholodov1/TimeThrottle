#include "utils.h"
#include <psapi.h>
#include <sstream>

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string out(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

std::string GetProcessNameUtf8(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == nullptr) return "unknown";

    wchar_t exeName[MAX_PATH];
    if (GetModuleFileNameExW(hProcess, nullptr, exeName, MAX_PATH)) {
        std::wstring wname(exeName);
        size_t pos = wname.find_last_of(L"\\");
        if (pos != std::wstring::npos) {
            wname = wname.substr(pos + 1);
        }
        CloseHandle(hProcess);
        return WideToUtf8(wname);
    }
    CloseHandle(hProcess);
    return "unknown";
}
std::string ParseDomain(std::string json) {
    size_t start = json.find(":\"") + 2;
    size_t end = json.find("\"", start);
    if (start != std::string::npos && end != std::string::npos) {
        return json.substr(start, end - start);
    }
    return "";
}
std::string ModeToString(AppConfig::Mode m) {
    switch (m) {
        case AppConfig::Mode::OFF:            return "OFF";
        case AppConfig::Mode::NO_INTERNET:   return "NO_INTERNET";
        case AppConfig::Mode::NO_MESSENGER_ONLY:   return "NO_MESSENGER_ONLY";
        case AppConfig::Mode::NO_VIDEO_AND_STREAMING:   return "NO_VIDEO_AND_STREAMING";
        case AppConfig::Mode::NO_SOCIAL_MEDIA_AND_NO_VIDEO:   return "NO_SOCIAL_MEDIA_AND_NO_VIDEO";
        case AppConfig::Mode::CUSTOM:         return "CUSTOM";
        default:             return "UNKNOWN";
    }
}
std::vector<std::string> Tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string temp;
    while (ss >> temp) tokens.push_back(temp);
    return tokens;
}