#ifndef TIMETHROTTLE_CONSOLE_COMMANDS_H
#define TIMETHROTTLE_CONSOLE_COMMANDS_H

#include <string>
#include <vector>

class ProcessGuard;
class NetDelay;
class ProxyServer;

class ConsoleCommands {
public:
    enum class Result {
        Continue,
        ExitRequested
    };

    ConsoleCommands(ProcessGuard& guard, NetDelay& netController, ProxyServer& proxy);

    Result HandleLine(const std::string& input);

private:
    ProcessGuard& guard;
    NetDelay& netController;
    ProxyServer& proxy;

    void PrintHelp() const;
    void RunSafeShutdown() const;
    void RunKillSequence(const std::wstring& taskName) const;
    bool TryHandleModeCommand(const std::vector<std::string>& args) const;
};

#endif //TIMETHROTTLE_CONSOLE_COMMANDS_H
