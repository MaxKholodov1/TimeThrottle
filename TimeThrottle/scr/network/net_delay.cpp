#include <windows.h>

#include "net_delay.h"
#include "../core/app_config.h"
#include "../lib/spdlog/include/spdlog/spdlog.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

NetDelay::NetDelay() {
    hDivert = INVALID_HANDLE_VALUE;
    running = false;
    std::cout << "[NetDelay] Object created. Ready to throttle.\n";
}

NetDelay::~NetDelay() {
    Stop();
    std::cout << "[NetDelay] Object destroyed.\n";
}

void NetDelay::Start() {
    if (running) return;

    running = true;

    worker_thread = std::thread(&NetDelay::RunLoop, this);
}

void NetDelay::Stop() {
    running = false;

    // Закрываем дескриптор WinDivert, чтобы вывести RunLoop из ожидания
    if (hDivert != INVALID_HANDLE_VALUE) {
        WinDivertClose(hDivert);
        hDivert = INVALID_HANDLE_VALUE;
    }

    if (worker_thread.joinable()) { // Ждем, пока поток корректно завершится
        worker_thread.join();
    }
}

void NetDelay::ForceMapDomain(uint32_t ip, const std::string &domain) {
    std::lock_guard<std::mutex> lock(map_mutex);
    ip_to_domain[ip] = domain;    // Сохраняем домен для этого IP
}


void NetDelay::RunLoop() {
    // открываем WinDivert (фильтруем только входящий/исходящий TCP трафик)
    hDivert = WinDivertOpen("tcp", WINDIVERT_LAYER_NETWORK, 0, 0);

    if (hDivert == INVALID_HANDLE_VALUE) {
        spdlog::critical("[NetDelay] Failed to open WinDivert\n");
        return;
    }

    char packet[65535];
    WINDIVERT_ADDRESS addr;
    UINT packetLen;


    while (running) {
        // получаем пакет
        if (!WinDivertRecv(hDivert, packet, sizeof(packet), &packetLen, &addr)) {
            continue;
        }

        PWINDIVERT_IPHDR ip_header;
        PWINDIVERT_TCPHDR tcp_header;

        // парсим IP и TCP заголовки одновременно
        WinDivertHelperParsePacket(
                packet,
                packetLen,
                &ip_header,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                &tcp_header,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr
        );

        if (ip_header && tcp_header) {

            // разрешаем пакеты, которые не выходят в интернет (внутри ПК)
            // это позволит браузеру общаться с прокси на 127.0.0.1 без задержек
            if (IsLocalIP(ip_header->SrcAddr) && IsLocalIP(ip_header->DstAddr)) {
                WinDivertSend(hDivert, packet, packetLen, nullptr, &addr);
                continue;
            }

            // всё, что идет в интернет или из него
            uint32_t targetIp = addr.Outbound ? ip_header->DstAddr : ip_header->SrcAddr;

            if (ShouldBlock(targetIp, AppConfig::Instance().GetMode())) {
                spdlog::trace("[NetDelay] BLOCKED: {}", targetIp);
                continue; // Пакет не отправляем, он уничтожен
            }
        }

        // Если пакет прошел все проверки — отправляем его дальше
        WinDivertSend(hDivert, packet, packetLen, nullptr, &addr);
    }
}

bool NetDelay::ShouldBlock(uint32_t ip, AppConfig::Mode mode) {
    if (mode == AppConfig::Mode::NO_INTERNET) {
        return true;
    }
    if (!AppConfig::Instance().IsTimerActive()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(map_mutex);
    auto it = ip_to_domain.find(ip);
    if (it == ip_to_domain.end())
        return false;
    std::string domain = it->second;

    switch (mode) {
        case AppConfig::Mode::OFF:
            return false;
        case AppConfig::Mode::NO_MESSENGER_ONLY:
            return Blacklist::IsMessenger(domain);
        case AppConfig::Mode::NO_VIDEO_AND_STREAMING:
            return Blacklist::IsVideoStreaming(domain);
        case AppConfig::Mode::NO_SOCIAL_MEDIA_AND_NO_VIDEO:
            return Blacklist::IsVideoStreaming(domain) or Blacklist::IsMessenger(domain);
        case AppConfig::Mode::NO_INTERNET:
            return true;
        case AppConfig::Mode::CUSTOM:
            return Blacklist::IsCustomDomain(domain);
        default:
            return false;

    }
}
bool NetDelay::IsLocalIP(uint32_t ip) {
    uint32_t h_ip = ntohl(ip);

    // 0x7F000000 это 127 в первом байте
    if ((h_ip & 0xFF000000) == 0x7F000000) {
        return true;
    }
    return false;
}