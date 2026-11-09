/*
 * Aeternum Engine public API — protocol opcode boundary (SDK 1.0 / IE-M1).
 * Real consumer today: Forever ClassicOpcodeRemap + client profile 1.60.1.70790.
 * Headers only: no gameplay behavior change. Trinity types forbidden.
 */
#ifndef AETERNUM_API_PROTOCOL_OPCODE_HPP
#define AETERNUM_API_PROTOCOL_OPCODE_HPP

#include "aeternum/api/status.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace aeternum::protocol
{
struct BuildId
{
    std::uint32_t value = 0;
};

/** Opaque wire opcode as seen on the Forever client wire. */
struct WireOpcode
{
    std::uint32_t raw = 0;
};

/** Core/internal opcode after ExtraInserts remap (still not a Trinity type). */
struct CoreOpcode
{
    std::uint32_t raw = 0;
};

enum class HandlerStatus : unsigned char
{
    Unhandled = 0,
    Ignored,
    Never,
    Authed,
    LoggedIn,
    Transfer,
    RecentlyLoggedIn
};

/**
 * Registry view for one Forever client profile.
 * Implementations live in adapters; this header must stay Trinity-free.
 */
class IOpcodeRegistry
{
public:
    virtual ~IOpcodeRegistry() = default;
    virtual BuildId Build() const = 0;
    virtual std::string_view ProfileId() const = 0;
    virtual Result<CoreOpcode> WireToCore(WireOpcode wire) const = 0;
    virtual Result<WireOpcode> CoreToWire(CoreOpcode core) const = 0;
    virtual HandlerStatus HandlerOf(CoreOpcode core) const = 0;
};

/** Opaque decoded message — payload remains bytes until typed codecs land. */
struct DecodedMessage
{
    CoreOpcode opcode{};
    std::vector<std::byte> payload;
};

class IPacketCodec
{
public:
    virtual ~IPacketCodec() = default;
    virtual Result<DecodedMessage> Decode(WireOpcode wire, std::span<std::byte const> bytes) const = 0;
    virtual Result<std::vector<std::byte>> Encode(DecodedMessage const& msg) const = 0;
};
}

#endif
