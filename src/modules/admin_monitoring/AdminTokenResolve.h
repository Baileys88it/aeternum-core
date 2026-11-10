/*
 * Aeternum Admin - token resolution helpers (header-only, Catch2-tested).
 * Precedence: environment variable wins over plaintext Admin.Token in conf.
 * If conf token is non-empty, callers MUST WARN (value never logged); fail-fast
 * for conf tokens is planned for a later version.
 * Per-instance: Admin.TokenFromEnv names the env var; empty -> AETERNUM_ADMIN_TOKEN.
 */
#ifndef AETERNUM_ADMIN_TOKEN_RESOLVE_H
#define AETERNUM_ADMIN_TOKEN_RESOLVE_H

#include <cstdint>
#include <string>
#include <string_view>

namespace AdminMonitoring
{
inline constexpr char const* kDefaultAdminTokenEnvVar = "AETERNUM_ADMIN_TOKEN";
inline constexpr char const* kDefaultAdminOperatorTokenEnvVar = "AETERNUM_ADMIN_OPERATOR_TOKEN";

/** Configured Admin.TokenFromEnv / OperatorTokenFromEnv; empty falls back to default. */
inline std::string EffectiveTokenEnvVarName(std::string_view configuredName,
    char const* defaultName = kDefaultAdminTokenEnvVar)
{
    if (configuredName.empty())
        return std::string(defaultName);
    return std::string(configuredName);
}

enum class TokenSource : uint8_t
{
    None = 0,
    Env = 1,
    Conf = 2
};

struct ResolvedToken
{
    std::string value;
    TokenSource source = TokenSource::None;
    bool confTokenNonEmpty = false; // true if conf had a plaintext token (WARN path)
};

/**
 * Resolve admin/operator secret.
 * Precedence: non-empty envToken wins; else confToken; else empty.
 * confTokenNonEmpty is set whenever confToken is non-empty (even if env wins),
 * so callers can emit a deprecation WARN without logging the value.
 */
inline ResolvedToken ResolveAdminToken(std::string_view confToken, std::string_view envToken)
{
    ResolvedToken r;
    r.confTokenNonEmpty = !confToken.empty();
    if (!envToken.empty())
    {
        r.value.assign(envToken.begin(), envToken.end());
        r.source = TokenSource::Env;
        return r;
    }
    if (!confToken.empty())
    {
        r.value.assign(confToken.begin(), confToken.end());
        r.source = TokenSource::Conf;
        return r;
    }
    return r;
}

inline char const* TokenSourceName(TokenSource s)
{
    switch (s)
    {
        case TokenSource::Env: return "env";
        case TokenSource::Conf: return "conf";
        default: return "none";
    }
}

/**
 * Format helpers for startup messages — NEVER include the token value.
 * Used by Catch2 leak tests and by AdminMonitoringScripts.
 */
inline std::string FormatConfTokenDeprecationWarn(bool isOperator)
{
    if (isOperator)
        return "Admin.OperatorToken is set in conf (plaintext); prefer Admin.OperatorTokenFromEnv / env. Value never logged. Fail-fast for conf tokens planned in a later version.";
    return "Admin.Token is set in conf (plaintext); prefer Admin.TokenFromEnv / env. Value never logged. Fail-fast for conf tokens planned in a later version.";
}

inline std::string FormatTokenMissingOrShortError(std::size_t minLength)
{
    return "Admin HTTP refused to start: token missing or shorter than Admin.TokenMinLength (" +
        std::to_string(static_cast<unsigned long long>(minLength)) +
        "); token value never logged";
}

inline std::string FormatAuthFailedAuditDetail()
{
    return "auth_failed";
}
}
#endif