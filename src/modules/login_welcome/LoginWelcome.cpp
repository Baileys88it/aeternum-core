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

#include "ChatPackets.h"
#include "Config.h"
// Player.h names SkillLineEntry. The game precompiled header usually declares it first.
#include "DB2Structure.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"

#include <string>

namespace
{
// SMSG_PRINT_NOTIFICATION (WorldSession::SendNotification, the .notify command) is the small top banner.
// CHAT_MSG_SYSTEM (Motd, ChatHandler::SendSysMessage) is the chat box.
// CHAT_MSG_RAID_BOSS_EMOTE is the large centered announcement (RaidBossEmote frame). Profile 1.60.1.70790
// maps SMSG_CHAT and does not block or stub it. The same typed packet is what Unit::TextEmote(..., true) builds.
void SendCenteredWelcome(Player* player)
{
    // quiet: a missing key uses the English default and must not warn on every character login.
    std::string text = sConfigMgr->GetStringDefault("Aeternum.Welcome.Message", "Welcome to Aeternum.", true);
    if (text.empty())
        return;

    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_RAID_BOSS_EMOTE, LANG_UNIVERSAL, nullptr, nullptr, text);
    player->SendDirectMessage(packet.Write());
}

class LoginWelcomeScript final : public PlayerScript
{
public:
    LoginWelcomeScript() : PlayerScript("LoginWelcomeScript") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        // HandlePlayerLogin runs while the loading screen is still up. Mark the character and send later.
        if (player)
            _pending.insert(player->GetGUID());
    }

    void OnLoadingScreenDismissed(Player* player) override
    {
        if (!player || !player->IsInWorld() || !player->GetSession())
            return;

        // First dismiss after this character login only. Later loading screens (teleport) do not repeat it.
        if (_pending.erase(player->GetGUID()) == 0)
            return;

        SendCenteredWelcome(player);
    }

    void OnLogout(Player* player) override
    {
        if (player)
            _pending.erase(player->GetGUID());
    }

private:
    // World-thread hooks only (PROCESS_THREADUNSAFE login and loading-screen opcodes).
    GuidUnorderedSet _pending;
};
}

void AddSC_login_welcome()
{
    new LoginWelcomeScript();
}
