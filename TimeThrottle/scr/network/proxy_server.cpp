#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "proxy_server.h"
#include "net_delay.h"
#include "spdlog/spdlog.h"

ProxyServer::ProxyServer(NetDelay* netDelay, uint16_t port)
        : netDelay(netDelay), port(port) {}

ProxyServer::~ProxyServer() {
    Stop();
}

void ProxyServer::Start() {
    if (running.exchange(true)) {
        return;
    }
    serverThread = std::thread(&ProxyServer::Run, this);
}

void ProxyServer::Stop() {
    if (!running.exchange(false)) {
        return;
    }

    if (listenSocket != INVALID_SOCKET) {
        // Это  поток из ожидания
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    if (serverThread.joinable())
        serverThread.join();

    spdlog::info("[Proxy] Stopped.");
}

void ProxyServer::Run() {
    WSADATA wsaData;
    WORD DllVersion = MAKEWORD(2, 2); // просто версия
    if (WSAStartup(DllVersion, &wsaData) != 0){ // инициализация wsadata
        spdlog::critical("[Proxy] WSAStartup failed");
        return;
    }
    // listenSocket  это сокет который слушает клиентов внутри устройства
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET) { //сокет состоит из ip, порта и протокола
        spdlog::critical("[Proxy]  Failed to create listen socket");
        WSACleanup();
        return;
    }
    sockaddr_in addr{};
    // создаем адрес для сокета
    addr.sin_family = AF_INET; //IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // любой IP
    addr.sin_port = htons(port);  // указанный порт

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { // связываем
        spdlog::critical("[Proxy]  Bind failed");
        // соект и порт
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) { // 2 параметр - длина очереди ожидающих сообщений
        spdlog::critical("[Proxy] Listen failed");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "[Proxy] Listening on port " << port << "\n";

    while(running.load()){
        // принимаем новых клиентов и создаем для них новый socket, через который и будет общаться proxy с этим клинетом
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr); // второй и третий параметры address и adress len

        if (!running.load()) break;

        if (clientSocket == INVALID_SOCKET) continue;

        std::thread(&ProxyServer::HandleClient, this, clientSocket).detach();
    }

    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }
    WSACleanup();
}
void ProxyServer::HandleClient(SOCKET clientSocket){
    char buffer[4096]; // создаем буфер
    int amountOfBytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (amountOfBytesRead <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[amountOfBytesRead] = '\0';
    std::string request(buffer);

    // Проверяем CONNECT запрос
    if (request.find("CONNECT") == 0) {

        size_t firstSpace = request.find(' ');
        size_t firstColon = request.find(':', firstSpace);
        size_t secondSpace = request.find(' ', firstColon);
        std::string port_str = request.substr(firstColon + 1, secondSpace - firstColon - 1);

        // Если кто-то пытается через прокси подключиться к самому прокси чтобы не было типо рекурсии
        if (port_str == std::to_string(this->port)) {
            spdlog::error("[Proxy] Recursion detected on port {}. Dropping", port_str);

            closesocket(clientSocket);
            return;
        }

        if (firstSpace != std::string::npos && firstColon != std::string::npos) {
            // получает подстроку домена
            std::string domain = request.substr(firstSpace + 1, firstColon - firstSpace - 1);

            // Получаем IP через DNS
            struct addrinfo hints{}, *target_addr_info = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(domain.c_str(), "443", &hints, &target_addr_info) == 0) { // узнаем инфу про домен
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)target_addr_info->ai_addr; // получаем ip
                uint32_t realIp = ipv4->sin_addr.S_un.S_addr; // ip  в новый формат

                netDelay->ForceMapDomain(realIp, domain); // Сохраняем ip->домен в NetDelay

                spdlog::trace("[Proxy] Mapped {}   ->  {} ", domain, inet_ntoa(ipv4->sin_addr) );


                SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);// Создаём сокет к реальному серверу
                if (remoteSocket == INVALID_SOCKET) {
                    freeaddrinfo(target_addr_info);
                    closesocket(clientSocket);
                    return;
                }

                if (connect(remoteSocket, (sockaddr*)ipv4, sizeof(*ipv4)) != 0) {
                    int err = WSAGetLastError();
                    spdlog::warn("[Proxy] Failed to connect. Error code: {}", err);
                    freeaddrinfo(target_addr_info);
                    closesocket(remoteSocket);
                    closesocket(clientSocket);
                    return;
                }

                freeaddrinfo(target_addr_info);

                std::string response = "HTTP/1.1 200 Connection Established\r\n\r\n";                // Отправляем OK браузеру
                send(clientSocket, response.c_str(), (int)response.length(), 0);

                TunnelSockets(clientSocket, remoteSocket);// Туннель: двунаправленная пересылка
                return;
            }
        }
    }else{
        //TODO надо обработать случай когда нет connect соответственно это http и там какой то гемор
    }
    closesocket(clientSocket);
}
void ProxyServer::TunnelSockets(SOCKET client, SOCKET remote) const {
    fd_set readfds;
    char buf[16384]; // Буфер  (16 КБ)

    while (running.load()) {
        FD_ZERO(&readfds);      // Очищаем набор дескрипторов
        FD_SET(client, &readfds); // Добавляем сокет клиента
        FD_SET(remote, &readfds); // Добавляем сокет сервера

        timeval timeout{0, 100000};        // Ждем активности на сокетах (таймаут  секунд, чтобы проверять running)
        int ret = select(0, &readfds, nullptr, nullptr, &timeout);

        if (ret < 0) break;    // Ошибка select
        if (ret == 0) continue; // Вышел таймаут, просто идем на новый круг

        if (FD_ISSET(client, &readfds)) {        // 1. Если данные от клиента  шлем их серверу
            int n = recv(client, buf, sizeof(buf), 0);
            if (n <= 0) break; // Клиент закрыл соединение

            int sent = send(remote, buf, n, 0);
            if (sent <= 0) break; // Ошибка отправки на сервер
        }

        // 2. Если данные от сервера, шлем их клиенту
        if (FD_ISSET(remote, &readfds)) {
            int n = recv(remote, buf, sizeof(buf), 0);
            if (n <= 0) break; // сервер закрыл соединение

            int sent = send(client, buf, n, 0);
            if (sent <= 0) break; // ошибка отправки браузеру
        }
    }

    closesocket(client);
    closesocket(remote);
}

