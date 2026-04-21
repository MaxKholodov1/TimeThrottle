#include "process_guard.h"
#include <spdlog/spdlog.h>
#include <iostream>

const WCHAR* szName = L"Local\\MySharedMem";


void ProcessGuard::StartProtection(const std::wstring& timeThrottlePath) {
    if (running) return;
    if (!InitSharedMemory()) {
        spdlog::error("Критическая ошибка: Не удалось создать Shared Memory.");
        return;
    }
    running = true;

    RunHeartBeat();

    //  Запускаем мониторинг самого timeThrottle
    monitorThread = std::thread([this, timeThrottlePath]() {
        const std::wstring timeThrottleMutexName = L"Global\\TimeThrottle_Unique_Mutex";

        spdlog::info("Фоновая защита запущена...");

        while (running) {
            if (!IsProcessAlive(timeThrottleMutexName)) {
                spdlog::warn("timeThrottle не найден! Перезапуск...");
                DWORD dummyPid;
                if (StartProcess(timeThrottlePath, dummyPid)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds (1500));
                }
            } else{
                if(!CheckHeartBeat()){
                    // убиваем чтобы освободить mutex
                    KillProcessByName(L"TimeThrottle.exe");
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    // возрождаем
                    DWORD dummyPid;
                    if (StartProcess(timeThrottlePath, dummyPid)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds (1500));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
}

bool ProcessGuard::InitSharedMemory() {
    // Если уже инициализировано, не делаем ничего
    if (sharedData) return true;

    hMapFile = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(HeartbeatData),
            szName);

    if (hMapFile == NULL) {
        spdlog::error("[FILE MAPPING] Creating  failed or opening failed: {}", GetLastError());
        return false;
    }


    sharedData = (HeartbeatData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HeartbeatData));

    if (sharedData == NULL) {
        spdlog::error("MapViewOfFile failed: {}", GetLastError());
        CloseHandle(hMapFile);
        hMapFile = NULL;
        return false;
    }

    return true;
}
void ProcessGuard::RunHeartBeat() {
    heartBeatThread = std::thread([this]() {
        spdlog::info("Поток Heartbeat запущен.");
        while (running) {
            UpdateHeartBeatInSharedMemory();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}

void ProcessGuard::UpdateHeartBeatInSharedMemory() {
    if (sharedData) {
        // Используем .store() для атомарной записи
        sharedData->watchDogTick.store(GetTickCount64());
    }
}
bool ProcessGuard::CheckHeartBeat() {
    auto now = GetTickCount64();

    if (now - sharedData->mainAppTick > 2000) { // Если тика не было 2 секунд
        spdlog::warn("Процесс завис или не шлет Heartbeat! Перезапуск...");
        std::cout << sharedData->mainAppTick << std::endl;
        return false;
    }
    return true;
}
void ProcessGuard::StopProtection() {
    if (!running) return;
    running = false;

    if (monitorThread.joinable()) monitorThread.join();
    if (heartBeatThread.joinable()) heartBeatThread.join();

    // Освобождаем ресурсы Windows
    if (sharedData) {
        UnmapViewOfFile(sharedData);
        sharedData = nullptr;
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }
    spdlog::info("Защита остановлена, ресурсы очищены.");
}

ProcessGuard::~ProcessGuard() {
    StopProtection();
}


bool ProcessGuard::IsProcessAlive(const std::wstring& mutexName) {
    HANDLE h = OpenMutexW(SYNCHRONIZE, FALSE, mutexName.c_str());
    if (h) {
        CloseHandle(h);
        return true;
    }
    return false;
}

bool ProcessGuard::StartProcess(const std::wstring& appPath, DWORD& outPid) {
    wchar_t myPath[MAX_PATH];
    GetModuleFileNameW(NULL, myPath, MAX_PATH);

    std::wstring cmdLine = L"\"" + appPath + L"\" \"" + myPath + L"\"";
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
void ProcessGuard::KillProcessByName(const std::wstring &filename) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);

    if (Process32FirstW(hSnapShot, &pEntry)) {
        do {
            if (_wcsicmp(pEntry.szExeFile, filename.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pEntry.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 9);
                    CloseHandle(hProcess);
                    std::wcout << L"Killed process: " << filename << std::endl;
                }
            }
        } while (Process32NextW(hSnapShot, &pEntry));
    }
    CloseHandle(hSnapShot);
}