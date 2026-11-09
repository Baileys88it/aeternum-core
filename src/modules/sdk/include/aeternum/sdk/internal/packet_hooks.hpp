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
 * Aeternum IE-M3 Blocco2 - dense packet hook table (INTERNAL; core <-> native runtime). HOT PATH.
 *
 * Included by the game (WorldSocket::ReadDataHandler inbound DROP + WorldSocket::SendPacket outbound MUTATE) and by the native runtime.
 * No Trinity types, no heap, no associative container (lint: contrib/aeternum/ci/check_adapter_no_map.ps1).
 *
 *   - 1984 CMSG + 1671 SMSG dense slots (same indices as Opcodes.h GetOpcodeArrayIndex, verified by Catch2),
 *     <= 4 hooks per opcode, one atomic `count` per opcode: fast path is ONE acquire load when count == 0.
 *   - Storage is a single constinit object (static storage, zero dynamic allocation, ~260 KiB).
 *   - Lifecycle: hooks are STAGED only while a module's on_init runs (boot thread), PUBLISHED (count release-store)
 *     once at the end of the native boot, then the table is FROZEN until shutdown. Hooks of a module that does not
 *     reach Active are retracted before publication.
 *   - Dispatch goes through `dispatch` (set by the native runtime): every entry is re-checked against
 *     {epoch, module slot, generation, slot state Active} and called through the F1 SEH / signal guard.
 *   - Forbidden: socket-level CMSG handled inside WorldSocket. M6 permits movement subscriptions.
 *
 * Single instance across translation units relies on C++17 inline variables in ONE image (worldserver.exe /
 * tests.exe, WITH_DYNAMIC_LINKING=OFF). Not designed for a shared `game` library.
 */
#ifndef AETERNUM_SDK_INTERNAL_PACKET_HOOKS_HPP
#define AETERNUM_SDK_INTERNAL_PACKET_HOOKS_HPP

#include "aeternum/abi/AeternumABI.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace aeternum::sdk::internal::hooks
{

inline constexpr std::uint32_t kNumCmsgSlots = 1984;     // == NUM_CMSG_OPCODES (Opcodes.h)
inline constexpr std::uint32_t kNumSmsgSlots = 1671;     // == NUM_SMSG_OPCODES (Opcodes.h)
inline constexpr std::uint32_t kMaxHooksPerOpcode = 4;
inline constexpr std::uint32_t kMaxNativeModules = 32;   // module slot index fits in 8 bits of the handle

/** Dense CMSG index; same mapping as GetOpcodeArrayIndex(OpcodeClient) for every in-range id. -1 = no slot
 *  (where Opcodes.h yields an out-of-range positive value, see below). */
inline constexpr std::ptrdiff_t CmsgIndex(std::uint32_t opcode) noexcept
{
    // signed on purpose: in Opcodes.h `idInGroup` is uint32, so `: -1` becomes 0xFFFFFFFF and the "miss"
    // is a large POSITIVE ptrdiff_t (Trinity callers rely on their `< ssize(table)` check). Here -1 is a real -1.
    std::ptrdiff_t const idInGroup = static_cast<std::ptrdiff_t>(opcode & 0xFFFFu);
    switch (opcode >> 16)
    {
        case 0x2A: return idInGroup <  55 ? idInGroup +    0 : -1;
        case 0x2C: return idInGroup <  51 ? idInGroup +   55 : -1;
        case 0x2D: return idInGroup <   3 ? idInGroup +  106 : -1;
        case 0x2E: return idInGroup <  39 ? idInGroup +  109 : -1;
        case 0x2F: return idInGroup <   2 ? idInGroup +  148 : -1;
        case 0x30: return idInGroup <   2 ? idInGroup +  150 : -1;
        case 0x31: return idInGroup <   9 ? idInGroup +  152 : -1;
        case 0x32: return idInGroup <  20 ? idInGroup +  161 : -1;
        case 0x33: return idInGroup <   8 ? idInGroup +  181 : -1;
        case 0x34: return idInGroup <   9 ? idInGroup +  189 : -1;
        case 0x35: return idInGroup <  39 ? idInGroup +  198 : -1;
        case 0x37: return idInGroup <   9 ? idInGroup +  237 : -1;
        case 0x39: return idInGroup <   8 ? idInGroup +  246 : -1;
        case 0x3A: return idInGroup <  16 ? idInGroup +  254 : -1;
        case 0x3B: return idInGroup <  16 ? idInGroup +  270 : -1;
        case 0x3D: return idInGroup < 786 ? idInGroup +  286 : -1;
        case 0x3E: return idInGroup < 311 ? idInGroup + 1072 : -1;
        case 0x3F: return idInGroup <   1 ? idInGroup + 1383 : -1;
        case 0x40: return idInGroup <  10 ? idInGroup + 1384 : -1;
        case 0x41: return idInGroup < 137 ? idInGroup + 1394 : -1;
        case 0x43: return idInGroup < 435 ? idInGroup + 1531 : -1;
        case 0x44: return idInGroup <  18 ? idInGroup + 1966 : -1;
        default: return -1;
    }
}

/** Dense SMSG index; same mapping as GetOpcodeArrayIndex(OpcodeServer) for every in-range id. -1 = no slot. */
inline constexpr std::ptrdiff_t SmsgIndex(std::uint32_t opcode) noexcept
{
    // signed on purpose: in Opcodes.h `idInGroup` is uint32, so `: -1` becomes 0xFFFFFFFF and the "miss"
    // is a large POSITIVE ptrdiff_t (Trinity callers rely on their `< ssize(table)` check). Here -1 is a real -1.
    std::ptrdiff_t const idInGroup = static_cast<std::ptrdiff_t>(opcode & 0xFFFFu);
    switch (opcode >> 16)
    {
        case 0x45: return idInGroup < 903 ? idInGroup +    0 : -1;
        case 0x46: return idInGroup <   5 ? idInGroup +  903 : -1;
        case 0x49: return idInGroup <  20 ? idInGroup +  908 : -1;
        case 0x4A: return idInGroup <  37 ? idInGroup +  928 : -1;
        case 0x4B: return idInGroup <  49 ? idInGroup +  965 : -1;
        case 0x4C: return idInGroup <  15 ? idInGroup + 1014 : -1;
        case 0x4D: return idInGroup <  12 ? idInGroup + 1029 : -1;
        case 0x4F: return idInGroup <  82 ? idInGroup + 1041 : -1;
        case 0x51: return idInGroup <  72 ? idInGroup + 1123 : -1;
        case 0x53: return idInGroup <   2 ? idInGroup + 1195 : -1;
        case 0x54: return idInGroup <   8 ? idInGroup + 1197 : -1;
        case 0x55: return idInGroup <  13 ? idInGroup + 1205 : -1;
        case 0x56: return idInGroup <   8 ? idInGroup + 1218 : -1;
        case 0x57: return idInGroup <   8 ? idInGroup + 1226 : -1;
        case 0x58: return idInGroup <  37 ? idInGroup + 1234 : -1;
        case 0x59: return idInGroup <  10 ? idInGroup + 1271 : -1;
        case 0x5A: return idInGroup <  35 ? idInGroup + 1281 : -1;
        case 0x5C: return idInGroup <   1 ? idInGroup + 1316 : -1;
        case 0x5E: return idInGroup < 130 ? idInGroup + 1317 : -1;
        case 0x5F: return idInGroup <   6 ? idInGroup + 1447 : -1;
        case 0x60: return idInGroup <  20 ? idInGroup + 1453 : -1;
        case 0x62: return idInGroup <   1 ? idInGroup + 1473 : -1;
        case 0x63: return idInGroup <   8 ? idInGroup + 1474 : -1;
        case 0x64: return idInGroup <  52 ? idInGroup + 1482 : -1;
        case 0x65: return idInGroup <  41 ? idInGroup + 1534 : -1;
        case 0x67: return idInGroup <  87 ? idInGroup + 1575 : -1;
        case 0x68: return idInGroup <   8 ? idInGroup + 1662 : -1;
        case 0x6A: return idInGroup <   1 ? idInGroup + 1670 : -1;
        default: return -1;
    }
}

/** Movement group = whole opcode group (D4, maintainer decision 4): CMSG 0x41xxxx (MovementHandler: CMSG_MOVE_*, time
 *  sync, active mover, missile trajectory), SMSG 0x5Exxxx (SMSG_MOVE_*, SMSG_ON_MONSTER_MOVE, spline/time sync). */
inline constexpr std::uint32_t kCmsgMovementGroup = 0x41;
inline constexpr std::uint32_t kSmsgMovementGroup = 0x5E;

inline constexpr bool IsMovementOpcode(std::uint32_t direction, std::uint32_t opcode) noexcept
{
    return direction == AETERNUM_PACKET_DIR_CMSG ? (opcode >> 16) == kCmsgMovementGroup
                                                 : (opcode >> 16) == kSmsgMovementGroup;
}

/** CMSG consumed inside WorldSocket::ReadDataHandler (never reach WorldSession::Update): not hookable. */
inline constexpr std::uint32_t kSocketLevelCmsg[] = {
    0x440006u, // CMSG_PING
    0x440001u, // CMSG_AUTH_SESSION
    0x440003u, // CMSG_AUTH_CONTINUED_SESSION
    0x4300ABu, // CMSG_KEEP_ALIVE
    0x440007u, // CMSG_LOG_DISCONNECT
    0x440009u, // CMSG_ENABLE_NAGLE
    0x430000u, // CMSG_CONNECT_TO_FAILED
    0x440005u, // CMSG_ENTER_ENCRYPTED_MODE_ACK
};

inline constexpr bool IsSocketLevelOpcode(std::uint32_t direction, std::uint32_t opcode) noexcept
{
    if (direction != AETERNUM_PACKET_DIR_CMSG)
        return false;
    for (std::uint32_t v : kSocketLevelCmsg)
        if (v == opcode)
            return true;
    return false;
}

struct HookEntry
{
    AeternumPacketHookFn fn = nullptr;
    std::uint32_t moduleSlot = 0;
    std::uint32_t generation = 0;
    std::uint32_t epoch = 0;
};

struct OpcodeHooks
{
    std::atomic<std::uint32_t> count{ 0 }; // PUBLISHED entries (readers: acquire)
    std::uint32_t staged = 0;              // boot thread only
    HookEntry entries[kMaxHooksPerOpcode]{};
};

/** IE-M5 Blocco1: aggregate result of one DispatchPacket call (stack / caller-owned).
 *  mutateData is a claimed HostAlloc user pointer (Policy F2); caller MUST ReleaseMutationBytes when done
 *  (HostDisposeClaimed). Zero CRT heap in NativeHooks hot path. drop==true means a CMSG must not reach the legacy handler. */
struct PacketDispatchOutcome
{
    bool drop = false;
    std::uint8_t* mutateData = nullptr;
    std::uint32_t mutateSize = 0;
};

using PacketHookDispatchFn = void (*)(std::uint32_t direction, std::uint32_t index, AeternumPacketView const* view,
                                      PacketDispatchOutcome* outcome) noexcept;

struct PacketHookTable
{
    std::atomic<PacketHookDispatchFn> dispatch{ nullptr };
    std::atomic<std::uint32_t> epoch{ 0 };
    std::atomic<std::uint32_t> frozen{ 0 };
    std::atomic<std::uint32_t> published{ 0 }; // total published entries (diagnostics)
    OpcodeHooks cmsg[kNumCmsgSlots]{};
    OpcodeHooks smsg[kNumSmsgSlots]{};
};

inline constinit PacketHookTable g_packetHooks{};

inline OpcodeHooks* SlotsFor(std::uint32_t direction) noexcept
{
    return direction == AETERNUM_PACKET_DIR_CMSG ? g_packetHooks.cmsg : g_packetHooks.smsg;
}

/** Free a mutation CRT copy produced by DispatchOutbound / DispatchPacket (implemented in NativeHooks.cpp). */
void AeternumReleaseMutationBytes(PacketDispatchOutcome& o) noexcept;

inline void ReleaseMutationBytes(PacketDispatchOutcome& o) noexcept
{
    AeternumReleaseMutationBytes(o);
}

/** Shared fast path. Zero heap when count==0. Mutation path claims a HostAlloc block (no CRT malloc here). */
inline PacketDispatchOutcome DispatchPacket(std::uint32_t direction, std::uint32_t opcode, void const* data,
                                            std::size_t size, void* session, std::uint32_t sessionId = UINT32_MAX) noexcept
{
    PacketDispatchOutcome outcome{};
    std::ptrdiff_t const idx = direction == AETERNUM_PACKET_DIR_CMSG ? CmsgIndex(opcode) : SmsgIndex(opcode);
    if (idx < 0 || idx >= static_cast<std::ptrdiff_t>(direction == AETERNUM_PACKET_DIR_CMSG ? kNumCmsgSlots : kNumSmsgSlots))
        return outcome; // belt and braces: never index past the dense table
    if (SlotsFor(direction)[idx].count.load(std::memory_order_acquire) == 0)
        return outcome; // fast path: nobody subscribed (always the case with the native runtime OFF)
    PacketHookDispatchFn const fn = g_packetHooks.dispatch.load(std::memory_order_acquire);
    if (!fn)
        return outcome;
    AeternumPacketMutation mutation{};
    mutation.struct_size = static_cast<std::uint32_t>(sizeof(AeternumPacketMutation));
    AeternumPacketView view;
    view.struct_size = static_cast<std::uint32_t>(sizeof(AeternumPacketView));
    view.direction = direction;
    view.opcode = opcode;
    view.size = size > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<std::uint32_t>(size);
    view.data = static_cast<std::uint8_t const*>(data);
    view.session = static_cast<AeternumSessionHandle>(session);
    view.mutation = (direction == AETERNUM_PACKET_DIR_SMSG) ? &mutation : nullptr;
    view.session_id = sessionId;
    fn(direction, static_cast<std::uint32_t>(idx), &view, &outcome);
    return outcome;
}

/** Inbound CMSG: returns true when a hook DROP must discard the packet before QueuePacket. */
inline bool DispatchInbound(std::uint32_t opcode, void const* data, std::size_t size, void* session, std::uint32_t sessionId = UINT32_MAX) noexcept
{
    return DispatchPacket(AETERNUM_PACKET_DIR_CMSG, opcode, data, size, session, sessionId).drop;
}

/** Outbound SMSG: may return a validated mutation copy in outcome.mutateData (caller frees). */
inline PacketDispatchOutcome DispatchOutbound(std::uint32_t opcode, void const* data, std::size_t size,
                                              void* session, std::uint32_t sessionId = UINT32_MAX) noexcept
{
    return DispatchPacket(AETERNUM_PACKET_DIR_SMSG, opcode, data, size, session, sessionId);
}

} // namespace aeternum::sdk::internal::hooks

#endif /* AETERNUM_SDK_INTERNAL_PACKET_HOOKS_HPP */
