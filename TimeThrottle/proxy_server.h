#ifndef TIMETHROTTLE_PROXY_SERVER_H
#define TIMETHROTTLE_PROXY_SERVER_H

#include <thread>
#include <vector>
#include <iostream>
#include "net_delay.h"

#pragma comment(lib, "ws2_32.lib")
class NetDelay;

class ProxyServer{
public:
    ProxyServer(NetDelay *netDelay, uint16_t port = 8899);
    ~ProxyServer();
    void Start();
    void Stop();
private:
    NetDelay *netDelay;
    uint16_t port;
    std::thread serverThread;
    SOCKET listenSocket = INVALID_SOCKET;
    bool running;

    void Run();
    void HandleClient(SOCKET clientSocket);
    void TunnelSockets(SOCKET clientSocket, SOCKET remoteSocket) const;
};
#endif //TIMETHROTTLE_PROXY_SERVER_H
