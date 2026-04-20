#ifndef TIMETHROTTLE_APP_CONFIG_H
#define TIMETHROTTLE_APP_CONFIG_H

#include <string>
#include <set>
#include <mutex>

class AppConfig {
public:
    enum class Mode {
        OFF,
        NO_INTERNET,
        NO_MESSENGER_ONLY,
        NO_VIDEO_AND_STREAMING,
        NO_SOCIAL_MEDIA_AND_NO_VIDEO,
        CUSTOM,
    };

    static AppConfig& Instance() {
        static AppConfig instance;
        return instance;
    }

    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    void LoadFromFile();

    void SetMode(Mode m);
    Mode GetMode() const;

    void SetNetworkDelay(int ms);
    int GetNetworkDelay() const;

    void AddDomain(const std::string& domain);
    void RemoveDomain(const std::string& domain);
    void ClearDomains();
    std::set<std::string> GetDomains() const;

    void SetTimer(int minutes);
    bool IsTimerActive() const;
    long long GetRemainingSeconds() const;

   /* // Для CPU (добавь позже)
    void AddProcess(const std::string& process_name);
    void RemoveProcess(const std::string& process_name);
    void ClearProcesses();
    const std::set<std::string>& GetProcesses() const;

    void SetCpuLimit(double percent);  // 0..100
    double GetCpuLimit() const;*/

private:
    AppConfig() = default;

    void SaveToFile();

    mutable std::mutex mtx;
    Mode current_mode = Mode::OFF;
    int network_delay_ms = 0;
    double cpu_limit_percent = 100.0;  // 100 = без ограничения
    std::time_t timer_end_timestamp = 0;
    std::set<std::string> blocked_domains;
    std::set<std::string> target_processes;
};
namespace Blacklist {
    const std::set<std::string> Messenger = {
            "telegram.org", "t.me", "vk.com", "instagram.com",
            "facebook.com", "ok.ru", "whatsapp.com", "twitter.com"
    };

    const std::set<std::string> VideoStreaming = {
            "youtube.com", "youtu.be", "googlevideo.com",
            "ytimg.com", "ggpht.com", "twitch.tv",
            "netflix.com", "vimeo.com", "rutube.ru"
    };

    bool IsMessenger(const std::string& domain);
    bool IsVideoStreaming(const std::string& domain);
    bool IsCustomDomain(const std::string& domai);
}
#endif //TIMETHROTTLE_APP_CONFIG_H
