#ifndef TRINITYCORE_CONFIG_SCHEMA_H
#define TRINITYCORE_CONFIG_SCHEMA_H

#include "Define.h"
#include <string>
#include <string_view>
#include <vector>

namespace Trinity::ConfigSchema
{
struct ValidationIssue
{
    enum class Kind { MissingRequired, UnknownKey, EmptyRequired };
    Kind kind;
    std::string key;
    std::string message;
};

struct ValidationResult
{
    bool ok = true;                 // false if any MissingRequired / EmptyRequired
    std::vector<ValidationIssue> issues;
    uint32 unknownWarned = 0;
    uint32 requiredFailed = 0;
};

/// Prefixes considered "known" for WARN-on-unknown (flat Trinity keys + Aeternum).
TC_COMMON_API bool IsKnownKey(std::string_view key);

/// Required keys that must be present and non-empty for worldserver start / --check-config.
TC_COMMON_API std::vector<std::string> DefaultRequiredKeys();

/// Validate currently loaded sConfigMgr keys. Does not touch DB.
TC_COMMON_API ValidationResult ValidateLoadedConfig(bool warnUnknown = true);

/// Print issues to stdout (for --check-config). Returns process exit code 0/1.
TC_COMMON_API void LogIssues(ValidationResult const& r);
TC_COMMON_API int PrintAndExitCode(ValidationResult const& r);
}

#endif
