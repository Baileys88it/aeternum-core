/*
 * Aeternum Engine public API — Forever client profile boundary (SDK 1.0 / IE-M1).
 * Real consumer: forever/client_profiles ACTIVE gate + ExtraInserts auto-apply.
 */
#ifndef AETERNUM_API_PROTOCOL_PROFILE_HPP
#define AETERNUM_API_PROTOCOL_PROFILE_HPP

#include "aeternum/api/protocol/opcode.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace aeternum::protocol
{
class IClientProfile
{
public:
    virtual ~IClientProfile() = default;
    virtual std::string_view ProfileId() const = 0;
    virtual BuildId Build() const = 0;
    virtual std::vector<std::uint32_t> ClientExtraInserts() const = 0;
    virtual std::vector<std::uint32_t> ServerExtraInserts() const = 0;
    virtual Status ReloadFromDisk() = 0;
};
}

#endif
