/*
 * Aeternum Admin & Monitoring — bind policy (localhost only).
 */
#ifndef AETERNUM_ADMIN_BIND_H
#define AETERNUM_ADMIN_BIND_H

#include <string_view>

namespace AdminMonitoring
{
inline bool IsLoopbackBindAddress(std::string_view addr)
{
    return addr == "127.0.0.1" || addr == "::1" || addr == "localhost";
}

inline bool ValidateAdminBindOrReject(std::string_view addr)
{
    if (addr.empty())
        return false;
    if (addr == "0.0.0.0" || addr == "::" || addr == "*")
        return false;
    return IsLoopbackBindAddress(addr);
}
}
#endif
