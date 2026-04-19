#ifndef TIMETHROTTLE_NET_DELAY_H
#define TIMETHROTTLE_NET_DELAY_H

#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <map>
#include "windivert.h"
#include "app_config.h"


class NetDelay {

public:
    NetDelay();
    ~NetDelay();
    void Start();
    void Stop();
    void ForceMapDomain(uint32_t ip, const std::string& domain);
private:
    void RunLoop();
    bool ShouldBlock(uint32_t ip, AppConfig::Mode mode);
    bool IsLocalIP(uint32_t ip);

    std::mutex mtx;

    std::thread worker_thread;

    std::atomic<bool> running{false};
    HANDLE hDivert = INVALID_HANDLE_VALUE;
    std::map<uint32_t, std::string> ip_to_domain;
    std::mutex map_mutex;

};


#endif //TIMETHROTTLE_NET_DELAY_H
