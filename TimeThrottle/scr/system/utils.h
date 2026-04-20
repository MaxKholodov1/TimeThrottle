#ifndef TIMETHROTTLE_UTILS_H
#define TIMETHROTTLE_UTILS_H

#include <string>
#include <vector>
#include <windows.h>
#include "../core/app_config.h"

std::string WideToUtf8(const std::wstring& w);
std::string GetProcessNameUtf8(DWORD pid);
std::string ParseDomain(std::string json);
std::string ModeToString(AppConfig::Mode m);
std::vector<std::string> Tokenize(const std::string& input);
#endif //TIMETHROTTLE_UTILS_H
