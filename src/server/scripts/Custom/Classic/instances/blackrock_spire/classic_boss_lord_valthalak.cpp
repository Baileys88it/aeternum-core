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

// Aeternum: Lord Valthalak (16042, UBRS, quest chain 8969-8996). Behaviour from the VMaNGOS creature_ai_scripts of
// 16042; on death he leaves the Spirit of Lord Valthalak (16073), ender of 8995 / starter of 8996.
// TC's boss_lord_valthalak is bound to TC's instance script and gets no AI under classic_instance_blackrock_spire.

#include "ScriptMgr.h"
#include "Creature.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

namespace
{
    enum ClassicValthalakData : uint32
    {
        SPELL_GHOST_VISUAL              = 27614,
        SPELL_IMMUNE_ALL                = 29230,
        SPELL_SUMMON_SPECTRAL_ASSASSIN  = 27249,
        SPELL_SHADOW_WRATH              = 27286,
        SPELL_FRENZY                    = 8269,
        SPELL_SHADOW_BOLT_VOLLEY        = 27382,

        NPC_SPIRIT_OF_LORD_VALTHALAK    = 16073,

        EVENT_SUMMON_ASSASSIN           = 1,
        EVENT_SHADOW_WRATH              = 2
    };

    Position const SpiritPos = { 44.71f, -536.0f, 110.0f, 1.56f };
}

struct classic_boss_lord_valthalak : public ScriptedAI
{
    classic_boss_lord_valthalak(Creature* creature) : ScriptedAI(creature), _phase(0) { }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_GHOST_VISUAL, true);
        DoCastSelf(SPELL_IMMUNE_ALL, true);
    }

    void Reset() override
    {
        _events.Reset();
        _phase = 0;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->RemoveAurasDueToSpell(SPELL_GHOST_VISUAL);
        me->RemoveAurasDueToSpell(SPELL_IMMUNE_ALL);
        _events.ScheduleEvent(EVENT_SUMMON_ASSASSIN, 10s);
        _events.ScheduleEvent(EVENT_SHADOW_WRATH, 8s, 12s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (!me->FindNearestCreature(NPC_SPIRIT_OF_LORD_VALTHALAK, 100.0f))
            me->SummonCreature(NPC_SPIRIT_OF_LORD_VALTHALAK, SpiritPos, TEMPSUMMON_TIMED_DESPAWN, 2h);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        // 40%: Frenzy, no more assassins. 15%: permanent Shadow Bolt Volley + Frenzy
        if (_phase == 0 && HealthBelowPct(40))
        {
            _phase = 1;
            _events.CancelEvent(EVENT_SUMMON_ASSASSIN);
            DoCastSelf(SPELL_FRENZY, true);
        }
        else if (_phase == 1 && HealthBelowPct(15))
            _phase = 2;

        if (_phase == 2)
        {
            if (!me->HasAura(SPELL_SHADOW_BOLT_VOLLEY))
                DoCastSelf(SPELL_SHADOW_BOLT_VOLLEY, true);
            if (!me->HasAura(SPELL_FRENZY))
                DoCastSelf(SPELL_FRENZY, true);
        }

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SUMMON_ASSASSIN:
                    DoCastSelf(SPELL_SUMMON_SPECTRAL_ASSASSIN);
                    _events.Repeat(10s);
                    break;
                case EVENT_SHADOW_WRATH:
                    DoCastVictim(SPELL_SHADOW_WRATH);
                    _events.Repeat(8s, 12s);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
    uint8 _phase;
};

void AddSC_classic_boss_lord_valthalak()
{
    RegisterCreatureAI(classic_boss_lord_valthalak);
}
