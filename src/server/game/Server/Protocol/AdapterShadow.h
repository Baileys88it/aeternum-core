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

#ifndef AETERNUM_ADAPTER_SHADOW_H
#define AETERNUM_ADAPTER_SHADOW_H

#include "Define.h"

namespace aeternum::protocol_adapter { class AdapterOpcodeTable; }

/// Aeternum IE-M2 Blocco3: ProtocolAdapter primary inbound dispatch for WorldSocket::ReadDataHandler.
/// ClassicOpcodes::TranslateClientOpcode is no longer on the inbound critical path; see docs/m2/M2_BLOCK3.md.
/// IE-M2 Blocco4: outbound SMSG logical -> wire via the dense SmsgAdapterTable; ClassicOpcodes::TranslateServerOpcode
/// is no longer on the send critical path; see docs/m2/M2_BLOCK4.md.
/// IE-M2 Blocco5: the legacy server translator and the block/stub helpers are removed from the codebase; SendPacket gets
/// wire id, block and stub policy from ONE dense-table entry (kFlagBlocked / kFlagStub); see docs/m2/M2_BLOCK5.md.
namespace Aeternum::ProtocolAdapterShadow
{
    // C++ test dependency seam only: redirects this thread to a separately owned REAL profile table.
    // Never resets/mutates the boot table or its freeze guard; not exposed in the C ABI.
    class TC_GAME_API ScopedTableForTest final
    {
    public:
        explicit ScopedTableForTest(aeternum::protocol_adapter::AdapterOpcodeTable& table) noexcept;
        ~ScopedTableForTest();
        ScopedTableForTest(ScopedTableForTest const&) = delete;
        ScopedTableForTest& operator=(ScopedTableForTest const&) = delete;
    private:
        aeternum::protocol_adapter::AdapterOpcodeTable* previous_;
    };

    /// Boot-only: load forever/client_profiles/<Classic.ActiveClientProfile>/ (manifest + SHA-256 + opcodes.json) into the
    /// process-wide dense table and hook the unknown-opcode rate limiter to TC_LOG_ERROR. Runs once (config reload = no-op).
    /// On any failure logs FATAL and leaves the table unloaded (fail closed: every inbound packet is dropped, never legacy).
    TC_GAME_API void LoadAtStartup();

    TC_GAME_API bool IsActive();

    /// Per packet. noexcept, O(1), no heap unless a (rate-limited) ERROR line is emitted for unknowns.
    /// Returns true and writes the adapter logical id when the packet must be dispatched.
    /// Returns false to drop at source (ignored_known / unknown / OOR / table not loaded) — stop the chain, no queue.
    TC_GAME_API bool TranslateInbound(uint32 rawWire, uint32& outLogical) noexcept;

    /// Peek-only wire->logical for the oversized CMSG_HOTFIX_REQUEST header gate. No counters / no sink.
    TC_GAME_API bool PeekDispatchLogical(uint32 rawWire, uint32& outLogical) noexcept;

    /// IE-M2 Blocco5: outbound verdict for one packet, read from ONE SmsgAdapterTable entry.
    struct OutboundPacket
    {
        uint32 wire = 0;                   // client wire id (valid when TranslateOutbound returned true)
        bool blocked = false;              // kFlagBlocked: known SMSG that must never reach the client
        bool stub = false;                 // kFlagStub: send stubData/stubSize instead of the packet body
        uint8 const* stubData = nullptr;   // boot-loaded, immutable (opcodes.json smsg_stub_payloads)
        uint32 stubSize = 0;
    };

    /// IE-M2 Blocco4/5, per outbound packet (WorldSocket::SendPacket). noexcept, O(1), no heap unless a (rate-limited) ERROR
    /// line is emitted. Returns true when the packet may be sent (out.wire set; out.stub = send the placeholder payload).
    /// Returns false = do NOT send: out.blocked (kFlagBlocked) or logical id not mapped / table not loaded.
    /// Never ClassicOpcodes, never the raw logical id.
    TC_GAME_API bool TranslateOutbound(uint32 logical, OutboundPacket& out) noexcept;

    /// IE-M2 Blocco4: peek-only logical -> wire for the write stage (WritePacketToBuffer / CompressPacket). No counters / no sink.
    TC_GAME_API bool PeekOutboundWire(uint32 logical, uint32& outWire) noexcept;
}

#endif // AETERNUM_ADAPTER_SHADOW_H