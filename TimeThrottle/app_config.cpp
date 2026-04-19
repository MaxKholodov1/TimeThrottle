#include "app_config.h"
#include "utils.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

const std::string CONFIG_FILE = "settings.json";

void AppConfig::LoadFromFile() {
    std::lock_guard<std::mutex> lock(mtx);
    std::ifstream f(CONFIG_FILE);
    if (!f.is_open()) {
        spdlog::warn("[Config] Config file not found. Using defaults.");
        return;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(f);

        network_delay_ms = j.value("network_delay_ms", 100);

        if (j.contains("current_mode")) {
            current_mode = static_cast<Mode>(j["current_mode"].get<int>());
        }

        if (j.contains("blocked_domains")) {
            blocked_domains.clear(); // Очищаем старое перед загрузкой
            for (const auto& d : j["blocked_domains"]) {
                blocked_domains.insert(d.get<std::string>());
            }
        }
        timer_end_timestamp = j.value("timer_end", (std::time_t)0);
        spdlog::info("[Config] Configuration loaded successfully");
    } catch (const std::exception& e) {
        spdlog::error("[Config] Parse error: {}", e.what());
    }
}

void AppConfig::SetMode(Mode m) {
    std::lock_guard<std::mutex> lock(mtx);
    current_mode = m;
    SaveToFile();
    std::cout << "[Config] Mode set to: " <<ModeToString(m) << "\n";
}

AppConfig::Mode AppConfig::GetMode() const {
    std::lock_guard<std::mutex> lock(mtx);
    return current_mode;
}

void AppConfig::SetNetworkDelay(int ms) {
    std::lock_guard<std::mutex> lock(mtx);
    network_delay_ms = ms;
    SaveToFile();
    std::cout << "[Config] network_delay_ms set to: " << ms << "\n";
}
int AppConfig::GetNetworkDelay() const{
    std::lock_guard<std::mutex> lock(mtx);
    return network_delay_ms;
}

void AppConfig::AddDomain(const std::string &domain) {
    std::lock_guard<std::mutex> lock(mtx);
    if (blocked_domains.insert(domain).second) {
        SaveToFile(); // Автоматически сохраняем
        std::cout << "[Config] Inserted and saved: " << domain << "\n";
    }
}
void AppConfig::RemoveDomain(const std::string &domain) {
    std::lock_guard<std::mutex> lock(mtx);
    blocked_domains.erase(domain);
    std::cout << "[Config] remove from blocked_domains : " << domain << "\n";
}
void AppConfig::ClearDomains() {
    std::lock_guard<std::mutex> lock(mtx);
    blocked_domains.clear();
    std::cout << "[Config] clear blocked_domains \n";
}

const std::set<std::string>& AppConfig::GetDomains() const {
    std::lock_guard<std::mutex> lock(mtx);
    return blocked_domains;
}

void AppConfig::SetTimer(int minutes) {
    std::lock_guard<std::mutex> lock(mtx);
    if (minutes <= 0) {
        timer_end_timestamp = 0;
    } else {
        timer_end_timestamp = std::time(nullptr) + (minutes * 60);
    }
    SaveToFile();
}
bool AppConfig::IsTimerActive() const {
    return std::time(nullptr) < timer_end_timestamp;
}

void AppConfig::SaveToFile() {

    nlohmann::json j;

    j["network_delay_ms"] = network_delay_ms;
    j["blocked_domains"] = blocked_domains;
    j["current_mode"] = static_cast<int>(current_mode);
    j["timer_end"] = timer_end_timestamp; // время окончания


    std::ofstream file(CONFIG_FILE);
    if (file.is_open()) {
        file << j.dump(4);
        spdlog::info("[Config] Settings saved to {}", CONFIG_FILE);
    } else {
        spdlog::error("[Config] Failed to open {} for writing", CONFIG_FILE);
    }
}


bool Blacklist::IsMessenger(const std::string& domain) {
    for (const auto& item : Messenger) {
        // проверяем, содержится ли ключевое слово в домене
        // (чтобы блокировать и www.telegram.org, и web.telegram.org)
        if (domain.find(item) != std::string::npos) return true;
    }
    return false;
}
bool Blacklist::IsVideoStreaming(const std::string &domain) {
    for (const auto& item : VideoStreaming) {
        if (domain.find(item) != std::string::npos) return true; // проверяем, содержится ли ключевое слово в домене
    }
    return false;
}
bool Blacklist::IsCustomDomain(const std::string& domain) {
    const auto& custom = AppConfig::Instance().GetDomains();
    for (const auto& item : custom) {
        if (domain.find(item) != std::string::npos) return true; // проверяем, содержится ли ключевое слово в домене
    }
    return false;
}

