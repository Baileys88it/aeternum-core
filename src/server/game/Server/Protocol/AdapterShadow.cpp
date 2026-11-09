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

#include "AdapterShadow.h"
#include "ClientProfile.h"
#include "Log.h"
#include "aeternum/protocol_adapter/outbound.hpp"
#include "aeternum/protocol_adapter/profile_loader.hpp"
#include "aeternum/protocol_adapter/shadow.hpp"
#include <chrono>
#include <filesystem>

namespace pa = aeternum::protocol_adapter;

namespace
{
// ~0.3 MB static storage, filled once at boot before the network threads start, read-only afterwards (except counters)
pa::AdapterOpcodeTable g_table;
bool g_loadAttempted = false;
thread_local pa::AdapterOpcodeTable* g_fixtureTable = nullptr;
pa::AdapterOpcodeTable& CurrentTable() noexcept { return g_fixtureTable ? *g_fixtureTable : g_table; }

std::uint64_t NowMs() noexcept
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Unknown-opcode rate limiter -> native logger. Called by AdapterOpcodeTable only above threshold_per_window,
// at most max_errors_per_window times per window_ms (unknown_policy.json). Never called for ignored_known.
void NativeUnknownSink(void* /*ctx*/, std::uint32_t opcode, pa::OpcodeDirection dir, std::uint32_t countInWindow,
                       std::uint64_t unknownTotal) noexcept
{
    try
    {
        char line[pa::kShadowLineBytes];
        pa::FormatUnknownRateLimited(line, sizeof(line), opcode, dir, countInWindow, unknownTotal, g_table.GetUnknownPolicy());
        TC_LOG_ERROR("network.opcode", "{}", line);
    }
    catch (...)
    {
    }
}
}

namespace Aeternum::ProtocolAdapterShadow
{
ScopedTableForTest::ScopedTableForTest(pa::AdapterOpcodeTable& table) noexcept : previous_(g_fixtureTable)
{
    g_fixtureTable = &table;
}
ScopedTableForTest::~ScopedTableForTest() { g_fixtureTable = previous_; }

void LoadAtStartup()
{
    if (g_loadAttempted)
        return;  // boot-only: a config reload never rebuilds the table under live readers
    g_loadAttempted = true;

    std::string const profileId = Forever::GetConfiguredActiveProfileId();
    if (profileId.empty())
    {
        TC_LOG_FATAL("server.loading", "Aeternum ProtocolAdapter: FATAL - Classic.ActiveClientProfile empty; inbound+outbound fail-closed (no ClassicOpcodes fallback)");
        return;
    }
    std::string const root = Forever::ResolveProfileDataRoot();
    if (root.empty())
    {
        TC_LOG_FATAL("server.loading", "Aeternum ProtocolAdapter: FATAL - forever/client_profiles not found (set Classic.ProfileDataRoot); inbound+outbound fail-closed (no ClassicOpcodes fallback)");
        return;
    }

    std::filesystem::path const dir = std::filesystem::path(root) / Forever::ProfileDirectoryRel(profileId);
    pa::ProfileLoadReport rep;
    if (!pa::LoadProtocolProfile(dir, g_table, rep))
    {
        TC_LOG_FATAL("server.loading", "Aeternum ProtocolAdapter: FATAL - profile {} load failed ({}); inbound+outbound fail-closed (no ClassicOpcodes fallback)",
            profileId, rep.reason);
        return;
    }

    g_table.SetUnknownErrorSink(&NativeUnknownSink, nullptr);
    pa::UnknownPolicy const& p = g_table.GetUnknownPolicy();
    // WARN so it reaches Appender.Server (level Warn) like the other Forever profile boot lines
    TC_LOG_WARN("server.loading",
        "Aeternum ProtocolAdapter: ACTIVE profile {} build {} cmsg={} (aliases {}) smsg={} ignored_known={} sha256 files={} "
        "unknown policy window={}ms threshold={} max_errors={} (dispatch=adapter ProtocolAdapter)",
        rep.profileId, rep.clientBuild, rep.cmsgRows, rep.cmsgAliases, rep.smsgRows, rep.ignoredKnown, rep.verified.size(),
        p.windowMs, p.thresholdPerWindow, p.maxErrorsPerWindow);

    // IE-M2 Blocco5: the Blocco4 boot parity audit against the legacy server translator is gone (the translator was removed).
    // Send policy (kFlagBlocked / kFlagStub) comes from the same dense entries the send path reads.
    TC_LOG_WARN("server.loading",
        "Aeternum ProtocolAdapter: SMSG outbound ACTIVE SmsgAdapterTable entries={} slots={} blocked={} stub={} (send=adapter SmsgAdapterTable)",
        g_table.SmsgAdapterTable().Entries(), g_table.SmsgAdapterTable().SlotsUsed(), rep.smsgBlocked, g_table.StubPayloadCount());
}

bool IsActive()
{
    return CurrentTable().Loaded();
}

bool TranslateInbound(uint32 rawWire, uint32& outLogical) noexcept
{
    outLogical = 0;
    pa::InboundResult const r = pa::TranslateInboundClientWire(CurrentTable(), rawWire, NowMs());
    if (!pa::ShouldDispatch(r))
        return false;
    outLogical = pa::DispatchLogical(r);
    return true;
}

bool PeekDispatchLogical(uint32 rawWire, uint32& outLogical) noexcept
{
    return pa::PeekDispatchLogical(CurrentTable(), rawWire, outLogical);
}

bool TranslateOutbound(uint32 logical, OutboundPacket& out) noexcept
{
    out = OutboundPacket{};
    pa::OutboundResult const r = pa::TranslateOutboundServerLogical(CurrentTable(), logical, NowMs());
    if (pa::IsBlocked(r))
    {
        out.blocked = true;
        return false;
    }
    if (!pa::ShouldSend(r))
        return false;
    out.wire = r.wire;
    if (r.stub)
    {
        out.stub = true;
        out.stubData = r.stub->bytes.data();
        out.stubSize = r.stub->size;
    }
    return true;
}

bool PeekOutboundWire(uint32 logical, uint32& outWire) noexcept
{
    return pa::PeekOutboundWire(CurrentTable(), logical, outWire);
}
}