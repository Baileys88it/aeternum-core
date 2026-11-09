/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Aeternum IE-M2 Blocco4 - outbound (SMSG) translation helper (hot-path half, header-only, pure).
 *
 * WorldSocket::SendPacket / WritePacketToBuffer / CompressPacket take the client wire id ONLY from the
 * ProtocolAdapter dense SmsgAdapterTable (= AdapterOpcodeTable::smsgLogicalToWire, built once at boot from
 * opcodes.json; no second copy). Blocco5: the legacy server translator and block/stub helpers are removed from the
 * codebase; block and stub policy come from the same entry flags (kFlagBlocked / kFlagStub).
 *
 * Actions:
 *   Drop (table not loaded)      fail closed: no lookup, no counter, no log, never a legacy / raw-logical send
 *   Drop (logical not mapped)    outbound counter + rate-limited ERROR via table sink (unknown_policy.json,
 *                                OUTBOUND window, separate from inbound); the packet is NOT sent
 *   Blocked (kFlagBlocked)       Blocco5: never sent; slot hit counter only, no unknown counter, no log
 *   Send                         wire = SmsgAdapterTable[logical]; kFlagStub -> stub = fixed placeholder payload
 *
 * Structural rules (contrib/aeternum/ci/check_adapter_no_map.ps1, hot-path list): no associative containers, no heap.
 */
#ifndef AETERNUM_PROTOCOL_ADAPTER_OUTBOUND_HPP
#define AETERNUM_PROTOCOL_ADAPTER_OUTBOUND_HPP

#include "aeternum/protocol_adapter/opcode_table.hpp"

#include <cstdint>

namespace aeternum::protocol_adapter
{
enum class OutboundAction : std::uint8_t
{
    Drop = 0,
    Send = 1,
    Blocked = 2  // Blocco5: kFlagBlocked - known for the client but must never be sent
};

struct OutboundResult
{
    OutboundAction action = OutboundAction::Drop;
    OpcodeClass cls = OpcodeClass::Unknown;
    std::uint32_t wire = kInvalidOpcode;  // set when action == Send
    std::uint16_t flags = 0;
    StubPayload const* stub = nullptr;  // Blocco5: kFlagStub -> send this payload instead of the packet body
};

/**
 * Translate one outbound logical (core) SMSG id to the target client wire id. noexcept, O(1), zero heap,
 * zero I/O (only side effects: per-slot hit counter, outbound unknown counters, rate-limited sink).
 */
inline OutboundResult TranslateOutboundServerLogical(AdapterOpcodeTable& table, std::uint32_t logical,
                                                     std::uint64_t nowMs) noexcept
{
    OutboundResult r;
    if (!table.Loaded())
        return r;

    OpcodeClassification const c = table.ClassifyServerLogical(logical, nowMs);
    r.cls = c.cls;
    r.flags = c.flags;
    if (c.decision == OpcodeDecision::Dispatch && c.translated <= kOpcodeKeyMax)
    {
        if (c.flags & kFlagBlocked)
        {
            r.action = OutboundAction::Blocked;
            return r;
        }
        if (c.flags & kFlagStub)
        {
            r.stub = table.FindStubPayload(logical);
            if (!r.stub)
                return r;  // fail closed (the loader guarantees a payload for every kFlagStub row)
        }
        r.action = OutboundAction::Send;
        r.wire = c.translated;
    }
    return r;
}

constexpr bool IsBlocked(OutboundResult const& r) noexcept
{
    return r.action == OutboundAction::Blocked;
}

constexpr bool ShouldSend(OutboundResult const& r) noexcept
{
    return r.action == OutboundAction::Send;
}

/** Peek-only logical -> wire for the write stage (no counters, no sink, no I/O). */
inline bool PeekOutboundWire(AdapterOpcodeTable const& table, std::uint32_t logical, std::uint32_t& outWire) noexcept
{
    outWire = kInvalidOpcode;
    if (!table.Loaded())
        return false;
    OpcodeEntry const* e = table.SmsgAdapterTable().Find(logical);
    if (!e || e->cls == OpcodeClass::IgnoredKnown || (e->flags & kFlagBlocked) || e->value > kOpcodeKeyMax)
        return false;
    outWire = e->value;
    return true;
}
} // namespace aeternum::protocol_adapter

#endif /* AETERNUM_PROTOCOL_ADAPTER_OUTBOUND_HPP */
