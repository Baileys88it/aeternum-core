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

// Aeternum: Mor Grayhoof (16080, UBRS, summoned for quest 8996 chain / Dungeon Set 2). Simple port of the VMaNGOS
// creature_ai_scripts of 16080: caster -> Bear Form (76%) -> Cat Form (51%) -> Faerie Dragon Form (26%).

#include "ScriptMgr.h"
#include "Creature.h"
#include "ScriptedCreature.h"

namespace
{
    enum ClassicMorGrayhoofData : uint32
    {
        SPELL_GHOST_VISUAL          = 27614,
        SPELL_IMMUNE_ALL            = 29230,

        // caster
        SPELL_MOONFIRE              = 27737,
        SPELL_HURRICANE             = 27530,
        SPELL_REJUVENATION          = 27532,
        SPELL_HEALING_TOUCH         = 27527,
        // bear
        SPELL_BEAR_FORM             = 27543,
        SPELL_SWIPE                 = 27554,
        SPELL_MAUL                  = 27553,
        SPELL_DEMORALIZING_ROAR     = 27551,
        // cat
        SPELL_CAT_FORM              = 27545,
        SPELL_SHRED                 = 27555,
        SPELL_RAKE                  = 27556,
        // faerie dragon
        SPELL_FAERIE_DRAGON_FORM    = 27546,
        SPELL_ARCANE_EXPLOSION      = 22271,
        SPELL_REFLECTION            = 27564,

        PHASE_CASTER                = 1,
        PHASE_BEAR                  = 2,
        PHASE_CAT                   = 3,
        PHASE_DRAGON                = 4,

        EVENT_MOONFIRE              = 1,
        EVENT_HURRICANE,
        EVENT_HEAL,
        EVENT_SWIPE,
        EVENT_MAUL,
        EVENT_DEMORALIZING_ROAR,
        EVENT_SHRED,
        EVENT_RAKE,
        EVENT_ARCANE_EXPLOSION,
        EVENT_REFLECTION
    };
}

struct classic_boss_mor_grayhoof : public ScriptedAI
{
    classic_boss_mor_grayhoof(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_GHOST_VISUAL, true);
        DoCastSelf(SPELL_IMMUNE_ALL, true);
    }

    void Reset() override
    {
        _events.Reset();
        me->RemoveAurasDueToSpell(SPELL_BEAR_FORM);
        me->RemoveAurasDueToSpell(SPELL_CAT_FORM);
        me->RemoveAurasDueToSpell(SPELL_FAERIE_DRAGON_FORM);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->RemoveAurasDueToSpell(SPELL_GHOST_VISUAL);
        me->RemoveAurasDueToSpell(SPELL_IMMUNE_ALL);
        _events.SetPhase(PHASE_CASTER);
        _events.ScheduleEvent(EVENT_MOONFIRE, 7s, 10s, 0, PHASE_CASTER);
        _events.ScheduleEvent(EVENT_HURRICANE, 15s, 20s, 0, PHASE_CASTER);
        _events.ScheduleEvent(EVENT_HEAL, 2s, 0, PHASE_CASTER);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (_events.IsInPhase(PHASE_CASTER) && HealthBelowPct(76))
        {
            EnterPhase(PHASE_BEAR, SPELL_BEAR_FORM);
            _events.ScheduleEvent(EVENT_SWIPE, 6s, 8s, 0, PHASE_BEAR);
            _events.ScheduleEvent(EVENT_MAUL, 4s, 6s, 0, PHASE_BEAR);
            _events.ScheduleEvent(EVENT_DEMORALIZING_ROAR, 2s, 10s, 0, PHASE_BEAR);
        }
        else if (_events.IsInPhase(PHASE_BEAR) && HealthBelowPct(51))
        {
            me->RemoveAurasDueToSpell(SPELL_BEAR_FORM);
            EnterPhase(PHASE_CAT, SPELL_CAT_FORM);
            _events.ScheduleEvent(EVENT_SHRED, 6s, 10s, 0, PHASE_CAT);
            _events.ScheduleEvent(EVENT_RAKE, 4s, 9s, 0, PHASE_CAT);
        }
        else if (_events.IsInPhase(PHASE_CAT) && HealthBelowPct(26))
        {
            me->RemoveAurasDueToSpell(SPELL_CAT_FORM);
            EnterPhase(PHASE_DRAGON, SPELL_FAERIE_DRAGON_FORM);
            me->SetSheath(SHEATH_STATE_UNARMED);
            _events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, 6s, 9s, 0, PHASE_DRAGON);
            _events.ScheduleEvent(EVENT_REFLECTION, 12s, 15s, 0, PHASE_DRAGON);
        }

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_MOONFIRE:
                    DoCastVictim(SPELL_MOONFIRE);
                    _events.Repeat(10s, 13s);
                    break;
                case EVENT_HURRICANE:
                    DoCastVictim(SPELL_HURRICANE);
                    _events.Repeat(15s, 20s);
                    break;
                case EVENT_HEAL:
                    if (HealthBelowPct(90))
                    {
                        if (!me->HasAura(SPELL_REJUVENATION))
                            DoCastSelf(SPELL_REJUVENATION, true);
                        DoCastSelf(SPELL_HEALING_TOUCH);
                        _events.Repeat(8s, 13s);
                    }
                    else
                        _events.Repeat(2s);
                    break;
                case EVENT_SWIPE:
                    DoCastVictim(SPELL_SWIPE);
                    _events.Repeat(6s, 8s);
                    break;
                case EVENT_MAUL:
                    DoCastVictim(SPELL_MAUL);
                    _events.Repeat(4s, 6s);
                    break;
                case EVENT_DEMORALIZING_ROAR:
                    DoCastSelf(SPELL_DEMORALIZING_ROAR);
                    _events.Repeat(15s, 20s);
                    break;
                case EVENT_SHRED:
                    DoCastVictim(SPELL_SHRED);
                    _events.Repeat(6s, 10s);
                    break;
                case EVENT_RAKE:
                    DoCastVictim(SPELL_RAKE);
                    _events.Repeat(4s, 9s);
                    break;
                case EVENT_ARCANE_EXPLOSION:
                    DoCastSelf(SPELL_ARCANE_EXPLOSION);
                    _events.Repeat(6s, 9s);
                    break;
                case EVENT_REFLECTION:
                    DoCastSelf(SPELL_REFLECTION);
                    _events.Repeat(12s, 15s);
                    break;
                default:
                    break;
            }
        }
    }

private:
    void EnterPhase(uint8 phase, uint32 formSpell)
    {
        me->InterruptNonMeleeSpells(false);
        _events.SetPhase(phase);
        DoCastSelf(formSpell, true);
    }

    EventMap _events;
};

void AddSC_classic_boss_mor_grayhoof()
{
    RegisterCreatureAI(classic_boss_mor_grayhoof);
}
