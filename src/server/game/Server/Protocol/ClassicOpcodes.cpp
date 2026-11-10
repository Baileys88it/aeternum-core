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

#include "ClassicOpcodes.h"
#include "Config.h"
#include "Opcodes.h"
#include "Forever/ClassicOpcodeRemap.h"
#include "Forever/ClientProfile.h"
#include "Forever/ProtocolSessionBridge.h"
#include "Realm.h"
#include "RealmList.h"
#include "Log.h"
#include <algorithm>
#include <unordered_map>

namespace
{
// Opcodes are 0xGGIIII: group byte + index. The connection-level groups appear shifted by one in Classic:
//  - client group 0x45 (retail 0x44): 0x450007 carried a single uint32 (3) = CMSG_LOG_DISCONNECT (retail 0x440007)
//  - server group 0x4D (retail 0x4C, assumed by symmetry)
constexpr uint32 ClassicClientConnectionGroup = 0x45;
constexpr uint32 RetailClientConnectionGroup = 0x44;
constexpr uint32 ClassicServerConnectionGroup = 0x4D;
constexpr uint32 RetailServerConnectionGroup = 0x4C;

// explicit Classic 1.60.1.70009 client opcode -> retail core opcode overrides
std::unordered_map<uint32, uint32> const ClientOpcodes =
{
#include "ClassicClientOpcodes.inc"
};

// IE-M2 Blocco5: the server-side tables (ServerOpcodes overrides, BlockedServerOpcodes, StubServerPayloads) were moved to
// contrib/aeternum/protocol/gen_opcode_table.py; at runtime they exist only as ProtocolAdapter dense-table flags
// (kFlagBlocked / kFlagStub + opcodes.json smsg_stub_payloads). TranslateServerOpcode(70245) was removed.

constexpr uint32 OpcodeGroup(uint32 opcode) { return opcode >> 16; }
constexpr uint32 WithOpcodeGroup(uint32 opcode, uint32 group) { return (group << 16) | (opcode & 0xFFFF); }
}

// Hypothesis under test: every opcode group is shifted by +1 in Classic (verified for the connection groups 0x44/0x4C)
constexpr uint32 FirstRetailClientGroup = 0x2A;
constexpr uint32 LastRetailClientGroup = RetailClientConnectionGroup;

// Client 1.60.1.70291 (2026-10-09), the tables above stay in 70245 numbers:
//  - client: only the connection group moved (CMSG_LOG_DISCONNECT 0x450007 -> 0x460007), CMSG_ENUM_CHARACTERS stayed 0x440014
//  - server: every group is one higher (SMSG_AUTH_CHALLENGE 0x4D0000 -> 0x4E0000; with the main group left at 0x46 the client ignores
//    AUTH_RESPONSE and never asks for the characters), and the main group got one more message: the official 70291 sniff has
//    70245 index 0x92 (BATTLE_PET_JOURNAL) unchanged and 0xAD (START_ELAPSED_TIMERS) at 0xAE, SET_TIME_ZONE_INFORMATION 0x124,
//    ACCOUNT_DATA_TIMES 0x1B6, SERVER_TIME_OFFSET 0x1C0, TUTORIAL_FLAGS 0x269, MIRROR_VARS 0x372, BATTLENET_RESPONSE 0x2B0;
//    the inserted message is index 0x98: the official 70338 sniff has PARTY_UPDATE (0x97) unchanged at 0x470097
// Classic.OpcodeGroupShift: -1 (default) = from the realm's game build (realmlist.gamebuild >= 70291 -> 1), or 0 / 1.
static uint32 ExtraGroupShift()
{
    static uint32 const shift = []() -> uint32
    {
        int32 configured = sConfigMgr->GetIntDefault("Classic.OpcodeGroupShift", -1);
        if (configured >= 0)
            return uint32(configured);
        std::shared_ptr<Realm const> realm = sRealmList->GetCurrentRealm();
        return realm && realm->Build >= 70291 ? 1 : 0;
    }();
    return shift;
}

// Client builds after 70707 (70755, 2026-11-04) can add messages to any opcode group, which moves every later index of that group
// up by one. Classic.ClientExtraInserts / Classic.ServerExtraInserts list the new build's full opcode numbers (0xGGIIII) of the added
// messages, comma separated; both empty = 70707 numbering. They are applied on top of the 70245/70291 translation below.

static std::vector<uint32> const& ClientExtraInserts()
{
    // M6: profile ExtraInserts auto-apply (when enabled) wins over conf lists.
    return Forever::EffectiveClientExtraInserts();
}

// new build number -> 70707 number (an added message itself has no 70707 number: returns InvalidClassicOpcode)
constexpr uint32 InvalidClassicOpcode = 0xFFFFFFFF;
static uint32 RemoveClientExtraInserts(uint32 classicOpcode)
{
    // IE-M1 step 3: session bridge (IOpcodeRegistry seat) — same Remap math, same inserts.
    if (Forever::ProtocolSession::Enabled())
        return Forever::ProtocolSession::RemoveClientExtraInserts(classicOpcode);
    return Forever::OpcodeRemap::RemoveClientExtraInserts(classicOpcode, ClientExtraInserts());
}

bool ClassicOpcodes::IsBuild70291OrLater()
{
    return ExtraGroupShift() != 0;
}

bool ClassicOpcodes::IsBuild70425OrLater()
{
    static bool const later = []()
    {
        std::shared_ptr<Realm const> realm = sRealmList->GetCurrentRealm();
        return realm && realm->Build >= 70425;
    }();
    return later;
}

uint32 ClassicOpcodes::TranslateClientOpcode(uint32 classicOpcode)
{
    classicOpcode = RemoveClientExtraInserts(classicOpcode);
    if (classicOpcode == InvalidClassicOpcode)
        return InvalidClassicOpcode;

    if (uint32 shift = ExtraGroupShift())
    {
        // client: the connection group (70245 0x45) moved to 0x46
        if (OpcodeGroup(classicOpcode) == ClassicClientConnectionGroup + shift)
            classicOpcode = WithOpcodeGroup(classicOpcode, ClassicClientConnectionGroup);
        // and the main client group 0x44 got one more message between 70245 index 0x13A and 0x150 (sniffs 2026-10-09: 0x13A
        // BATTLE_PAY_OPEN_CHECKOUT unchanged, 0x150 -> 0x151; RECENT_ALLY_REQUEST_DATA, ACCEPT_SOCIAL_CONTRACT, club finder are
        // above it). Classic.ClientMainGroupInsertIndex = the 70291 index of the new message, everything after it is one lower.
        static uint32 const clientInsertIndex = uint32(sConfigMgr->GetIntDefault("Classic.ClientMainGroupInsertIndex", 0x13B));
        if (OpcodeGroup(classicOpcode) == ClassicClientConnectionGroup - 1 && (classicOpcode & 0xFFFF) > clientInsertIndex)
            --classicOpcode;
    }

    return TranslateClientOpcode70245(classicOpcode);
}

uint32 ClassicOpcodes::TranslateClientOpcode70245(uint32 classicOpcode)
{
    if (auto itr = ClientOpcodes.find(classicOpcode); itr != ClientOpcodes.end())
        return itr->second;

    uint32 group = OpcodeGroup(classicOpcode);
    if (group >= FirstRetailClientGroup + 1 && group <= ClassicClientConnectionGroup)
        return WithOpcodeGroup(classicOpcode, group - 1);

    return classicOpcode;
}
