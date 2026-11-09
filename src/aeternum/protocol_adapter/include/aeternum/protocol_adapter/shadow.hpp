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
 * Aeternum IE-M2 Blocco3 - inbound dispatch helper (hot-path half, header-only, pure).
 *
 * WorldSocket::ReadDataHandler dispatches ONLY on the ProtocolAdapter O(1) result.
 * ClassicOpcodes::TranslateClientOpcode is not called on the inbound critical path.
 *
 * Actions:
 *   Drop (table not loaded)  fail closed: no lookup, no counter, no log, no legacy fallback
 *   Drop (IgnoredKnown)      counter only, NEVER a log (golden rule), stop the chain
 *   Drop (Unknown / OOR)     counter + rate-limited ERROR via table sink (unknown_policy.json); stop the chain
 *   Dispatch (Handled / KnownUnhandled)  use adapter logical id for the existing handler table
 *
 * Structural rules (contrib/aeternum/ci/check_adapter_no_map.ps1, hot-path list): no associative containers,
 * no heap. Formatting goes into caller-provided fixed buffers (snprintf).
 */
#ifndef AETERNUM_PROTOCOL_ADAPTER_SHADOW_HPP
#define AETERNUM_PROTOCOL_ADAPTER_SHADOW_HPP

#include "aeternum/protocol_adapter/opcode_table.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace aeternum::protocol_adapter
{
inline constexpr std::size_t kShadowLineBytes = 256u;

enum class InboundAction : std::uint8_t
{
    Drop = 0,
    Dispatch = 1
};

struct InboundResult
{
    InboundAction action = InboundAction::Drop;
    OpcodeClass cls = OpcodeClass::Unknown;
    std::uint32_t logical = kInvalidOpcode;  // set when action == Dispatch
};

/**
 * Translate one inbound client wire id for dispatch. noexcept, O(1), zero heap, zero I/O
 * (the only side effect besides counters is the table's rate-limited unknown sink).
 * Table not loaded -> Drop, no counters (fail closed, never ClassicOpcodes).
 */
inline InboundResult TranslateInboundClientWire(AdapterOpcodeTable& table, std::uint32_t rawWire,
                                                std::uint64_t nowMs) noexcept
{
    InboundResult r;
    if (!table.Loaded())
        return r;

    OpcodeClassification const c = table.ClassifyClientWire(rawWire, nowMs);
    r.cls = c.cls;
    r.logical = c.translated;
    if (c.decision == OpcodeDecision::Dispatch)
        r.action = InboundAction::Dispatch;
    return r;
}

constexpr bool ShouldDispatch(InboundResult const& r) noexcept
{
    return r.action == InboundAction::Dispatch;
}

constexpr std::uint32_t DispatchLogical(InboundResult const& r) noexcept
{
    return r.logical;
}

/** Peek-only wire->logical for the oversized-HOTFIX header gate. No counters, no sink, no I/O. */
inline bool PeekDispatchLogical(AdapterOpcodeTable const& table, std::uint32_t rawWire,
                                std::uint32_t& outLogical) noexcept
{
    outLogical = kInvalidOpcode;
    if (!table.Loaded())
        return false;
    OpcodeEntry const* e = table.cmsgWireToLogical.Find(rawWire);
    if (!e || e->cls == OpcodeClass::IgnoredKnown || e->cls == OpcodeClass::Unknown)
        return false;
    outLogical = e->value;
    return true;
}

/** Line handed to the native logger by the unknown-opcode sink (rate-limited by AdapterOpcodeTable). */
inline int FormatUnknownRateLimited(char* buf, std::size_t cap, std::uint32_t opcode, OpcodeDirection dir,
                                    std::uint32_t countInWindow, std::uint64_t unknownTotal, UnknownPolicy const& p) noexcept
{
    if (!buf || cap == 0)
        return 0;
    int const n = std::snprintf(buf, cap,
        "Aeternum ProtocolAdapter unknown opcode rate limit: %s=0x%06X count_in_window=%u (> threshold %u per %u ms) unknown_total=%llu",
        dir == OpcodeDirection::ClientWire ? "wire" : (dir == OpcodeDirection::ServerLogical ? "smsg_logical" : "logical"), opcode, countInWindow, p.thresholdPerWindow, p.windowMs,
        static_cast<unsigned long long>(unknownTotal));
    return n < 0 ? 0 : n;
}
} // namespace aeternum::protocol_adapter

#endif /* AETERNUM_PROTOCOL_ADAPTER_SHADOW_HPP */