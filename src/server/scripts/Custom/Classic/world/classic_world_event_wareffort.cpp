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

// Classic 1.60 port of VMaNGOS src/scripts/world/world_event_wareffort.cpp (MaNGOS / Nostalrius / Light's Hope lineage, GPL-2)
// Ported: npc_aqwar_saurfang (High Overlord Saurfang, Cenarion Hold), npc_aqwar_cenarionhold_attack (Qiraji waves),
// a simplified npc_resonating_crystal, plus Aeternum's Scarab Gong / Ten Hours of War controller (see below).
// TODO(classic): the rest of the VMaNGOS war effort system (resource counters in saved variables,
// npc_AQwar_collector, transport/transition game events, infantry followers) does not exist in TC and is not ported.

#include "ScriptMgr.h"
#include "DB2Stores.h"
#include "GameEventMgr.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"
#include "World.h"
#include "WorldStateMgr.h"
#include "classic_script_text.h"
#include <array>
#include <iterator>

enum AQWarSaurfang
{
    // VMaNGOS game_event 59/61 imported as Aeternum game_event 121/122 (fixes/legendary/05_aq_scarab_gong_war.sql)
    AQWAR_EVENT_WAR_EFFORT_CH_ATTACK    = 121,
    AQWAR_EVENT_WAR_EFFORT_FINALBATTLE  = 122,

    AQWAR_ZONE_SILITHUS                 = 1377,

    AQWAR_BCT_SAURFANG_SPEECH1          = 11620,
    AQWAR_BCT_SAURFANG_SPEECH2          = 11621,
    AQWAR_BCT_SAURFANG_SPEECH3          = 11622,
    AQWAR_BCT_SAURFANG_SPEECH4          = 11623,
    AQWAR_BCT_SAURFANG_SPEECH5          = 11624,
    AQWAR_BCT_SAURFANG_SPEECH6          = 11625,
    AQWAR_BCT_SAURFANG_SPEECH7          = 11626,
    AQWAR_BCT_SAURFANG_SPEECH8          = 11627,
    AQWAR_BCT_SAURFANG_SPEECH9          = 11628,
    AQWAR_BCT_SAURFANG_SPEECH10         = 11629,
    AQWAR_BCT_SAURFANG_SPEECH11         = 11630,
    AQWAR_BCT_SAURFANG_SPEECH12         = 11631,
    AQWAR_BCT_SAURFANG_SPEECH13         = 11646,
    AQWAR_BCT_SAURFANG_SPEECH14         = 11647,
    AQWAR_BCT_SAURFANG_FINAL_BATTLE     = 11619,

    AQWAR_BCT_SAURFANG_AGGRO1           = 11527, // guessed
    AQWAR_BCT_SAURFANG_AGGRO2           = 11528, // guessed
    AQWAR_BCT_SAURFANG_AGGRO3           = 11538, // sniffed
    AQWAR_BCT_SAURFANG_AGGRO4           = 11540, // sniffed
    AQWAR_BCT_SAURFANG_AGGRO5           = 11541, // guessed
    AQWAR_BCT_SAURFANG_AGGRO6           = 11614, // sniffed
    AQWAR_BCT_SAURFANG_AGGRO7           = 11615, // sniffed
    AQWAR_BCT_SAURFANG_AGGRO8           = 11616, // sniffed
    AQWAR_BCT_SAURFANG_AGGRO9           = 11648, // sniffed

    AQWAR_BCT_SAURFANG_KILLED_UNIT      = 7237,  // sniffed
    AQWAR_BCT_SAURFANG_RAGE             = 11563, // sniffed
    AQWAR_BCT_SAURFANG_BATTLE_WON       = 11651, // sniffed

    AQWAR_SPELL_SF_TERRIFYING_ROAR      = 14100,
    AQWAR_SPELL_SF_CLEAVE               = 16044,
    AQWAR_SPELL_SF_CHARGE               = 15749,
    AQWAR_SPELL_SF_MORTAL_STRIKE        = 24573,
    AQWAR_SPELL_SF_SAURFANG_RAGE        = 26341,

    AQWAR_FACTION_MIGHT_OF_KALIMDOR     = 777,

    AQWAR_MOUNT_DISPLAY_SAURFANG        = 10278
};

static constexpr uint32 AQWarSaurfangSpeech[] =
{
    AQWAR_BCT_SAURFANG_SPEECH1, AQWAR_BCT_SAURFANG_SPEECH2, AQWAR_BCT_SAURFANG_SPEECH3, AQWAR_BCT_SAURFANG_SPEECH4,
    AQWAR_BCT_SAURFANG_SPEECH5, AQWAR_BCT_SAURFANG_SPEECH6, AQWAR_BCT_SAURFANG_SPEECH7, AQWAR_BCT_SAURFANG_SPEECH8,
    AQWAR_BCT_SAURFANG_SPEECH9, AQWAR_BCT_SAURFANG_SPEECH10, AQWAR_BCT_SAURFANG_SPEECH11, AQWAR_BCT_SAURFANG_SPEECH12,
    AQWAR_BCT_SAURFANG_SPEECH13, AQWAR_BCT_SAURFANG_SPEECH14
};

static constexpr uint32 AQWarSaurfangAggro[] =
{
    AQWAR_BCT_SAURFANG_AGGRO1, AQWAR_BCT_SAURFANG_AGGRO2, AQWAR_BCT_SAURFANG_AGGRO3, AQWAR_BCT_SAURFANG_AGGRO4,
    AQWAR_BCT_SAURFANG_AGGRO5, AQWAR_BCT_SAURFANG_AGGRO6, AQWAR_BCT_SAURFANG_AGGRO7, AQWAR_BCT_SAURFANG_AGGRO8,
    AQWAR_BCT_SAURFANG_AGGRO9
};

static constexpr uint32 AQWarSaurfangSpeechCount = 14;

Position const AQWarSaurfangWaveIncomingPosition = { -6985.67f, 956.06f, 10.21f, 2.6f };

std::array<Position, 12> const AQWarSaurfangGatePath
{{
    { -7002.48f, 967.38f, 6.70f, 3.15f },
    { -7205.49f, 967.08f, 0.95f, 2.9f },
    { -7265.48f, 995.34f, 2.55f, 3.16f },
    { -7418.73f, 1000.99f, 0.91f, 2.91f },
    { -7661.01f, 1052.23f, 4.82f, 2.32f },
    { -7759.05f, 1164.64f, 0.02f, 2.22f },
    { -7810.24f, 1275.49f, -11.08f, 2.72f },
    { -7909.35f, 1319.05f, -7.79f, 2.32f },
    { -7952.77f, 1377.95f, 2.94f, 1.38f },
    { -7933.09f, 1490.65f, -6.62f, 2.68f },
    { -8014.01f, 1532.97f, 2.81f, 3.10f },
    { -8079.99f, 1523.19f, 2.61f, 3.15f }
}};

struct classic_npc_aqwar_saurfang : public ScriptedAI
{
    bool m_lastWave;    // set by classic_npc_aqwar_ch_attack before the last wave

    classic_npc_aqwar_saurfang(Creature* creature) : ScriptedAI(creature)
    {
        m_lastWave = false;
        _cenarionHoldAttackWarn = false;
        _finalBattle = false;
        _speechDelay = 120000; // 2 minutes on the first delay to give people time to reach CH
        _speechStep = 0;
        _inSpeech = false;
        _movingToGate = false;

        _movePoint = 0;
        _movePointReached = true;
        _movementPaused = false;

        _mortalStrikeTimer = 0;
        _terrifyingRoarTimer = 0;
        _chargeTimer = 0;
        _cleaveTimer = 0;

        me->SetFaction(AQWAR_FACTION_MIGHT_OF_KALIMDOR);
    }

    void Reset() override
    {
        if (_movingToGate)
            me->Mount(AQWAR_MOUNT_DISPLAY_SAURFANG);

        _mortalStrikeTimer = urand(1, 15) * IN_MILLISECONDS;
        _chargeTimer = urand(0, 4) * IN_MILLISECONDS;
        _cleaveTimer = urand(3, 9) * IN_MILLISECONDS;
        _terrifyingRoarTimer = urand(4, 12) * IN_MILLISECONDS;
    }

    void JustEngagedWith(Unit* who) override
    {
        me->Dismount();
        _movementPaused = true;
        ClassicScriptText(AQWarSaurfangAggro[urand(0, uint32(std::size(AQWarSaurfangAggro)) - 1)], me, who);

        if (me->CastSpell(me, AQWAR_SPELL_SF_SAURFANG_RAGE, false) == SPELL_CAST_OK)
            ClassicScriptText(AQWAR_BCT_SAURFANG_RAGE, me);
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        if (roll_chance(10))
            ClassicScriptText(AQWAR_BCT_SAURFANG_KILLED_UNIT, me);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        if (m_lastWave)
        {
            ClassicScriptText(AQWAR_BCT_SAURFANG_BATTLE_WON, me);
            m_lastWave = false;
        }
        ScriptedAI::EnterEvadeMode(why);
    }

    // VMaNGOS: engages any hostile unit in line of sight (ignores aggro radius)
    void MoveInLineOfSight(Unit* who) override
    {
        if (!who || me->IsEngaged())
            return;

        if (me->HasReactState(REACT_PASSIVE) || me->IsImmuneToPC() || me->IsImmuneToNPC())
            return;

        if (me->IsValidAttackTarget(who) && me->IsHostileTo(who))
        {
            if (who->isInAccessiblePlaceFor(me) && me->IsWithinLOSInMap(who))
                me->EngageWithTarget(who);
        }
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (_movingToGate && pointId < AQWarSaurfangGatePath.size())
        {
            me->SetFacingTo(AQWarSaurfangGatePath[pointId].GetOrientation());
            _movePointReached = true;
        }

        if (_movePoint == AQWarSaurfangGatePath.size())
        {
            _movingToGate = false;
            me->Dismount();
            Position const& last = AQWarSaurfangGatePath[_movePoint - 1];
            me->SetHomePosition(last.GetPositionX(), last.GetPositionY(), last.GetPositionZ(), 2.6f);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsAlive())
            return;

        if (!_cenarionHoldAttackWarn && sGameEventMgr->IsActiveEvent(AQWAR_EVENT_WAR_EFFORT_CH_ATTACK))
        {
            MoveToWaveBattlePosition();
            _cenarionHoldAttackWarn = true;
            return;
        }

        if (!_finalBattle && sGameEventMgr->IsActiveEvent(AQWAR_EVENT_WAR_EFFORT_FINALBATTLE))
        {
            me->SetImmuneToPC(true);
            me->SetImmuneToNPC(true);
            _inSpeech = true;
            _finalBattle = true;
            me->Mount(AQWAR_MOUNT_DISPLAY_SAURFANG);

            // Recover from crash/restart
            if (!_cenarionHoldAttackWarn)
            {
                MoveToWaveBattlePosition();
                _cenarionHoldAttackWarn = true;
            }
        }

        if (_finalBattle)
        {
            if (_inSpeech)
            {
                if (_speechDelay <= diff)
                {
                    if (_speechStep < AQWarSaurfangSpeechCount)
                        ClassicScriptText(AQWarSaurfangSpeech[_speechStep], me);
                    ++_speechStep;
                    _speechDelay = 10000;
                }
                else
                    _speechDelay -= diff;

                if (_speechStep == AQWarSaurfangSpeechCount)
                {
                    // TODO(classic): VMaNGOS sWorld.SendBroadcastTextToWorld(BCT_SAURFANG_FINAL_BATTLE) sends this text to every
                    // player online; TC has no equivalent helper, so it is said by Saurfang with its own chat type instead.
                    ClassicScriptText(AQWAR_BCT_SAURFANG_FINAL_BATTLE, me);
                    _inSpeech = false;
                    _movingToGate = true;
                    me->SetImmuneToPC(false);
                    me->SetImmuneToNPC(false);
                    me->SetWalk(false);
                }
            }
            else if (_movingToGate && _movePointReached && !_movementPaused && _movePoint < AQWarSaurfangGatePath.size())
            {
                Position const& point = AQWarSaurfangGatePath[_movePoint];
                me->GetMotionMaster()->MovePoint(_movePoint, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), true, {}, {},
                    MovementWalkRunSpeedSelectionMode::ForceRun);
                me->SetHomePosition(point);
                ++_movePoint;

                _movePointReached = false;
            }
        }

        if (!UpdateVictim())
            return;

        if (_mortalStrikeTimer < diff)
        {
            if (DoCastVictim(AQWAR_SPELL_SF_MORTAL_STRIKE) == SPELL_CAST_OK)
                _mortalStrikeTimer = urand(11, 20) * IN_MILLISECONDS;
        }
        else
            _mortalStrikeTimer -= diff;

        if (_cleaveTimer < diff)
        {
            if (DoCastVictim(AQWAR_SPELL_SF_CLEAVE) == SPELL_CAST_OK)
                _cleaveTimer = urand(9, 21) * IN_MILLISECONDS;
        }
        else
            _cleaveTimer -= diff;

        if (_chargeTimer < diff)
        {
            if (DoCastVictim(AQWAR_SPELL_SF_CHARGE) == SPELL_CAST_OK)
                _chargeTimer = urand(4, 12) * IN_MILLISECONDS;
        }
        else
            _chargeTimer -= diff;

        if (_terrifyingRoarTimer < diff)
        {
            if (DoCastVictim(AQWAR_SPELL_SF_TERRIFYING_ROAR) == SPELL_CAST_OK)
                _terrifyingRoarTimer = urand(30, 40) * IN_MILLISECONDS;
        }
        else
            _terrifyingRoarTimer -= diff;
    }

    // VMaNGOS JustRespawned(): respawned back at CH, keep running to the gate.
    // TODO(classic): TC recreates the AI on respawn, so the war-effort state below is lost there and is re-derived from
    // the active game events in UpdateAI (the final battle speech would restart after a respawn).
    void JustAppeared() override
    {
        if (_movingToGate)
        {
            _movePoint = 0;
            _movePointReached = true;
            _movementPaused = false;
        }

        ScriptedAI::JustAppeared();
    }

    void MoveToWaveBattlePosition()
    {
        me->SetHomePosition(AQWarSaurfangWaveIncomingPosition);
        me->GetMotionMaster()->MoveTargetedHome();
    }

    void JustReachedHome() override
    {
        _movementPaused = false;
    }

private:
    bool _cenarionHoldAttackWarn;
    bool _finalBattle;
    uint32 _speechDelay;
    uint32 _speechStep;
    bool _inSpeech;
    bool _movingToGate;
    uint32 _movePoint;
    bool _movePointReached;
    bool _movementPaused;

    uint32 _mortalStrikeTimer;
    uint32 _terrifyingRoarTimer;
    uint32 _chargeTimer;
    uint32 _cleaveTimer;
};

// VMaNGOS uses this AI only in Silithus; elsewhere (e.g. Orgrimmar) the creature keeps CreatureEventAI.
// Returning nullptr lets TC pick the default AI for other zones.
// TODO(classic): the non-Silithus Saurfang loses its EventAI/SmartAI because the generated SQL clears AIName for
// every creature using this script name.
static classic_npc_aqwar_saurfang* GetAI_classic_npc_aqwar_saurfang(Creature* creature)
{
    if (creature->GetMap()->GetZoneId(creature->GetPhaseShift(), creature->GetPosition()) == AQWAR_ZONE_SILITHUS)
        return new classic_npc_aqwar_saurfang(creature);

    return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Aeternum: The Scarab Gong (quest 8743 "Bang a Gong!") and a simplified "Ten Hours of War".
// Timeline after the gong (VMaNGOS game events 65/59/61/86 imported as TC game events 120/121/122/123):
//   0h-4h   event 120: crystals, Qiraji invaders, Saurfang at Cenarion Hold
//   4h-8h   event 120 + 121: Cenarion Hold attack (Warcaller Finster waves)
//   8h-10h  event 120 + 122: final battle (Saurfang speech, Anubisaths/Destroyers at the gates)
//   10h+    event 123: post war (Jonathan the Revelator)
// The gong time is kept in a realm world state saved in characters.world_state_value, so the controller recovers
// the right stage after a restart. AQ gates are already open on this realm: no scarab wall/gate handling.
// ---------------------------------------------------------------------------------------------------------------------

enum AQWarTenHours
{
    AQWAR_EVENT_TEN_HOURS_WAR           = 120,
    AQWAR_EVENT_POST_WAR                = 123,

    AQWAR_WORLDSTATE_GONG_TIME          = 1377001,  // unix time the gong was rung, 0 = never
    AQWAR_WORLDSTATE_STAGE              = 1377002,

    AQWAR_QUEST_BANG_A_GONG             = 8743,

    AQWAR_BCT_GONG_RUNG                 = 11427,
    AQWAR_BCT_CRYSTALS_EMERGE           = 11432,
    AQWAR_BCT_WAR_WON                   = 11649,

    AQWAR_BCT_QIRAJI_SUMMONED_1         = 11536,
    AQWAR_BCT_QIRAJI_SUMMONED_2         = 11611,
    AQWAR_BCT_QIRAJI_SUMMONED_3         = 11612,
    AQWAR_BCT_QIRAJI_SUMMONED_4         = 11613,

    AQWAR_NPC_SAURFANG                  = 14720,
    AQWAR_NPC_COLOSSAL_ANUBISATH        = 15743,
    AQWAR_NPC_QIRAJI_DESTROYER          = 15744,
};

enum AQWarStage : int32
{
    AQWAR_STAGE_NONE            = 0,
    AQWAR_STAGE_WAR             = 1,
    AQWAR_STAGE_CH_ATTACK       = 2,
    AQWAR_STAGE_FINAL_BATTLE    = 3,
    AQWAR_STAGE_POST_WAR        = 4
};

// Sends a BroadcastText to every player online (VMaNGOS SendBroadcastTextToWorld), $n = player name
static void AQWarBroadcastToWorld(uint32 broadcastTextId, Player const* player = nullptr)
{
    BroadcastTextEntry const* broadcastText = sBroadcastTextStore.LookupEntry(broadcastTextId);
    if (!broadcastText)
        return;

    std::string text = DB2Manager::GetBroadcastTextValue(broadcastText);
    if (player)
    {
        for (char const* token : { "$n", "$N" })
            for (size_t pos = text.find(token); pos != std::string::npos; pos = text.find(token))
                text.replace(pos, 2, player->GetName());
    }

    sWorld->SendGlobalText(text.c_str(), nullptr);
}

// GO 180717: turning in 8743 starts the war (once per realm)
struct classic_go_scarab_gong : public GameObjectAI
{
    classic_go_scarab_gong(GameObject* go) : GameObjectAI(go) { }

    void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
    {
        if (quest->GetQuestId() != AQWAR_QUEST_BANG_A_GONG)
            return;

        if (WorldStateMgr::GetValue(AQWAR_WORLDSTATE_GONG_TIME, nullptr) != 0)
            return; // the gong has already been rung on this realm

        WorldStateMgr::SetValueAndSaveInDb(AQWAR_WORLDSTATE_GONG_TIME, int32(GameTime::GetGameTime()), true, nullptr);
        AQWarBroadcastToWorld(AQWAR_BCT_GONG_RUNG, player);
    }
};

// Starts/stops the war game events from the gong time (every 5 s, also right after a restart)
class classic_aqwar_war_controller : public WorldScript
{
public:
    classic_aqwar_war_controller() : WorldScript("classic_aqwar_war_controller"), _checkTimer(0) { }

    void OnUpdate(uint32 diff) override
    {
        if (_checkTimer > diff)
        {
            _checkTimer -= diff;
            return;
        }
        _checkTimer = 5 * IN_MILLISECONDS;

        int32 const gongTime = WorldStateMgr::GetValue(AQWAR_WORLDSTATE_GONG_TIME, nullptr);
        if (gongTime <= 0)
            return;

        time_t const elapsed = GameTime::GetGameTime() - time_t(gongTime);
        AQWarStage stage = AQWAR_STAGE_POST_WAR;
        if (elapsed < 4 * HOUR)
            stage = AQWAR_STAGE_WAR;
        else if (elapsed < 8 * HOUR)
            stage = AQWAR_STAGE_CH_ATTACK;
        else if (elapsed < 10 * HOUR)
            stage = AQWAR_STAGE_FINAL_BATTLE;

        SetEventActive(AQWAR_EVENT_TEN_HOURS_WAR, stage != AQWAR_STAGE_POST_WAR);
        SetEventActive(AQWAR_EVENT_WAR_EFFORT_CH_ATTACK, stage == AQWAR_STAGE_CH_ATTACK);
        SetEventActive(AQWAR_EVENT_WAR_EFFORT_FINALBATTLE, stage == AQWAR_STAGE_FINAL_BATTLE);
        SetEventActive(AQWAR_EVENT_POST_WAR, stage == AQWAR_STAGE_POST_WAR);

        int32 const savedStage = WorldStateMgr::GetValue(AQWAR_WORLDSTATE_STAGE, nullptr);
        if (savedStage == stage)
            return;

        if (savedStage == AQWAR_STAGE_NONE && stage != AQWAR_STAGE_POST_WAR)
            AQWarBroadcastToWorld(AQWAR_BCT_CRYSTALS_EMERGE);
        else if (stage == AQWAR_STAGE_POST_WAR)
            AQWarBroadcastToWorld(AQWAR_BCT_WAR_WON);

        WorldStateMgr::SetValueAndSaveInDb(AQWAR_WORLDSTATE_STAGE, stage, true, nullptr);
    }

private:
    static void SetEventActive(uint16 eventId, bool active)
    {
        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
        if (eventId >= events.size() || !events[eventId].isValid())
            return; // SQL 05_aq_scarab_gong_war.sql not applied

        if (sGameEventMgr->IsActiveEvent(eventId) == active)
            return;

        if (active)
            sGameEventMgr->StartEvent(eventId, true);
        else
            sGameEventMgr->StopEvent(eventId, true);
    }

    uint32 _checkTimer;
};

// NPC 15878 Warcaller Finster (event 121): Qiraji waves against Cenarion Hold, VMaNGOS npc_aqwar_cenarionhold_attack
struct classic_npc_aqwar_ch_attack : public ScriptedAI
{
    classic_npc_aqwar_ch_attack(Creature* creature) : ScriptedAI(creature)
    {
        _timeUntilWave = 60 * IN_MILLISECONDS;
        _waveCount = 1;
        me->setActive(true); // keep the waves going even without players nearby
    }

    void UpdateAI(uint32 diff) override
    {
        if (_waveCount == MAX_WAVE_COUNT)
            return;

        if (_timeUntilWave > diff)
        {
            _timeUntilWave -= diff;
            return;
        }

        // 10 mobs per wave, mostly Colossal Anubisaths
        for (uint8 i = 0; i < 10; ++i)
        {
            uint32 entry = urand(0, 5) ? AQWAR_NPC_COLOSSAL_ANUBISATH : AQWAR_NPC_QIRAJI_DESTROYER;
            float x = -7067.77f + irand(-15, 15);
            float y = 966.62f + irand(-15, 15);
            float z = me->GetMap()->GetHeight(me->GetPhaseShift(), x, y, 4.56f + 10.0f);

            if (Creature* summon = me->SummonCreature(entry, Position(x, y, z, 6.0f), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4min))
            {
                summon->SetHomePosition(TargetPos);
                summon->GetMotionMaster()->MovePoint(1, TargetPos);
            }
        }

        static constexpr uint32 waveTexts[] = { AQWAR_BCT_QIRAJI_SUMMONED_1, AQWAR_BCT_QIRAJI_SUMMONED_2, AQWAR_BCT_QIRAJI_SUMMONED_3, AQWAR_BCT_QIRAJI_SUMMONED_4 };
        ClassicScriptText(waveTexts[urand(0, 3)], me);

        _timeUntilWave = 15 * MINUTE * IN_MILLISECONDS;
        ++_waveCount;

        if (_waveCount == MAX_WAVE_COUNT)
            if (Creature* saurfang = me->FindNearestCreature(AQWAR_NPC_SAURFANG, 200.0f))
                if (classic_npc_aqwar_saurfang* saurfangAI = dynamic_cast<classic_npc_aqwar_saurfang*>(saurfang->AI()))
                    saurfangAI->m_lastWave = true;
    }

private:
    static constexpr uint32 MAX_WAVE_COUNT = 12; // VMaNGOS: waves for ~3h, 1h break before the final battle
    static inline Position const TargetPos = { -6959.35f, 940.41f, 14.55f, 3.0f };

    uint32 _timeUntilWave;
    uint32 _waveCount;
};

// NPC 15769 Resonating Crystal (event 120): stationary, Qiraji whispers (mind control) on a random attacker
// when at least two players fight it. Simplified from VMaNGOS npc_resonating_crystal (no wave spawning).
struct classic_npc_resonating_crystal : public ScriptedAI
{
    classic_npc_resonating_crystal(Creature* creature) : ScriptedAI(creature), _whisperSpell(0), _whisperTimer(0)
    {
        SetCombatMovement(false);
    }

    void Reset() override
    {
        _whisperTimer = 2 * IN_MILLISECONDS;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (_whisperTimer <= diff)
        {
            _whisperTimer = 2 * IN_MILLISECONDS;
            if (CountPlayersNearby() >= 2)
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 55.0f, true, true, -int32(GetWhisperSpell())))
                    if (me->CastSpell(target, GetWhisperSpell(), false) == SPELL_CAST_OK)
                        _whisperTimer = 20 * IN_MILLISECONDS;
        }
        else
            _whisperTimer -= diff;

    }

private:
    uint32 GetWhisperSpell()
    {
        if (!_whisperSpell)
        {
            switch (me->GetZoneId())
            {
                case 17:  _whisperSpell = 26259; break; // The Barrens
                case 400: _whisperSpell = 26258; break; // Thousand Needles
                case 405: _whisperSpell = 26195; break; // Desolace
                case 357:                               // Feralas
                case 440:                               // Tanaris
                case 490: _whisperSpell = 26197; break; // Un'Goro Crater
                default:  _whisperSpell = 26198; break; // Silithus
            }
        }
        return _whisperSpell;
    }

    uint32 CountPlayersNearby() const
    {
        uint32 count = 0;
        for (MapReference const& ref : me->GetMap()->GetPlayers())
        {
            Player const* player = ref.GetSource();
            if (player->IsAlive() && !player->IsGameMaster() && me->IsWithinDistInMap(player, 55.0f))
                ++count;
        }
        return count;
    }

    uint32 _whisperSpell;
    uint32 _whisperTimer;
};

void AddSC_classic_world_event_wareffort()
{
    RegisterCreatureAIWithFactory(classic_npc_aqwar_saurfang, GetAI_classic_npc_aqwar_saurfang);
    RegisterCreatureAI(classic_npc_aqwar_ch_attack);
    RegisterCreatureAI(classic_npc_resonating_crystal);
    RegisterGameObjectAI(classic_go_scarab_gong);
    new classic_aqwar_war_controller();
}
