#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <cstring>

namespace AirGuitar {

class CrashLogger
{
public:
    static CrashLogger& instance()
    {
        static CrashLogger inst;
        return inst;
    }

    void init(const std::string& appDir)
    {
        std::lock_guard<std::mutex> lock(mu);
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto* tm = std::localtime(&t);

        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm);
        logPath = appDir + "/crash_" + buf + ".log";

        std::ofstream f(logPath, std::ios::app);
        f << "=== AirGuitar Crash Log ===" << "\n";
        f << "Timestamp: " << buf << "\n";
        f << "Version: 0.1.0" << "\n";
        f << "---" << "\n";
    }

    void log(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mu);
        if (logPath.empty())
            return;

        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto* tm = std::localtime(&t);

        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);

        std::ofstream f(logPath, std::ios::app);
        f << "[" << buf << "] " << message << "\n";
    }

    void logError(const std::string& context, const std::string& detail)
    {
        log("[ERROR] " + context + ": " + detail);
    }

    void logInfo(const std::string& message)
    {
        log("[INFO] " + message);
    }

    std::string getLogPath() const { return logPath; }

    void installSignalHandlers()
    {
        std::signal(SIGSEGV, signalHandler);
        std::signal(SIGABRT, signalHandler);
        std::signal(SIGFPE, signalHandler);
        std::signal(SIGILL, signalHandler);
    }

private:
    CrashLogger() = default;

    std::mutex mu;
    std::string logPath;

    static void signalHandler(int sig)
    {
        const char* sigName = "UNKNOWN";
        switch (sig)
        {
            case SIGSEGV: sigName = "SIGSEGV (segmentation fault)"; break;
            case SIGABRT: sigName = "SIGABRT (abort)"; break;
            case SIGFPE:  sigName = "SIGFPE (floating point exception)"; break;
            case SIGILL:  sigName = "SIGILL (illegal instruction)"; break;
        }

        auto& inst = instance();
        std::lock_guard<std::mutex> lock(inst.mu);
        if (!inst.logPath.empty())
        {
            std::ofstream f(inst.logPath, std::ios::app);
            f << "\n!!! CRASH: " << sigName << " !!!\n";
            f << "Signal: " << sig << "\n";
        }

        std::_Exit(128 + sig);
    }
};

} // namespace AirGuitar
