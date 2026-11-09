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

#ifndef TRINITYCORE_CLASSIC_OPCODES_H
#define TRINITYCORE_CLASSIC_OPCODES_H

#include "Define.h"
#include <vector>

/// Opcode translation between the WoW Classic 1.60.1 client (build 70009) and the retail opcode numbers used by the core.
/// Entries are added as they are discovered; opcodes without an entry are passed through unchanged.
namespace ClassicOpcodes
{
    /// client number -> core OpcodeClient value
    TC_GAME_API uint32 TranslateClientOpcode(uint32 classicOpcode);

    /// the same in the numbering of client 70245 (the tables); the one above adds the newer builds' group shift
    TC_GAME_API uint32 TranslateClientOpcode70245(uint32 classicOpcode);

    /// IE-M2 Blocco5: server-side TranslateServerOpcode / TranslateServerOpcode70245 / IsServerOpcodeBlocked / GetServerStubPayload
    /// were removed. Outbound wire id, block and stub policy come only from the ProtocolAdapter dense SmsgAdapterTable
    /// (kFlagBlocked / kFlagStub), see docs/m2/M2_BLOCK5.md.

    /// client 70291 or later (Classic.OpcodeGroupShift / realm build): also selects that build's changed packet and update field layouts
    TC_GAME_API bool IsBuild70291OrLater();

    /// client 70425 or later (realm build): selects that build's update field layouts. Client 70450 keeps them: same game data
    /// (only DLLs changed in CASC), same client strings except build number and date, same RTTI types as 70425
    TC_GAME_API bool IsBuild70425OrLater();
}

#endif // TRINITYCORE_CLASSIC_OPCODES_H
