#ifndef TIMETHROTTLE_CONTROL_PIPE_SERVER_H
#define TIMETHROTTLE_CONTROL_PIPE_SERVER_H

#include <atomic>
#include <functional>
#include <string>
#include <thread>

struct ControlCommandResult {
    bool exitRequested = false;
    std::string response;
};

class ControlPipeServer {
public:
    using CommandHandler = std::function<ControlCommandResult(const std::string&)>;

    explicit ControlPipeServer(CommandHandler handler);
    ~ControlPipeServer();

    void Start();
    void Stop();

private:
    void Run();
    void WakeServer() const;

    CommandHandler handler;
    std::thread worker;
    std::atomic<bool> running{false};
};

#endif // TIMETHROTTLE_CONTROL_PIPE_SERVER_H
