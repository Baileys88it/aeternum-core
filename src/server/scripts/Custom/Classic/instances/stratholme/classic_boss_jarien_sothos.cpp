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

// Aeternum: Jarien (16101) and Sothos (16102), Stratholme, summoned for the Dungeon Set 2 quest chain.
// Abilities from the VMaNGOS creature_ai_scripts of 16101/16102. When one twin dies the other enrages and the
// dead twin's spirit (16103/16104) fights the survivor; when the last one dies its spirit thanks the players.

#include "ScriptMgr.h"
#include "Creature.h"
#include "ScriptedCreature.h"
#include "StringFormat.h"
#include "TemporarySummon.h"
#include "classic_script_text.h"
#include <iterator>

namespace
{
    enum ClassicJarienSothosData : uint32
    {
        NPC_JARIEN                  = 16101,
        NPC_SOTHOS                  = 16102,
        NPC_SPIRIT_OF_JARIEN        = 16103,
        NPC_SPIRIT_OF_SOTHOS        = 16104,

        // Jarien
        SPELL_CLEAVE                = 15284,
        SPELL_MORTAL_STRIKE         = 16856,
        SPELL_SHADOW_SHOCK          = 22575,
        // Sothos
        SPELL_SHIELD_SLAM           = 15655,
        SPELL_FEAR                  = 27641,
        SPELL_SHADOW_BOLT_VOLLEY    = 27646,
        SPELL_SHIELD_CHARGE         = 15749,
        // survivor
        SPELL_ENRAGE                = 27650,

        BCT_INTRO_1                 = 11971, // Jarien
        BCT_INTRO_2                 = 11972, // Sothos
        BCT_INTRO_3                 = 11973, // Jarien
        BCT_INTRO_4                 = 11974, // Sothos
        BCT_INTRO_5                 = 11975, // Jarien
        BCT_SPIRIT_OF_JARIEN_THANKS = 11819,
        BCT_SPIRIT_OF_SOTHOS_THANKS = 11859,

        EVENT_INTRO                 = 1,
        EVENT_SPELL_1,
        EVENT_SPELL_2,
        EVENT_SPELL_3,
        EVENT_SPELL_4
    };

    uint32 const IntroTexts[] = { BCT_INTRO_1, BCT_INTRO_2, BCT_INTRO_3, BCT_INTRO_4, BCT_INTRO_5 };
}

struct classic_boss_jarien_sothos : public ScriptedAI
{
    classic_boss_jarien_sothos(Creature* creature) : ScriptedAI(creature), _introStep(0)
    {
        _isJarien = me->GetEntry() == NPC_JARIEN;
    }

    void JustAppeared() override
    {
        // Jarien leads the short dialogue right after both are summoned
        if (_isJarien)
            _events.ScheduleEvent(EVENT_INTRO, 2s);
    }

    void Reset() override
    {
        _events.CancelEvent(EVENT_SPELL_1);
        _events.CancelEvent(EVENT_SPELL_2);
        _events.CancelEvent(EVENT_SPELL_3);
        _events.CancelEvent(EVENT_SPELL_4);
    }

    void JustEngagedWith(Unit* who) override
    {
        if (_isJarien)
        {
            _events.ScheduleEvent(EVENT_SPELL_1, 4s, 9s);   // Cleave
            _events.ScheduleEvent(EVENT_SPELL_2, 7s, 12s);  // Mortal Strike
            _events.ScheduleEvent(EVENT_SPELL_3, 7s, 14s);  // Shadow Shock
        }
        else
        {
            _events.ScheduleEvent(EVENT_SPELL_1, 1s);       // Shield Slam
            _events.ScheduleEvent(EVENT_SPELL_2, 10s, 14s); // Fear
            _events.ScheduleEvent(EVENT_SPELL_3, 8s, 10s);  // Shadow Bolt Volley
            _events.ScheduleEvent(EVENT_SPELL_4, 6s, 9s);   // Shield Charge
        }

        if (Creature* partner = GetPartner())
            if (!partner->IsEngaged())
                partner->AI()->AttackStart(who);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        // the spirit of the dead twin only helps during this fight
        if (Creature* spirit = me->FindNearestCreature(GetPartnerSpiritEntry(), 100.0f))
            spirit->DespawnOrUnsummon();

        ScriptedAI::EnterEvadeMode(why);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* partner = GetPartner())
        {
            // "%s goes into a rage after seeing <name> fall in battle!"
            partner->CastSpell(partner, SPELL_ENRAGE, true);
            partner->TextEmote(Trinity::StringFormat("%s goes into a rage after seeing {} fall in battle!", me->GetName()));

            if (Creature* spirit = me->SummonCreature(GetSpiritEntry(), me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 5min))
                spirit->AI()->AttackStart(partner);
            return;
        }

        // last twin down: the other spirit leaves, this one thanks the players
        if (Creature* spirit = me->FindNearestCreature(GetPartnerSpiritEntry(), 100.0f))
            spirit->DespawnOrUnsummon();

        if (Creature* spirit = me->SummonCreature(GetSpiritEntry(), me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 1min))
            ClassicScriptText(_isJarien ? BCT_SPIRIT_OF_JARIEN_THANKS : BCT_SPIRIT_OF_SOTHOS_THANKS, spirit);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        if (!me->IsEngaged())
        {
            if (_events.ExecuteEvent() == EVENT_INTRO && _introStep < std::size(IntroTexts))
            {
                Creature* speaker = (_introStep % 2 == 0) ? me : GetPartner();
                if (speaker)
                    ClassicScriptText(IntroTexts[_introStep], speaker);
                if (++_introStep < std::size(IntroTexts))
                    _events.ScheduleEvent(EVENT_INTRO, 4s);
            }
            return;
        }

        if (!UpdateVictim())
            return;

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (_isJarien)
                ExecuteJarienEvent(eventId);
            else
                ExecuteSothosEvent(eventId);
        }
    }

private:
    void ExecuteJarienEvent(uint32 eventId)
    {
        switch (eventId)
        {
            case EVENT_SPELL_1:
                DoCastVictim(SPELL_CLEAVE);
                _events.Repeat(7s, 11s);
                break;
            case EVENT_SPELL_2:
                DoCastVictim(SPELL_MORTAL_STRIKE);
                _events.Repeat(12s, 15s);
                break;
            case EVENT_SPELL_3:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                    DoCast(target, SPELL_SHADOW_SHOCK);
                _events.Repeat(12s, 17s);
                break;
            default:
                break;
        }
    }

    void ExecuteSothosEvent(uint32 eventId)
    {
        switch (eventId)
        {
            case EVENT_SPELL_1:
                DoCastVictim(SPELL_SHIELD_SLAM);
                _events.Repeat(5s);
                break;
            case EVENT_SPELL_2:
                DoCastVictim(SPELL_FEAR);
                _events.Repeat(10s, 14s);
                break;
            case EVENT_SPELL_3:
                DoCastSelf(SPELL_SHADOW_BOLT_VOLLEY);
                _events.Repeat(14s, 16s);
                break;
            case EVENT_SPELL_4:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                    DoCast(target, SPELL_SHIELD_CHARGE);
                _events.Repeat(6s, 9s);
                break;
            default:
                break;
        }
    }

    Creature* GetPartner() const
    {
        return me->FindNearestCreature(_isJarien ? NPC_SOTHOS : NPC_JARIEN, 100.0f);
    }

    uint32 GetSpiritEntry() const { return _isJarien ? NPC_SPIRIT_OF_JARIEN : NPC_SPIRIT_OF_SOTHOS; }
    uint32 GetPartnerSpiritEntry() const { return _isJarien ? NPC_SPIRIT_OF_SOTHOS : NPC_SPIRIT_OF_JARIEN; }

    EventMap _events;
    uint8 _introStep;
    bool _isJarien;
};

void AddSC_classic_boss_jarien_sothos()
{
    RegisterCreatureAI(classic_boss_jarien_sothos); // ScriptName shared by 16101 and 16102
}
