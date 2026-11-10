/*
 * Aeternum Engine public API — admin HTTP surface (SDK 1.0).
 * Real consumer: src/modules/admin_monitoring (default OFF, loopback only).
 */
#ifndef AETERNUM_API_OPS_ADMIN_HPP
#define AETERNUM_API_OPS_ADMIN_HPP

#include "aeternum/api/status.hpp"

#include <cstdint>
#include <string_view>

namespace aeternum::ops
{
enum class AdminRole : unsigned char
{
    Anonymous = 0,
    ReadOnly,
    Operator
};

struct AdminBind
{
    std::string_view address; // must be loopback in production policy
    std::uint16_t port = 0;
};

class IAdminService
{
public:
    virtual ~IAdminService() = default;
    virtual bool Enabled() const = 0;
    virtual AdminBind Bind() const = 0;
    virtual Status Start() = 0;
    virtual void Stop() = 0;
    /** Drain is operator-only and disabled by default in conf. */
    virtual Status RequestDrain(std::uint32_t delaySeconds) = 0;
};
}

#endif
