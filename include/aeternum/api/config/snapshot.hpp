/*
 * Aeternum Engine public API — config snapshot (SDK 1.0).
 * Real consumer: Config.Schema.* FAIL-fast / worldserver --check-config.
 * Never carry DatabaseInfo connection strings in public snapshots.
 */
#ifndef AETERNUM_API_CONFIG_SNAPSHOT_HPP
#define AETERNUM_API_CONFIG_SNAPSHOT_HPP

#include "aeternum/api/status.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aeternum::config
{
struct EngineConfigSnapshot
{
    std::string profileId;
    std::string confPath;
    std::uint16_t worldPort = 0;
    std::uint16_t bnetPort = 0;
    // Intentionally no DatabaseInfo / passwords / cert paths.
};

struct SchemaFinding
{
    enum class Severity : unsigned char { Info = 0, Warn, Error };
    Severity severity = Severity::Info;
    std::string key;
    std::string message;
};

class IConfigSchema
{
public:
    virtual ~IConfigSchema() = default;
    virtual Status Validate(EngineConfigSnapshot const& snap, std::vector<SchemaFinding>& out) const = 0;
};
}

#endif
