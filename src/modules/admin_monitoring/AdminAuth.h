/*
 * Aeternum Admin & Monitoring — auth + RBAC helpers (header-only, Catch2-tested).
 * Token values must never be logged. M8: min length + constant-time compare.
 * M9: Operator role (distinct token) for drain; ReadOnly unchanged.
 */
#ifndef AETERNUM_ADMIN_AUTH_H
#define AETERNUM_ADMIN_AUTH_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace AdminMonitoring
{
enum class Role : uint8_t
{
    None = 0,
    ReadOnly = 1,
    Operator = 2
};

inline constexpr std::size_t kDefaultTokenMinLength = 32;

inline bool ConstantTimeEqual(std::string_view a, std::string_view b)
{
    std::size_t const n = a.size() < b.size() ? b.size() : a.size();
    unsigned char diff = static_cast<unsigned char>(a.size() ^ b.size());
    for (std::size_t i = 0; i < n; ++i)
    {
        unsigned char ca = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        unsigned char cb = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        diff |= static_cast<unsigned char>(ca ^ cb);
    }
    return diff == 0;
}

inline bool TokenMeetsMinLength(std::string_view token, std::size_t minLength = kDefaultTokenMinLength)
{
    return !token.empty() && token.size() >= minLength;
}

inline bool ExtractBearerToken(std::string_view authorizationHeader, std::string& outToken)
{
    outToken.clear();
    constexpr std::string_view prefix = "Bearer ";
    if (authorizationHeader.size() < prefix.size())
        return false;
    if (authorizationHeader.substr(0, prefix.size()) != prefix)
        return false;
    outToken.assign(authorizationHeader.substr(prefix.size()));
    return !outToken.empty();
}

inline bool RoleAllows(Role role, std::string_view permission)
{
    if (role == Role::None)
        return false;
    if (role == Role::ReadOnly)
        return permission == "metrics.read" || permission == "admin.read";
    if (role == Role::Operator)
        return permission == "metrics.read" || permission == "admin.read" || permission == "admin.drain";
    return false;
}

/** v0/M8: valid readonly token (min length on expected) maps to ReadOnly. */
inline Role AuthenticateToken(std::string_view provided, std::string_view expected,
    std::size_t minLength = kDefaultTokenMinLength)
{
    if (!TokenMeetsMinLength(expected, minLength))
        return Role::None;
    if (provided.empty())
        return Role::None;
    if (!ConstantTimeEqual(provided, expected))
        return Role::None;
    return Role::ReadOnly;
}

/**
 * M9: prefer OperatorToken (distinct from readonly). Operator never equals readonly
 * when both are set — if tokens are identical, Operator wins (misconfig WARN at load).
 */
inline Role AuthenticateDualToken(std::string_view provided,
    std::string_view readToken, std::string_view operatorToken,
    std::size_t minLength = kDefaultTokenMinLength)
{
    if (provided.empty())
        return Role::None;
    if (TokenMeetsMinLength(operatorToken, minLength) && ConstantTimeEqual(provided, operatorToken))
        return Role::Operator;
    if (TokenMeetsMinLength(readToken, minLength) && ConstantTimeEqual(provided, readToken))
        return Role::ReadOnly;
    return Role::None;
}

inline char const* RoleName(Role r)
{
    switch (r)
    {
        case Role::ReadOnly: return "readonly";
        case Role::Operator: return "operator";
        default: return "none";
    }
}
}
#endif
