/*
 * Aeternum Engine public API — runtime / process / DB ping (SDK 1.0).
 * Real consumer: admin_monitoring HTTP + Module SDK host.
 * Trinity types forbidden here; implementations live in sdk/adapters/trinity.
 */
#ifndef AETERNUM_API_OPS_RUNTIME_HPP
#define AETERNUM_API_OPS_RUNTIME_HPP

#include "aeternum/api/status.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace aeternum::ops
{
enum class ProcessExit : unsigned char
{
    Shutdown = 0,
    Error = 1,
    Restart = 2
};

class IRuntimeStats
{
public:
    virtual ~IRuntimeStats() = default;
    virtual std::uint64_t UptimeSeconds() const = 0;
    virtual std::uint32_t ActiveSessionCount() const = 0;
    virtual std::uint32_t QueuedSessionCount() const = 0;
    virtual std::uint32_t PlayerCount() const = 0;
};

class IProcessControl
{
public:
    virtual ~IProcessControl() = default;
    virtual void StopNow(ProcessExit code) = 0;
    virtual void SendServerMessage(std::string_view message) = 0;
    virtual void RequestShutdown(std::uint32_t delaySeconds, std::string_view reason) = 0;
};

class IDatabasePing
{
public:
    virtual ~IDatabasePing() = default;
    /** Read-only liveness probes (SELECT 1). Never mutate. */
    virtual bool PingWorld() = 0;
    virtual bool PingCharacter() = 0;
    virtual bool PingLogin() = 0;
};

class IScriptCatalog
{
public:
    virtual ~IScriptCatalog() = default;
    virtual std::uint32_t ScriptCount() const = 0;
};
}

#endif