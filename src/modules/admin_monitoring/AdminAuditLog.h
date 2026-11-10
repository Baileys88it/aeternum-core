#ifndef AETERNUM_ADMIN_AUDIT_LOG_H
#define AETERNUM_ADMIN_AUDIT_LOG_H

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace AdminMonitoring
{
struct AuditEvent
{
    std::string_view Who;      // peer address (never a token)
    std::string_view What;     // METHOD path
    std::string_view Result;   // ok|denied|error|rate_limited|...
    int Status = 0;
    std::string_view Role;
    std::string_view Detail;   // free-form, never tokens
    uint32_t DurationMs = 0;
};

class AuditLog
{
public:
    void Configure(std::string path, uint64_t rotateBytes = 2ull * 1024ull * 1024ull, uint32_t keepRotated = 3);
    /// Never pass token material into event fields.
    void Write(AuditEvent const& ev);
    /// Back-compat thin wrapper used by older call sites.
    void Write(std::string_view method, std::string_view path, int status, std::string_view role, std::string_view detail = {});

private:
    void RotateIfNeededUnlocked();

    std::mutex _mutex;
    std::string _path;
    uint64_t _rotateBytes = 2ull * 1024ull * 1024ull;
    uint32_t _keepRotated = 3;
};
}
#endif
