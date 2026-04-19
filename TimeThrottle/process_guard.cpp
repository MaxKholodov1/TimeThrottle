#include "process_guard.h"
#include <spdlog/spdlog.h>
#include <iostream>

const WCHAR* szName = L"Local\\MySharedMem";

void ProcessGuard::StartProtection(const std::wstring& watchdogPath) {
    if (running) return;

    // 1. Инициализируем общую память ДО запуска потоков
    if (!InitSharedMemory()) {
        spdlog::error("Критическая ошибка: Не удалось создать Shared Memory.");
        return;
    }

    running = true;

    // 2. Запускаем обновление пульса
    RunHeartBeat();

    // 3. Запускаем мониторинг самого Watchdog
    monitorThread = std::thread([this, watchdogPath]() {
        const std::wstring watchdogMutexName = L"Global\\WatchDog_Unique_Mutex";

        spdlog::info("Фоновая защита запущена...");

        while (running) {
            if (!IsProcessAlive(watchdogMutexName)) {
                spdlog::warn("Watchdog не найден! Перезапуск...");
                DWORD dummyPid;
                if (StartProcess(watchdogPath, dummyPid)) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }else {
                    spdlog::warn("не удалось запустить");
                }

            }else{
                if(!CheckHeartBeat()){
                    spdlog::warn("Watchdog спит! Перезапуск...");
                    // убиваем чтобы освободить mutex
                    KillProcessByName(L"WinSvcMonitor.exe");
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    // возрождаем
                    DWORD dummyPid;
                    if (StartProcess(watchdogPath, dummyPid)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds (1500));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
}

bool ProcessGuard::InitSharedMemory() {
    if (sharedData) return true;

    // Создаем объект в памяти (размером ровно под структуру) или если уже создать то вернет указатель на обьект
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

    // Начальная метка времени
    sharedData->mainAppTick.store(GetTickCount64());
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
        sharedData->mainAppTick.store(GetTickCount64());
    }
}
bool ProcessGuard::CheckHeartBeat() {
    auto now = GetTickCount64();

    if (now - sharedData->watchDogTick > 2000) { // Если тика не было 2 секунд
        spdlog::warn("Процесс завис или не шлет Heartbeat! Перезапуск...");
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

// Вспомогательные функции (оставляем твою логику)
bool ProcessGuard::IsProcessAlive(const std::wstring& mutexName) {
    HANDLE h = OpenMutexW(SYNCHRONIZE, FALSE, mutexName.c_str());
    if (h) {
        CloseHandle(h);
        return true;
    }
    return false;
}

bool ProcessGuard::StartProcess(const std::wstring& appPath, DWORD& outPid) {

    std::wstring cmdLine = L"\"" + appPath + L"\"";
    // spdlog::info(appPath.c_str());
    STARTUPINFOW info = { sizeof(info) };
    PROCESS_INFORMATION pi = { 0 };
    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(L'\0');

    if (CreateProcessW(NULL, buf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &info, &pi)) {
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
                    spdlog::warn("Killed process: {}", std::pmr::string( filename.begin(), filename.end()));
                }
            }
        } while (Process32NextW(hSnapShot, &pEntry));
    }
    CloseHandle(hSnapShot);
}