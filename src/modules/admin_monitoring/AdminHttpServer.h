#ifndef AETERNUM_ADMIN_HTTP_SERVER_H
#define AETERNUM_ADMIN_HTTP_SERVER_H

#include "AdminAuditLog.h"
#include "AdminAuthRateLimit.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace AdminMonitoring
{
struct ServerConfig
{
    bool Enable = false;
    std::string BindIP = "127.0.0.1";
    uint16_t Port = 0;
    std::string Token;         // readonly; never log
    std::string OperatorToken; // M9 drain; never log; distinct from Token
    std::size_t TokenMinLength = 32;
    std::size_t AuthFailMax = 10;
    uint32_t AuthFailWindowSec = 60;
    std::string MetricsDirectory;
    std::string AuditLogPath = "Logs/admin_audit.log";
    uint64_t AuditRotateBytes = 2ull * 1024ull * 1024ull;
    uint32_t AuditKeepRotated = 3;
    bool DenseScrapeDefault = false;
    uint32_t MetricsTopN = 20;
    // M9 drain — DISABLED by default
    bool DrainEnable = false;
    uint32_t DrainMinDelaySec = 120;
};

class HttpServer
{
public:
    HttpServer();
    ~HttpServer();

    HttpServer(HttpServer const&) = delete;
    HttpServer& operator=(HttpServer const&) = delete;

    bool Start(ServerConfig cfg);
    void Stop();
    bool IsRunning() const { return _running.load(); }

    /// Hot-reload token / rate-limit / dense / drain flags (conf reload). Never logs tokens.
    void ReloadAuthConfig(ServerConfig const& cfg);

private:
    void ThreadMain();
    ServerConfig SnapshotConfig() const;

    struct Impl;
    std::unique_ptr<Impl> _impl;
    mutable std::mutex _cfgMutex;
    ServerConfig _cfg;
    AuditLog _audit;
    AuthFailureRateLimiter _authLimiter{10, std::chrono::seconds(60)};
    std::atomic<bool> _running{false};
    std::atomic<bool> _stop{false};
    std::thread _thread;
};
}
#endif
