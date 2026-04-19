#ifndef TIMETHROTTLE_PROCESS_GUARD_H
#define TIMETHROTTLE_PROCESS_GUARD_H


#include <windows.h>
#include <atomic>
#include <string>
#include <thread>
#include <tlhelp32.h>


struct HeartbeatData {
    std::atomic<long long> mainAppTick; // Атомарная запись времени mainApp
    std::atomic<long long> watchDogTick; // Атомарная запись времени watchDog
};

class ProcessGuard {
public:
    ProcessGuard() : hMapFile(NULL), sharedData(nullptr), running(false) {}
    ~ProcessGuard();

    void StartProtection(const std::wstring& watchdogPath);
    void StopProtection();

private:
    bool InitSharedMemory();
    void RunHeartBeat();
    void UpdateHeartBeatInSharedMemory();
    bool CheckHeartBeat();
    bool IsProcessAlive(const std::wstring& mutexName);
    bool StartProcess(const std::wstring& appPath, DWORD& outPid);
    void KillProcessByName(const std::wstring& filename);


    HANDLE hMapFile;
    HeartbeatData* sharedData;
    std::atomic<bool> running;
    std::thread monitorThread;
    std::thread heartBeatThread;
};

#endif //TIMETHROTTLE_PROCESS_GUARD_H
