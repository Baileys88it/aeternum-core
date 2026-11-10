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

// Aeternum arena: 1v1, 2v2 and 3v3 on Mak'gora Arena (map 2995, BattlemasterList 1169) with a personal rating
// per bracket (Elo), a top 10, honor per win and end of season titles. Config: Aeternum.Arena.* (worldserver.conf).
// SQL: sql/optional/arena
//
// Levels: rated needs every member at Aeternum.Arena.Rated.Level (60). Skirmish is open from MinLevel (1) and only
// pairs two entries when max - min level of all the players of the match is <= Aeternum.Arena.Skirmish.MaxLevelDiff
// (3). Both rules are checked when queueing and at every matchmaking pass with the live levels: an entry that breaks
// them while queued (a level-up inside a skirmish group, a rated member below the level) leaves the queue.
//
// Flow: players queue with ".arena queue" or the Arena Master gossip. The matchmaker (world tick) pairs two entries
// of the same bracket and queue type, creates an unrated core Arena and sends everyone there as if they had accepted
// a battlemaster invite. The core Arena class does the rest (prep phase, win conditions, scoreboard, exit teleport
// to the entry point). arena_wf_makgora (map script of 2995) makes the two teams hostile, keeps players inside the
// leash radius and applies the rating when the match ends.
//
// Cancelled matches: an invited player who does not reach the arena within InviteTimeout (logout or disconnect while
// loading, failed teleport) or who leaves it before the gates open (Leave Arena, .arena leave, logout) has declined
// the match. The match is then cancelled for everybody, rated or skirmish: no victory, no defeat, no rating, no honor.
// The players already inside are sent back to where they queued from, the queue entries (players or groups) whose
// members all accepted go back into the queue with their original join time (same priority, same rating window),
// the entries with a member who declined are removed from the queue. Everybody gets a system message.
//
// Threads: commands, gossip, logout and the map script can run in map threads, so the queue, the ratings and the
// pending rewards are behind one mutex. Teleports, arena creation and reward delivery only run in the world tick
// (WorldScript::OnUpdate, after the map threads are done).

#include "ScriptMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundScript.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Group.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"
#include "WorldSession.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace Trinity::ChatCommands;

namespace wfArena
{
    constexpr uint16 BATTLEMASTER_LIST_ID = 1169;   // Mak'gora Arena, map 2995 (BattlemasterListXMap in the client DB2)
    constexpr uint8 BRACKET_COUNT = 3;
    constexpr std::array<uint8, BRACKET_COUNT> TEAM_SIZES = { 1, 2, 3 };
    constexpr std::array<char const*, BRACKET_COUNT> BRACKET_NAMES = { "1v1", "2v2", "3v3" };
    constexpr uint32 LEADERBOARD_SIZE = 10;

    struct Settings
    {
        bool Enabled = false;
        uint32 UpdateInterval = 5000;
        uint32 MinLevel = 1;
        uint32 RatedLevel = 60;
        uint32 SkirmishMaxLevelDiff = 3;
        int32 StartRating = 1500;
        float K = 32.0f;
        uint32 WindowInitial = 100;
        uint32 WindowStep = 50;
        uint32 WindowStepSeconds = 30;
        uint32 WindowMax = 1000;
        float LeashRadius = 60.0f;
        uint32 InviteTimeout = 60;
        uint32 HonorRatedWin = 50;
        uint32 HonorUnratedWin = 15;
        uint32 HonorLoss = 0;
        uint32 SeasonTopN = 3;
        uint32 SeasonMinGames = 10;
        uint32 TitleRank1 = 1352;
        uint32 TitleTopN = 1330;
    };

    struct Rating
    {
        int32 Value = 0;
        uint32 Wins = 0;
        uint32 Losses = 0;
    };

    struct QueueEntry
    {
        ObjectGuid Leader;
        std::vector<ObjectGuid> Members;
        uint8 Bracket = 0;
        bool Rated = true;
        int32 Rating = 0;               // members average when they queued
        time_t JoinTime = 0;
        time_t RequeuedAt = 0;          // put back after a cancelled match: members may still be leaving the arena
    };

    struct Match
    {
        uint8 Bracket = 0;
        bool Rated = true;
        BattlegroundQueueTypeId QueueId = BATTLEGROUND_QUEUE_NONE;
        std::array<std::vector<ObjectGuid>, 2> Teams;   // [TEAM_ALLIANCE] gold, [TEAM_HORDE] green
        std::vector<ObjectGuid> NotArrived;             // invited, not in the arena yet
        std::array<QueueEntry, 2> Entries;              // queue entries of both sides, put back on cancel
        std::vector<ObjectGuid> Declined;               // did not arrive in time or left before the start
        time_t Created = 0;
        bool Ended = false;                             // result applied, or cancel handled
        bool Cancelled = false;                         // a player declined: no result, handled in the world tick
    };

    struct Reward
    {
        uint32 Honor = 0;
        std::vector<uint32> Titles;
    };

    Settings cfg;
    bool dbLoaded = false;
    std::mutex lock;                                    // guards everything below
    uint32 season = 1;
    std::array<std::unordered_map<ObjectGuid::LowType, Rating>, BRACKET_COUNT> ratings;
    std::vector<QueueEntry> queue;                      // oldest JoinTime first
    std::unordered_map<uint32 /*instanceId*/, Match> matches;
    std::unordered_map<ObjectGuid::LowType, Reward> rewards;

    bool IsEnabled() { return cfg.Enabled && dbLoaded; }

    std::optional<uint8> ParseBracket(std::string_view text)
    {
        for (uint8 b = 0; b < BRACKET_COUNT; ++b)
            if (text == BRACKET_NAMES[b] || (text.size() == 1 && text[0] == char('0' + TEAM_SIZES[b])))
                return b;
        return std::nullopt;
    }

    void Notify(ObjectGuid guid, std::string const& text)
    {
        if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
            ChatHandler(player->GetSession()).SendSysMessage(text);
    }

    void LoadConfig()
    {
        cfg.Enabled = sConfigMgr->GetBoolDefault("Aeternum.Arena.Enable", false);
        cfg.UpdateInterval = std::max<int32>(1000, sConfigMgr->GetIntDefault("Aeternum.Arena.UpdateInterval", 5000));
        cfg.MinLevel = sConfigMgr->GetIntDefault("Aeternum.Arena.MinLevel", 1);
        cfg.RatedLevel = sConfigMgr->GetIntDefault("Aeternum.Arena.Rated.Level", 60);
        cfg.SkirmishMaxLevelDiff = sConfigMgr->GetIntDefault("Aeternum.Arena.Skirmish.MaxLevelDiff", 3);
        cfg.StartRating = sConfigMgr->GetIntDefault("Aeternum.Arena.Rating.Start", 1500);
        cfg.K = sConfigMgr->GetFloatDefault("Aeternum.Arena.Rating.K", 32.0f);
        cfg.WindowInitial = sConfigMgr->GetIntDefault("Aeternum.Arena.Matchmaking.InitialWindow", 100);
        cfg.WindowStep = sConfigMgr->GetIntDefault("Aeternum.Arena.Matchmaking.WindowStep", 50);
        cfg.WindowStepSeconds = std::max<int32>(1, sConfigMgr->GetIntDefault("Aeternum.Arena.Matchmaking.WindowStepSeconds", 30));
        cfg.WindowMax = sConfigMgr->GetIntDefault("Aeternum.Arena.Matchmaking.MaxWindow", 1000);
        cfg.LeashRadius = sConfigMgr->GetFloatDefault("Aeternum.Arena.Leash.Radius", 60.0f);
        cfg.InviteTimeout = sConfigMgr->GetIntDefault("Aeternum.Arena.InviteTimeout", 60);
        cfg.HonorRatedWin = sConfigMgr->GetIntDefault("Aeternum.Arena.Honor.RatedWin", 50);
        cfg.HonorUnratedWin = sConfigMgr->GetIntDefault("Aeternum.Arena.Honor.UnratedWin", 15);
        cfg.HonorLoss = sConfigMgr->GetIntDefault("Aeternum.Arena.Honor.Loss", 0);
        cfg.SeasonTopN = sConfigMgr->GetIntDefault("Aeternum.Arena.Season.TopN", 3);
        cfg.SeasonMinGames = sConfigMgr->GetIntDefault("Aeternum.Arena.Season.MinGames", 10);
        cfg.TitleRank1 = sConfigMgr->GetIntDefault("Aeternum.Arena.Season.TitleRank1", 1352);
        cfg.TitleTopN = sConfigMgr->GetIntDefault("Aeternum.Arena.Season.TitleTopN", 1330);

        for (uint32 title : { cfg.TitleRank1, cfg.TitleTopN })
            if (title && !sCharTitlesStore.LookupEntry(title))
                TC_LOG_ERROR("server.loading", "Aeternum arena: CharTitles {} does not exist in the client DB2, that season title is skipped", title);
    }

    // Startup (world thread): season, ratings of the current season, rewards not delivered yet.
    void LoadDatabase()
    {
        if (!CharacterDatabase.Query("SELECT 1 FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'custom_arena_pending_reward'"))
        {
            TC_LOG_ERROR("server.loading", "Aeternum arena: characters.custom_arena_* tables are missing (sql/optional/arena), arena disabled");
            return;
        }

        std::lock_guard<std::mutex> guard(lock);
        if (QueryResult result = CharacterDatabase.Query("SELECT id FROM custom_arena_season WHERE end_time IS NULL ORDER BY id DESC LIMIT 1"))
            season = result->Fetch()[0].GetUInt32();
        else
        {
            QueryResult last = CharacterDatabase.Query("SELECT IFNULL(MAX(id), 0) FROM custom_arena_season");
            season = (last ? last->Fetch()[0].GetUInt32() : 0) + 1;
            CharacterDatabase.DirectPExecute("INSERT INTO custom_arena_season (id, start_time) VALUES ({}, UNIX_TIMESTAMP())", season);
        }

        for (auto& bracket : ratings)
            bracket.clear();
        if (QueryResult result = CharacterDatabase.PQuery("SELECT guid, bracket, rating, wins, losses FROM custom_arena_rating WHERE season = {}", season))
        {
            do
            {
                Field* fields = result->Fetch();
                uint8 bracket = fields[1].GetUInt8();
                if (bracket < BRACKET_COUNT)
                    ratings[bracket][fields[0].GetUInt64()] = { fields[2].GetInt32(), fields[3].GetUInt32(), fields[4].GetUInt32() };
            } while (result->NextRow());
        }

        rewards.clear();
        if (QueryResult result = CharacterDatabase.Query("SELECT guid, honor, titles FROM custom_arena_pending_reward"))
        {
            do
            {
                Field* fields = result->Fetch();
                Reward& reward = rewards[fields[0].GetUInt64()];
                reward.Honor = fields[1].GetUInt32();
                for (std::string_view title : Trinity::Tokenize(fields[2].GetStringView(), ' ', false))
                    if (Optional<uint32> id = Trinity::StringTo<uint32>(title))
                        reward.Titles.push_back(*id);
            } while (result->NextRow());
        }

        dbLoaded = true;
        TC_LOG_INFO("server.loading", "Aeternum arena: season {}, {} pending rewards", season, rewards.size());
    }

    // ---- data helpers, lock held ----

    Rating& GetRatingLocked(ObjectGuid::LowType guid, uint8 bracket)
    {
        auto [itr, inserted] = ratings[bracket].try_emplace(guid);
        if (inserted)
            itr->second.Value = cfg.StartRating;
        return itr->second;
    }

    void SaveRewardLocked(ObjectGuid::LowType guid)
    {
        auto itr = rewards.find(guid);
        if (itr == rewards.end())
        {
            CharacterDatabase.PExecute("DELETE FROM custom_arena_pending_reward WHERE guid = {}", guid);
            return;
        }

        std::string titles;
        for (uint32 title : itr->second.Titles)
            titles += Trinity::StringFormat("{} ", title);
        CharacterDatabase.PExecute("REPLACE INTO custom_arena_pending_reward (guid, honor, titles) VALUES ({}, {}, '{}')", guid, itr->second.Honor, titles);
    }

    QueueEntry* FindQueueEntryLocked(ObjectGuid guid)
    {
        for (QueueEntry& entry : queue)
            if (std::find(entry.Members.begin(), entry.Members.end(), guid) != entry.Members.end())
                return &entry;
        return nullptr;
    }

    // ---- queue ----

    // Checks one player for queueing or for being sent to the arena.
    std::string CheckPlayer(Player* player)
    {
        if (player->InBattleground() || player->InArena())
            return Trinity::StringFormat("{} is in a battleground or arena.", player->GetName());
        if (player->InBattlegroundQueue())
            return Trinity::StringFormat("{} is in a battleground queue.", player->GetName());
        if (!player->HasFreeBattlegroundQueueId())
            return Trinity::StringFormat("{} has no free battleground queue slot.", player->GetName());
        if (player->GetLevel() < cfg.MinLevel)
            return Trinity::StringFormat("{} must be level {} or higher.", player->GetName(), cfg.MinLevel);
        return {};
    }

    // Lowest and highest current level of players ({0, 0} when empty).
    std::pair<uint32, uint32> LevelRange(std::vector<Player*> const& players)
    {
        uint32 low = 0;
        uint32 high = 0;
        for (Player* player : players)
        {
            uint32 level = player->GetLevel();
            if (!low || level < low)
                low = level;
            high = std::max(high, level);
        }
        return { low, high };
    }

    // Level rules of one queue entry: rated needs every member at RatedLevel, skirmish a level spread within
    // SkirmishMaxLevelDiff (the spread of the whole match is checked by the matchmaker).
    std::string CheckLevels(std::vector<Player*> const& players, bool rated)
    {
        if (rated)
        {
            for (Player* player : players)
                if (player->GetLevel() < cfg.RatedLevel)
                    return Trinity::StringFormat("Rated arena is for level {} only ({} is level {}). Use skirmish: .arena queue 1v1/2v2/3v3 skirmish",
                        cfg.RatedLevel, player->GetName(), uint32(player->GetLevel()));
            return {};
        }

        auto [low, high] = LevelRange(players);
        if (high - low > cfg.SkirmishMaxLevelDiff)
            return Trinity::StringFormat("Skirmish allows at most {} levels of difference, your group is level {}-{}.", cfg.SkirmishMaxLevelDiff, low, high);
        return {};
    }

    // Returns the queue entry for player (alone or with the group) or an error text. players: the members, in order.
    std::string BuildEntry(Player* player, uint8 bracket, bool rated, QueueEntry& entry, std::vector<Player*>& players)
    {
        uint8 teamSize = TEAM_SIZES[bracket];
        entry.Leader = player->GetGUID();
        entry.Bracket = bracket;
        entry.Rated = rated;
        entry.JoinTime = GameTime::GetGameTime();

        if (teamSize == 1)
        {
            entry.Members.push_back(player->GetGUID());
            players.push_back(player);
            std::string error = CheckPlayer(player);
            return error.empty() ? CheckLevels(players, rated) : error;
        }

        Group* group = player->GetGroup();
        if (!group || group->isBGGroup())
            return Trinity::StringFormat("{} needs a group of exactly {} players.", BRACKET_NAMES[bracket], teamSize);
        if (group->GetMembersCount() != teamSize)
            return Trinity::StringFormat("{} needs a group of exactly {} players (yours has {}).", BRACKET_NAMES[bracket], teamSize, group->GetMembersCount());
        if (!group->IsLeader(player->GetGUID()))
            return "Only the group leader can queue the group.";

        for (Group::MemberSlot const& slot : group->GetMemberSlots())
        {
            Player* member = ObjectAccessor::FindConnectedPlayer(slot.guid);
            if (!member || !member->IsInWorld())
                return Trinity::StringFormat("{} is offline.", slot.name);
            std::string error = CheckPlayer(member);
            if (!error.empty())
                return error;
            entry.Members.push_back(slot.guid);
            players.push_back(member);
        }
        return CheckLevels(players, rated);
    }

    std::string Join(Player* player, uint8 bracket, bool rated)
    {
        if (!IsEnabled())
            return "Arena is disabled.";

        QueueEntry entry;
        std::vector<Player*> players;
        std::string error = BuildEntry(player, bracket, rated, entry, players);
        if (!error.empty())
            return error;

        std::lock_guard<std::mutex> guard(lock);
        for (ObjectGuid const& member : entry.Members)
            if (FindQueueEntryLocked(member))
                return "You or a group member are already in the arena queue (.arena leave).";

        int32 total = 0;
        for (ObjectGuid const& member : entry.Members)
            total += GetRatingLocked(member.GetCounter(), bracket).Value;
        entry.Rating = total / int32(entry.Members.size());

        std::string text;
        if (rated)
            text = Trinity::StringFormat("Queued for {} rated arena (rating {}).", BRACKET_NAMES[bracket], entry.Rating);
        else
        {
            auto [low, high] = LevelRange(players);
            std::string levels = low == high ? Trinity::StringFormat("level {}", low) : Trinity::StringFormat("levels {}-{}", low, high);
            text = Trinity::StringFormat("Queued for {} skirmish arena ({}): every player of the match within {} levels.", BRACKET_NAMES[bracket], levels, cfg.SkirmishMaxLevelDiff);
        }
        for (ObjectGuid const& member : entry.Members)
            if (member != player->GetGUID())
                Notify(member, text);
        queue.push_back(std::move(entry));
        return text;
    }

    // Removes the whole entry (group) of guid. Returns the other members to notify.
    bool Leave(ObjectGuid guid, std::vector<ObjectGuid>* members = nullptr)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto itr = std::find_if(queue.begin(), queue.end(), [&](QueueEntry const& entry)
        {
            return std::find(entry.Members.begin(), entry.Members.end(), guid) != entry.Members.end();
        });
        if (itr == queue.end())
            return false;
        if (members)
            *members = itr->Members;
        queue.erase(itr);
        return true;
    }

    void LeaveAndNotify(ObjectGuid guid, std::string const& reason)
    {
        std::vector<ObjectGuid> members;
        if (Leave(guid, &members))
            for (ObjectGuid const& member : members)
                Notify(member, reason);
    }

    uint32 Window(QueueEntry const& entry, time_t now)
    {
        uint32 steps = uint32(std::max<time_t>(0, now - entry.JoinTime)) / cfg.WindowStepSeconds;
        uint32 window = cfg.WindowInitial + steps * cfg.WindowStep;
        return cfg.WindowMax ? std::min(window, cfg.WindowMax) : window;
    }

    enum class EntryState { Ready, Wait, Drop };

    // World thread only. Players of a Ready entry are returned in order.
    EntryState ValidateEntry(QueueEntry const& entry, std::vector<Player*>& players, std::string& reason)
    {
        players.clear();
        Group* group = nullptr;
        for (ObjectGuid const& guid : entry.Members)
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(guid);
            if (!player || !player->IsInWorld())
            {
                reason = "A member of your arena queue went offline, queue left.";
                return EntryState::Drop;
            }
            if (player->IsBeingTeleported() || player->IsInCombat())
                return EntryState::Wait;
            // back in the queue after a cancelled match, still on the way out of the arena
            if (entry.RequeuedAt && player->InBattleground() && GameTime::GetGameTime() - entry.RequeuedAt < time_t(cfg.InviteTimeout + 60))
                return EntryState::Wait;
            reason = CheckPlayer(player);
            if (!reason.empty())
            {
                reason += " Arena queue left.";
                return EntryState::Drop;
            }
            if (entry.Members.size() > 1)
            {
                Group* playerGroup = player->GetGroup();
                if (!playerGroup || (group && playerGroup != group) || playerGroup->GetMembersCount() != entry.Members.size() || !playerGroup->IsLeader(entry.Leader))
                {
                    reason = "Your group changed, arena queue left.";
                    return EntryState::Drop;
                }
                group = playerGroup;
            }
            players.push_back(player);
        }
        // live levels: a level-up while queued can break the skirmish spread of a group
        reason = CheckLevels(players, entry.Rated);
        if (!reason.empty())
        {
            reason += " Arena queue left.";
            return EntryState::Drop;
        }
        return EntryState::Ready;
    }

    void Invite(Player* player, Battleground* bg, BattlegroundQueueTypeId queueId, Team team)
    {
        // same steps as an accepted battlemaster invite (WorldSession::HandleBattleFieldPortOpcode)
        player->AddBattlegroundQueueId(queueId);
        player->SetInviteForBattlegroundQueueType(queueId, bg->GetInstanceID());
        bg->IncreaseInvitedCount(team);
        player->SetBattlegroundEntryPoint();
        if (!player->IsAlive())
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();
        }
        player->FinishTaxiFlight();
        player->SetBattlegroundId(bg->GetInstanceID(), bg->GetTypeID(), queueId);
        player->SetBGTeam(team);
        BattlegroundMgr::SendToBattleground(player, bg);
    }

    void StartMatch(QueueEntry const& a, std::vector<Player*> const& playersA, QueueEntry const& b, std::vector<Player*> const& playersB)
    {
        BattlegroundQueueTypeId queueId = BattlegroundMgr::BGQueueTypeId(BATTLEMASTER_LIST_ID, BattlegroundQueueIdType::Arena, false, TEAM_SIZES[a.Bracket]);
        Battleground* bg = sBattlegroundMgr->CreateNewBattleground(queueId, BG_BRACKET_ID_FIRST);
        if (!bg)
        {
            TC_LOG_ERROR("scripts", "Aeternum arena: could not create the arena (battleground_template 1169 / pvp_difficulty for map 2995 missing?)");
            for (QueueEntry const* entry : { &a, &b })
                for (ObjectGuid const& guid : entry->Members)
                    Notify(guid, "The arena could not be created, please tell a GM. Arena queue left.");
            return;
        }
        bg->StartBattleground();

        Match match;
        match.Bracket = a.Bracket;
        match.Rated = a.Rated;
        match.QueueId = queueId;
        match.Created = GameTime::GetGameTime();
        match.Teams[TEAM_ALLIANCE] = a.Members;
        match.Teams[TEAM_HORDE] = b.Members;
        match.Entries = { a, b };
        match.Entries[0].RequeuedAt = match.Entries[1].RequeuedAt = 0;
        match.NotArrived = a.Members;
        match.NotArrived.insert(match.NotArrived.end(), b.Members.begin(), b.Members.end());
        {
            std::lock_guard<std::mutex> guard(lock);
            matches[bg->GetInstanceID()] = std::move(match);
        }

        for (Player* player : playersA)
            Invite(player, bg, queueId, ALLIANCE);
        for (Player* player : playersB)
            Invite(player, bg, queueId, HORDE);

        TC_LOG_INFO("scripts", "Aeternum arena: {} {} match started, instance {} (rating {} vs {})",
            BRACKET_NAMES[a.Bracket], a.Rated ? "rated" : "skirmish", bg->GetInstanceID(), a.Rating, b.Rating);
    }

    void UpdateQueue()
    {
        std::vector<QueueEntry> entries;
        {
            std::lock_guard<std::mutex> guard(lock);
            entries.swap(queue);
        }

        time_t now = GameTime::GetGameTime();
        std::vector<std::vector<Player*>> players(entries.size());
        std::vector<EntryState> states(entries.size());
        std::vector<std::pair<uint32, uint32>> levels(entries.size());   // live lowest/highest level of Ready entries
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            std::string reason;
            states[i] = ValidateEntry(entries[i], players[i], reason);
            if (states[i] == EntryState::Ready)
                levels[i] = LevelRange(players[i]);
            if (states[i] == EntryState::Drop)
                for (ObjectGuid const& guid : entries[i].Members)
                    Notify(guid, reason);
        }

        // oldest entry first, paired with the closest rating inside the larger of the two windows (rated, all at RatedLevel);
        // skirmish: the oldest entry whose players keep the whole match within SkirmishMaxLevelDiff levels
        std::vector<bool> used(entries.size(), false);
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            if (used[i] || states[i] != EntryState::Ready)
                continue;
            std::size_t best = entries.size();
            uint32 bestDiff = 0;
            for (std::size_t j = i + 1; j < entries.size(); ++j)
            {
                if (used[j] || states[j] != EntryState::Ready || entries[j].Bracket != entries[i].Bracket || entries[j].Rated != entries[i].Rated)
                    continue;
                if (!entries[i].Rated && std::max(levels[i].second, levels[j].second) - std::min(levels[i].first, levels[j].first) > cfg.SkirmishMaxLevelDiff)
                    continue;
                uint32 diff = uint32(std::abs(entries[i].Rating - entries[j].Rating));
                if (entries[i].Rated && diff > std::max(Window(entries[i], now), Window(entries[j], now)))
                    continue;
                if (best == entries.size() || diff < bestDiff)
                {
                    best = j;
                    bestDiff = diff;
                }
                if (!entries[i].Rated)
                    break;
            }
            if (best == entries.size())
                continue;
            used[i] = used[best] = true;
            StartMatch(entries[i], players[i], entries[best], players[best]);
        }

        std::lock_guard<std::mutex> guard(lock);
        std::vector<QueueEntry> kept;
        for (std::size_t i = 0; i < entries.size(); ++i)
            if (!used[i] && states[i] != EntryState::Drop)
                kept.push_back(std::move(entries[i]));
        // entries queued meanwhile (none in the world tick, kept for safety) go after the older ones
        kept.insert(kept.end(), std::make_move_iterator(queue.begin()), std::make_move_iterator(queue.end()));
        queue.swap(kept);
    }

    std::string GetName(ObjectGuid::LowType guid);

    void DeclineLocked(Match& match, ObjectGuid guid)
    {
        if (match.Ended)
            return;
        if (std::find(match.Declined.begin(), match.Declined.end(), guid) == match.Declined.end())
            match.Declined.push_back(guid);
        match.Cancelled = true;
    }

    // Lock held, world thread: messages, queue entries put back (original JoinTime, so the same priority) or removed.
    void HandleCancelLocked(Match& match, std::vector<std::pair<ObjectGuid, std::string>>& messages)
    {
        match.Ended = true;
        std::string who;
        for (ObjectGuid const& guid : match.Declined)
            who += (who.empty() ? "" : ", ") + GetName(guid.GetCounter());
        std::string reason = who.empty() ? "a player could not reach the arena" : who + " did not accept";
        std::string mode = Trinity::StringFormat("{} {}", BRACKET_NAMES[match.Bracket], match.Rated ? "rated" : "skirmish");
        time_t now = GameTime::GetGameTime();

        for (QueueEntry const& entry : match.Entries)
        {
            bool declined = std::any_of(entry.Members.begin(), entry.Members.end(), [&](ObjectGuid const& guid)
            {
                return std::find(match.Declined.begin(), match.Declined.end(), guid) != match.Declined.end();
            });
            bool alreadyQueued = std::any_of(entry.Members.begin(), entry.Members.end(), [&](ObjectGuid const& guid) { return FindQueueEntryLocked(guid) != nullptr; });
            if (!declined && !alreadyQueued && !entry.Members.empty())
            {
                QueueEntry requeued = entry;
                requeued.RequeuedAt = now;
                auto pos = std::upper_bound(queue.begin(), queue.end(), requeued.JoinTime, [](time_t joinTime, QueueEntry const& queued) { return joinTime < queued.JoinTime; });
                queue.insert(pos, std::move(requeued));
                for (ObjectGuid const& guid : entry.Members)
                    messages.emplace_back(guid, Trinity::StringFormat("Arena match cancelled ({}): no result, no rating change. You are back in the {} queue with your previous priority.", reason, mode));
                continue;
            }
            for (ObjectGuid const& guid : entry.Members)
            {
                bool self = std::find(match.Declined.begin(), match.Declined.end(), guid) != match.Declined.end();
                messages.emplace_back(guid, self
                    ? std::string("Arena match cancelled: you did not enter the arena in time or left it before the start. You were removed from the arena queue.")
                    : Trinity::StringFormat("Arena match cancelled ({}): no result, no rating change. Your group was removed from the queue because a member did not accept, queue again.", reason));
            }
        }
        TC_LOG_INFO("scripts", "Aeternum arena: {} match cancelled ({})", mode, reason);
    }

    // World thread. Invited players that never reach the arena (logout while loading, failed teleport) are released
    // after InviteTimeout and count as a decline. Cancelled matches: messages, queue, and everybody still inside the
    // arena (also late arrivals) is sent back to the entry point. Also forgets deleted arenas.
    void UpdateMatches()
    {
        time_t now = GameTime::GetGameTime();
        std::vector<std::pair<ObjectGuid, std::string>> messages;
        std::vector<std::pair<uint32 /*instanceId*/, ObjectGuid>> kicks;
        {
            std::lock_guard<std::mutex> guard(lock);
            for (auto itr = matches.begin(); itr != matches.end();)
            {
                Battleground* bg = sBattlegroundMgr->GetBattleground(itr->first, BattlegroundTypeId(BATTLEMASTER_LIST_ID));
                if (!bg)
                {
                    itr = matches.erase(itr);
                    continue;
                }

                Match& match = itr->second;
                std::vector<ObjectGuid> released;
                std::erase_if(match.NotArrived, [&](ObjectGuid const& guid)
                {
                    if (bg->IsPlayerInBattleground(guid))
                        return true;
                    Player* player = ObjectAccessor::FindConnectedPlayer(guid);
                    bool timedOut = now - match.Created > time_t(cfg.InviteTimeout);
                    if (player && player->GetBattlegroundId() == bg->GetInstanceID() && (!timedOut || player->IsBeingTeleportedFar()))
                        return false;   // still loading
                    Team team = std::find(match.Teams[TEAM_ALLIANCE].begin(), match.Teams[TEAM_ALLIANCE].end(), guid) != match.Teams[TEAM_ALLIANCE].end() ? ALLIANCE : HORDE;
                    bg->DecreaseInvitedCount(team);
                    if (player && player->GetBattlegroundId() == bg->GetInstanceID())
                    {
                        player->RemoveBattlegroundQueueId(match.QueueId);
                        player->SetBattlegroundId(0, BATTLEGROUND_TYPE_NONE, BATTLEGROUND_QUEUE_NONE);
                        player->SetBGTeam(TEAM_OTHER);
                    }
                    released.push_back(guid);
                    return true;
                });
                for (ObjectGuid const& guid : released)
                    DeclineLocked(match, guid);

                if (match.Cancelled)
                {
                    if (!match.Ended)
                        HandleCancelLocked(match, messages);
                    for (auto const& [guid, _] : bg->GetPlayers())
                        kicks.emplace_back(bg->GetInstanceID(), guid);
                }
                ++itr;
            }
        }

        for (auto const& [guid, text] : messages)
            Notify(guid, text);

        // outside the lock: leaving ends the arena (Arena::CheckWinConditions -> OnMatchEnd, which ignores cancelled matches)
        for (auto const& [instanceId, guid] : kicks)
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(guid);
            if (!player || !player->IsInWorld() || player->IsBeingTeleported() || player->GetBattlegroundId() != instanceId)
                continue;
            ChatHandler(player->GetSession()).SendSysMessage("Arena match cancelled, you are sent back.");
            player->LeaveBattleground(true, true);
        }
    }

    // Map thread, before the gates open: an invited player who already entered and is no longer in the arena left it
    // (Leave Arena, .arena leave, logout): that is a decline.
    void CheckPrepLeavers(uint32 instanceId, Battleground const* bg)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto itr = matches.find(instanceId);
        if (itr == matches.end() || itr->second.Ended)
            return;
        Match& match = itr->second;
        for (auto const& team : match.Teams)
            for (ObjectGuid const& guid : team)
                if (std::find(match.NotArrived.begin(), match.NotArrived.end(), guid) == match.NotArrived.end() && !bg->IsPlayerInBattleground(guid))
                    DeclineLocked(match, guid);
    }

    // Honor and titles are given once the player is out of the arena (RewardHonor does nothing inside one).
    void DeliverRewards()
    {
        std::vector<std::pair<Player*, Reward>> deliveries;
        {
            std::lock_guard<std::mutex> guard(lock);
            for (auto itr = rewards.begin(); itr != rewards.end();)
            {
                Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(itr->first));
                if (!player || player->InBattleground() || player->IsBeingTeleported())
                {
                    ++itr;
                    continue;
                }
                ObjectGuid::LowType guid = itr->first;
                deliveries.emplace_back(player, std::move(itr->second));
                itr = rewards.erase(itr);
                SaveRewardLocked(guid);
            }
        }

        for (auto& [player, reward] : deliveries)
        {
            ChatHandler handler(player->GetSession());
            if (reward.Honor)
            {
                player->RewardHonor(nullptr, 1, int32(reward.Honor), HonorGainSource::ArenaCompletion);
                handler.SendSysMessage(Trinity::StringFormat("Arena: you received {} honor.", reward.Honor));
            }
            for (uint32 titleId : reward.Titles)
            {
                CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(titleId);
                if (!title || player->HasTitle(title))
                    continue;
                player->SetTitle(title);
                handler.SendSysMessage("Arena: you received a season title, check your titles.");
            }
        }
    }

    // Called by the map script when an invited player enters the arena (map thread).
    void MarkArrived(uint32 instanceId, ObjectGuid guid)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto itr = matches.find(instanceId);
        if (itr != matches.end())
            std::erase(itr->second.NotArrived, guid);
    }

    // ---- match end, any thread ----

    // started: the gates were open. A match that ends before (someone left during the preparation), that ends while
    // an invited player is still not inside (a whole side missing when the gates open) or that already has a decline
    // is cancelled instead: no result here, the world tick sends the messages, the queue entries and the players back.
    void OnMatchEnd(uint32 instanceId, Team winner, bool started, Battleground const* bg)
    {
        std::vector<std::pair<ObjectGuid, std::string>> messages;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto itr = matches.find(instanceId);
            if (itr == matches.end() || itr->second.Ended)
                return;
            Match& match = itr->second;
            if (match.Cancelled)
                return;
            if (!started || !match.NotArrived.empty())
            {
                for (auto const& team : match.Teams)
                    for (ObjectGuid const& guid : team)
                        if (std::find(match.NotArrived.begin(), match.NotArrived.end(), guid) == match.NotArrived.end() && !bg->IsPlayerInBattleground(guid))
                            DeclineLocked(match, guid);
                match.Cancelled = true;
                return;
            }
            match.Ended = true;

            if (winner != ALLIANCE && winner != HORDE)
            {
                for (auto const& team : match.Teams)
                    for (ObjectGuid const& guid : team)
                        messages.emplace_back(guid, "Arena: draw, no rating change.");
            }
            else
            {
                TeamId winnerTeam = winner == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE;
                int32 delta = 0;
                if (match.Rated)
                {
                    std::array<float, 2> average = { };
                    for (uint8 side = 0; side < 2; ++side)
                    {
                        for (ObjectGuid const& guid : match.Teams[side])
                            average[side] += GetRatingLocked(guid.GetCounter(), match.Bracket).Value;
                        average[side] /= float(std::max<std::size_t>(1, match.Teams[side].size()));
                    }
                    float expected = 1.0f / (1.0f + std::pow(10.0f, (average[1 - winnerTeam] - average[winnerTeam]) / 400.0f));
                    delta = std::max(1, int32(std::lround(cfg.K * (1.0f - expected))));
                }

                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                for (uint8 side = 0; side < 2; ++side)
                {
                    bool won = side == winnerTeam;
                    for (ObjectGuid const& guid : match.Teams[side])
                    {
                        if (match.Rated)
                        {
                            Rating& rating = GetRatingLocked(guid.GetCounter(), match.Bracket);
                            int32 old = rating.Value;
                            rating.Value = std::max(0, rating.Value + (won ? delta : -delta));
                            ++(won ? rating.Wins : rating.Losses);
                            trans->PAppend("REPLACE INTO custom_arena_rating (guid, bracket, season, rating, wins, losses) VALUES ({}, {}, {}, {}, {}, {})",
                                guid.GetCounter(), match.Bracket, season, rating.Value, rating.Wins, rating.Losses);
                            messages.emplace_back(guid, Trinity::StringFormat("Arena {}: {}, rating {} -> {}.", BRACKET_NAMES[match.Bracket], won ? "victory" : "defeat", old, rating.Value));
                        }
                        else
                            messages.emplace_back(guid, Trinity::StringFormat("Arena {} skirmish: {}.", BRACKET_NAMES[match.Bracket], won ? "victory" : "defeat"));

                        uint32 honor = won ? (match.Rated ? cfg.HonorRatedWin : cfg.HonorUnratedWin) : cfg.HonorLoss;
                        if (honor)
                        {
                            rewards[guid.GetCounter()].Honor += honor;
                            SaveRewardLocked(guid.GetCounter());
                        }
                    }
                }
                if (match.Rated)
                    CharacterDatabase.CommitTransaction(trans);
            }
        }

        for (auto const& [guid, text] : messages)
            Notify(guid, text);
    }

    // ---- leaderboard and season ----

    struct Standing
    {
        ObjectGuid::LowType Guid;
        Rating Value;
    };

    std::vector<Standing> GetStandingsLocked(uint8 bracket, uint32 minGames, std::size_t count)
    {
        std::vector<Standing> standings;
        for (auto const& [guid, rating] : ratings[bracket])
            if (rating.Wins + rating.Losses >= std::max<uint32>(1, minGames))
                standings.push_back({ guid, rating });
        std::sort(standings.begin(), standings.end(), [](Standing const& left, Standing const& right)
        {
            if (left.Value.Value != right.Value.Value)
                return left.Value.Value > right.Value.Value;
            if (left.Value.Wins != right.Value.Wins)
                return left.Value.Wins > right.Value.Wins;
            return left.Guid < right.Guid;
        });
        if (standings.size() > count)
            standings.resize(count);
        return standings;
    }

    std::string GetName(ObjectGuid::LowType guid)
    {
        std::string name;
        if (!sCharacterCache->GetCharacterNameByGuid(ObjectGuid::Create<HighGuid::Player>(guid), name))
            name = Trinity::StringFormat("#{}", guid);
        return name;
    }

    void SendTop(ChatHandler* handler, uint8 bracket)
    {
        std::vector<Standing> standings;
        uint32 currentSeason;
        {
            std::lock_guard<std::mutex> guard(lock);
            standings = GetStandingsLocked(bracket, 1, LEADERBOARD_SIZE);
            currentSeason = season;
        }
        handler->SendSysMessage(Trinity::StringFormat("Arena {} top {} (season {}):", BRACKET_NAMES[bracket], LEADERBOARD_SIZE, currentSeason));
        if (standings.empty())
            handler->SendSysMessage("  nobody has played yet.");
        for (std::size_t i = 0; i < standings.size(); ++i)
            handler->SendSysMessage(Trinity::StringFormat("  {}. {} - {} ({}W/{}L)", i + 1, GetName(standings[i].Guid),
                standings[i].Value.Value, standings[i].Value.Wins, standings[i].Value.Losses));
    }

    void SendRating(ChatHandler* handler, ObjectGuid guid)
    {
        std::lock_guard<std::mutex> guard(lock);
        handler->SendSysMessage(Trinity::StringFormat("Arena ratings (season {}):", season));
        for (uint8 b = 0; b < BRACKET_COUNT; ++b)
        {
            auto itr = ratings[b].find(guid.GetCounter());
            if (itr == ratings[b].end())
                handler->SendSysMessage(Trinity::StringFormat("  {}: {} (no games)", BRACKET_NAMES[b], cfg.StartRating));
            else
                handler->SendSysMessage(Trinity::StringFormat("  {}: {} ({}W/{}L)", BRACKET_NAMES[b], itr->second.Value, itr->second.Wins, itr->second.Losses));
        }
        if (QueueEntry const* entry = FindQueueEntryLocked(guid))
            handler->SendSysMessage(Trinity::StringFormat("  queued for {} {} since {}s", BRACKET_NAMES[entry->Bracket],
                entry->Rated ? "rated" : "skirmish", GameTime::GetGameTime() - entry->JoinTime));
    }

    // Titles for the top N of every bracket (rank 1: TitleRank1, others: TitleTopN), standings saved, next season opened.
    void EndSeason(ChatHandler* handler)
    {
        std::lock_guard<std::mutex> guard(lock);
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        for (uint8 b = 0; b < BRACKET_COUNT; ++b)
        {
            std::vector<Standing> standings = GetStandingsLocked(b, cfg.SeasonMinGames, cfg.SeasonTopN);
            for (std::size_t i = 0; i < standings.size(); ++i)
            {
                uint32 title = i == 0 ? cfg.TitleRank1 : cfg.TitleTopN;
                if (title && !sCharTitlesStore.LookupEntry(title))
                    title = 0;
                trans->PAppend("INSERT INTO custom_arena_season_result (season, bracket, `rank`, guid, rating, wins, losses, title) VALUES ({}, {}, {}, {}, {}, {}, {}, {})",
                    season, b, i + 1, standings[i].Guid, standings[i].Value.Value, standings[i].Value.Wins, standings[i].Value.Losses, title);
                if (title)
                {
                    rewards[standings[i].Guid].Titles.push_back(title);
                    SaveRewardLocked(standings[i].Guid);
                }
                handler->SendSysMessage(Trinity::StringFormat("Season {} {} #{}: {} ({}){}", season, BRACKET_NAMES[b], i + 1,
                    GetName(standings[i].Guid), standings[i].Value.Value, title ? Trinity::StringFormat(", title {}", title) : ""));
            }
            ratings[b].clear();
        }
        trans->PAppend("UPDATE custom_arena_season SET end_time = UNIX_TIMESTAMP() WHERE id = {}", season);
        ++season;
        trans->PAppend("INSERT INTO custom_arena_season (id, start_time) VALUES ({}, UNIX_TIMESTAMP())", season);
        CharacterDatabase.CommitTransaction(trans);
        handler->SendSysMessage(Trinity::StringFormat("Arena season {} ended, season {} started (titles are given at the next world update or login).", season - 1, season));
    }
}

// Map script of Mak'gora Arena (battleground_scripts 2995): hostility, leash, rating.
struct arena_wf_makgora : ArenaScript
{
    explicit arena_wf_makgora(BattlegroundMap* map) : ArenaScript(map) { }

    void OnInit() override
    {
        WorldSafeLocsEntry const* gold = battleground->GetTeamStartPosition(TEAM_ALLIANCE);
        WorldSafeLocsEntry const* green = battleground->GetTeamStartPosition(TEAM_HORDE);
        if (gold && green)
        {
            _center.Relocate((gold->Loc.GetPositionX() + green->Loc.GetPositionX()) / 2.0f, (gold->Loc.GetPositionY() + green->Loc.GetPositionY()) / 2.0f,
                (gold->Loc.GetPositionZ() + green->Loc.GetPositionZ()) / 2.0f);
            _hasCenter = true;
        }
    }

    void OnUpdate(uint32 diff) override
    {
        _timer += diff;
        if (_timer < 1000)
            return;
        _timer = 0;

        // Classic has no arena team faction spells: players of the same faction would stay friendly. During the match
        // every player is FFA (as in Gurubashi Arena); teammates stay friendly because they share the BG raid.
        bool fighting = battleground->GetStatus() == STATUS_IN_PROGRESS;
        if (battleground->GetStatus() == STATUS_WAIT_JOIN)
            wfArena::CheckPrepLeavers(battleground->GetInstanceID(), battleground);
        for (auto const& [guid, _] : battleground->GetPlayers())
        {
            Player* player = ObjectAccessor::GetPlayer(battlegroundMap, guid);
            if (!player || !player->IsInWorld() || player->IsBeingTeleported())
                continue;

            if (player->pvpInfo.IsInFFAPvPArea != fighting)
            {
                player->pvpInfo.IsInFFAPvPArea = fighting;
                player->UpdatePvPState(true);
            }

            // the prep phase keeps players at their start (battleground_template StartMaxDist), the leash covers the match
            if (fighting && _hasCenter && !player->IsGameMaster()
                && (player->GetExactDist2d(&_center) > wfArena::cfg.LeashRadius || player->GetPositionZ() < _center.GetPositionZ() - 30.0f))
            {
                if (WorldSafeLocsEntry const* start = battleground->GetTeamStartPosition(Battleground::GetTeamIndexByTeamId(player->GetBGTeam())))
                {
                    player->NearTeleportTo(start->Loc);
                    ChatHandler(player->GetSession()).SendSysMessage("You left the arena area.");
                }
            }
        }
    }

    void OnPlayerJoined(Player* player, bool /*inBattleground*/) override
    {
        wfArena::MarkArrived(battleground->GetInstanceID(), player->GetGUID());
    }

    void OnStart() override
    {
        _started = true;
    }

    void OnEnd(Team winner) override
    {
        wfArena::OnMatchEnd(battleground->GetInstanceID(), winner, _started, battleground);
    }

private:
    bool _started = false;                              // gates open
    uint32 _timer = 0;
    Position _center;
    bool _hasCenter = false;
};

// Matchmaker tick, world thread.
class wf_arena_worldscript : public WorldScript
{
public:
    wf_arena_worldscript() : WorldScript("wf_arena_worldscript") { }

    void OnConfigLoad(bool reload) override
    {
        if (!reload)
            return;
        wfArena::LoadConfig();
        if (wfArena::cfg.Enabled && !wfArena::dbLoaded)
            wfArena::LoadDatabase();
    }

    void OnStartup() override
    {
        wfArena::LoadConfig();
        if (wfArena::cfg.Enabled)
            wfArena::LoadDatabase();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!wfArena::dbLoaded)
            return;
        _timer += diff;
        if (_timer < wfArena::cfg.UpdateInterval)
            return;
        _timer = 0;

        if (wfArena::IsEnabled())
            wfArena::UpdateQueue();
        wfArena::UpdateMatches();
        wfArena::DeliverRewards();
    }

private:
    uint32 _timer = 0;
};

class wf_arena_playerscript : public PlayerScript
{
public:
    wf_arena_playerscript() : PlayerScript("wf_arena_playerscript") { }

    void OnLogout(Player* player) override
    {
        wfArena::LeaveAndNotify(player->GetGUID(), Trinity::StringFormat("{} logged out, arena queue left.", player->GetName()));
    }
};

// Arena Master (creature_template 9500000).
struct npc_wf_arena_master : public ScriptedAI
{
    enum Actions
    {
        ACTION_QUEUE_RATED      = GOSSIP_ACTION_INFO_DEF,       // + bracket
        ACTION_QUEUE_SKIRMISH   = GOSSIP_ACTION_INFO_DEF + 10,  // + bracket
        ACTION_TOP              = GOSSIP_ACTION_INFO_DEF + 20,  // + bracket
        ACTION_LEAVE            = GOSSIP_ACTION_INFO_DEF + 30,
        ACTION_RATING           = GOSSIP_ACTION_INFO_DEF + 31
    };

    explicit npc_wf_arena_master(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        InitGossipMenuFor(player, 0);
        if (wfArena::IsEnabled())
        {
            bool queued = false;
            {
                std::lock_guard<std::mutex> guard(wfArena::lock);
                queued = wfArena::FindQueueEntryLocked(player->GetGUID()) != nullptr;
            }
            if (queued)
                AddGossipItemFor(player, GossipOptionNpc::None, "Leave the arena queue", GOSSIP_SENDER_MAIN, ACTION_LEAVE);
            else
            {
                // rated: level 60 only (the group members are checked when queueing)
                if (player->GetLevel() >= wfArena::cfg.RatedLevel)
                    for (uint8 b = 0; b < wfArena::BRACKET_COUNT; ++b)
                        AddGossipItemFor(player, GossipOptionNpc::None, Trinity::StringFormat("Queue {} rated", wfArena::BRACKET_NAMES[b]), GOSSIP_SENDER_MAIN, ACTION_QUEUE_RATED + b);
                for (uint8 b = 0; b < wfArena::BRACKET_COUNT; ++b)
                    AddGossipItemFor(player, GossipOptionNpc::None, Trinity::StringFormat("Queue {} skirmish (no rating)", wfArena::BRACKET_NAMES[b]), GOSSIP_SENDER_MAIN, ACTION_QUEUE_SKIRMISH + b);
            }
        }
        AddGossipItemFor(player, GossipOptionNpc::None, "My arena rating", GOSSIP_SENDER_MAIN, ACTION_RATING);
        for (uint8 b = 0; b < wfArena::BRACKET_COUNT; ++b)
            AddGossipItemFor(player, GossipOptionNpc::None, Trinity::StringFormat("Top 10 {}", wfArena::BRACKET_NAMES[b]), GOSSIP_SENDER_MAIN, ACTION_TOP + b);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 action = GetGossipActionFor(player, gossipListId);
        CloseGossipMenuFor(player);
        ChatHandler handler(player->GetSession());

        if (action >= ACTION_QUEUE_RATED && action < ACTION_QUEUE_RATED + wfArena::BRACKET_COUNT)
            handler.SendSysMessage(wfArena::Join(player, uint8(action - ACTION_QUEUE_RATED), true));
        else if (action >= ACTION_QUEUE_SKIRMISH && action < ACTION_QUEUE_SKIRMISH + wfArena::BRACKET_COUNT)
            handler.SendSysMessage(wfArena::Join(player, uint8(action - ACTION_QUEUE_SKIRMISH), false));
        else if (action >= ACTION_TOP && action < ACTION_TOP + wfArena::BRACKET_COUNT)
            wfArena::SendTop(&handler, uint8(action - ACTION_TOP));
        else if (action == ACTION_LEAVE)
            wfArena::LeaveAndNotify(player->GetGUID(), "Arena queue left.");
        else if (action == ACTION_RATING)
            wfArena::SendRating(&handler, player->GetGUID());
        return true;
    }
};

class wf_arena_commandscript : public CommandScript
{
public:
    wf_arena_commandscript() : CommandScript("wf_arena_commandscript") { }

    // merged with the core ".arena" table (cs_arena.cpp: create, disband, rename, captain, info, lookup)
    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable arenaCommandTable =
        {
            { "queue",      HandleQueueCommand,     rbac::RBAC_PERM_COMMAND_WF_ARENA,       Console::No },
            { "leave",      HandleLeaveCommand,     rbac::RBAC_PERM_COMMAND_WF_ARENA,       Console::No },
            { "rating",     HandleRatingCommand,    rbac::RBAC_PERM_COMMAND_WF_ARENA,       Console::No },
            { "top",        HandleTopCommand,       rbac::RBAC_PERM_COMMAND_WF_ARENA,       Console::Yes },
            { "season end", HandleSeasonEndCommand, rbac::RBAC_PERM_COMMAND_WF_ARENA_GM,    Console::Yes },
        };
        static ChatCommandTable commandTable =
        {
            { "arena", arenaCommandTable },
        };
        return commandTable;
    }

    // .arena queue 1v1|2v2|3v3 [skirmish]: rated (default) only at Rated.Level, skirmish from MinLevel within Skirmish.MaxLevelDiff
    static bool HandleQueueCommand(ChatHandler* handler, std::string bracketName, Optional<std::string> mode)
    {
        Optional<uint8> bracket = wfArena::ParseBracket(bracketName);
        if (!bracket)
        {
            handler->SendSysMessage(Trinity::StringFormat("Usage: .arena queue 1v1/2v2/3v3 [skirmish] - rated: every member level {}; skirmish: from level {}, every player of the match within {} levels.",
                wfArena::cfg.RatedLevel, wfArena::cfg.MinLevel, wfArena::cfg.SkirmishMaxLevelDiff));
            handler->SetSentErrorMessage(true);
            return false;
        }
        bool rated = !mode || (*mode != "skirmish" && *mode != "unrated");
        handler->SendSysMessage(wfArena::Join(handler->GetPlayer(), *bracket, rated));
        return true;
    }

    // .arena leave: the queue, or the arena itself (no deserter in arenas; before the gates open it cancels the match)
    static bool HandleLeaveCommand(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (wfArena::Leave(player->GetGUID(), nullptr))
        {
            handler->SendSysMessage("Arena queue left.");
            return true;
        }
        if (player->InArena() && player->GetMapId() == 2995)
        {
            player->LeaveBattleground();
            return true;
        }
        handler->SendSysMessage("You are not in the arena queue.");
        return true;
    }

    static bool HandleRatingCommand(ChatHandler* handler)
    {
        wfArena::SendRating(handler, handler->GetPlayer()->GetGUID());
        return true;
    }

    // .arena top [1v1|2v2|3v3]
    static bool HandleTopCommand(ChatHandler* handler, Optional<std::string> bracketName)
    {
        if (bracketName)
        {
            Optional<uint8> bracket = wfArena::ParseBracket(*bracketName);
            if (!bracket)
            {
                handler->SendSysMessage("Usage: .arena top [1v1|2v2|3v3]");
                handler->SetSentErrorMessage(true);
                return false;
            }
            wfArena::SendTop(handler, *bracket);
            return true;
        }
        for (uint8 b = 0; b < wfArena::BRACKET_COUNT; ++b)
            wfArena::SendTop(handler, b);
        return true;
    }

    static bool HandleSeasonEndCommand(ChatHandler* handler)
    {
        if (!wfArena::dbLoaded)
        {
            handler->SendSysMessage("Arena is disabled (Aeternum.Arena.Enable or the custom_arena_* tables).");
            handler->SetSentErrorMessage(true);
            return false;
        }
        wfArena::EndSeason(handler);
        return true;
    }
};

void AddSC_wf_arena()
{
    RegisterBattlegroundMapScript(arena_wf_makgora, 2995);
    new wf_arena_worldscript();
    new wf_arena_playerscript();
    RegisterCreatureAI(npc_wf_arena_master);
    new wf_arena_commandscript();
}
