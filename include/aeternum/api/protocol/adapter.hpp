/*
 * Aeternum Engine public API — protocol adapter boundary (IE-M2 Discovery skeleton).
 * Headers only: no gameplay / WorldSocket behavior change in Discovery.
 * Trinity pointer types forbidden. Real wiring lands in later M2 blocks.
 *
 * Consumers (planned): Forever ProtocolSession / ClassicOpcodes façade.
 * Related: opcode.hpp, profile.hpp, docs/m2/PROTOCOL_ADAPTER_DESIGN.md
 */
#ifndef AETERNUM_API_PROTOCOL_ADAPTER_HPP
#define AETERNUM_API_PROTOCOL_ADAPTER_HPP

#include "aeternum/api/protocol/opcode.hpp"
#include "aeternum/api/protocol/profile.hpp"
#include "aeternum/api/status.hpp"

#include <cstdint>
#include <string_view>

namespace aeternum::protocol
{
/** Dense logical index into preallocated handler / remap tables (not a wire id). */
struct LogicalOpcode
{
    std::uint16_t index = 0;
};

enum class UnknownOpcodePolicy : unsigned char
{
    LogOnceMetricDrop = 0,
    SilentIgnore,
    ForwardRaw // non-default; profile must opt in
};

struct AdapterBootInfo
{
    BuildId build{};
    std::string_view profileId{};
    UnknownOpcodePolicy unknownPolicy = UnknownOpcodePolicy::LogOnceMetricDrop;
    bool skipBuildAuthKeyCheck = false;
    std::uint32_t enterEncryptedModeRegionGroup = 0;
};

/**
 * Boot-time / process-wide adapter seat.
 * Hot-path methods must not allocate (P-c). Implementations live under Forever/.
 */
class IOpcodeAdapter
{
public:
    virtual ~IOpcodeAdapter() = default;

    virtual AdapterBootInfo Info() const = 0;

    virtual Result<CoreOpcode> WireToCore(WireOpcode wire) const = 0;
    virtual Result<WireOpcode> CoreToWire(CoreOpcode core) const = 0;

    /** Optional dense view when the implementation exposes a stable logical index. */
    virtual Result<LogicalOpcode> CoreToLogical(CoreOpcode core) const = 0;

    virtual HandlerStatus HandlerOf(CoreOpcode core) const = 0;
    virtual UnknownOpcodePolicy UnknownPolicy() const = 0;
};
}

#endif
