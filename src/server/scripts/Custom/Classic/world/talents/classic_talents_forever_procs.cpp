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

// Classic 1.60 (client 1.60.1.70888) talent procs whose client data has no server side behaviour: dummy auras with no handler, proc
// trigger auras whose talent chance is an effect amount (trait rank curve) while the client proc chance is 100, and dummy trigger
// spells (18350, 12162) that nothing finishes. Behaviour follows the 70888 tooltips; rank values are always read from the aura
// amounts the core computes from the trait curves, never hardcoded. Proc filters live in spell_proc (sql/custom/world).
// Several scripts follow the open source Forever emulator (TrinityCore "forever" tree, GPL-2.0) and VMaNGOS (GPL-2.0) behaviour.

#include "ScriptMgr.h"
#include "Formulas.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

namespace
{
enum ForeverTalentProcSpells : uint32
{
    SPELL_MAGE_WINTERS_CHILL_TALENT     = 11180,   // effect 1: proc chance
    SPELL_MAGE_WINTERS_CHILL            = 12579,   // effect 1: % spell power for the caster's next Cone of Cold, per stack
    SPELL_MAGE_MAGIC_ABSORPTION_MANA    = 29442,
    SPELL_SHAMAN_ANCESTRAL_FORTITUDE    = 16177,
    SPELL_WARLOCK_SHADOW_TRANCE         = 17941,
    SPELL_WARLOCK_PYROCLASM_STUN        = 18093,
    SPELL_DRUID_NATURAL_REACTION_RAGE   = 417053,
    SPELL_PRIEST_HOLY_BINDING           = 1328569,
    SPELL_PRIEST_HOLY_BINDING_HEAL      = 1328570,
    SPELL_PALADIN_GUARDED_MANA          = 415058,
    SPELL_PALADIN_ILLUMINATION_MANA     = 20272,
    SPELL_WARRIOR_DEEP_WOUND            = 412609,  // periodic dummy (bleed), amount = damage per tick
    SPELL_WARRIOR_DEEP_WOUND_DAMAGE     = 412613,
    SPELL_HUNTER_RAPID_KILLING          = 415405,  // effect 1: next Shot bonus
    SPELL_HUNTER_RAPID_KILLING_BUFF     = 415407
};

constexpr uint32 MAGE_FLAG0_FIREBALL        = 0x00000001;
constexpr uint32 MAGE_FLAG0_FROSTBOLT       = 0x00000020;   // Frostbolt (and Frost Blast, which shares the whole mask)
constexpr uint32 MAGE_FLAG0_AM_CHANNEL      = 0x00000800;
constexpr uint32 MAGE_FLAG0_AM_MISSILE      = 0x00200000;
constexpr uint32 MAGE_FLAG0_ARCANE_BLAST    = 0x20000000;
constexpr uint32 MAGE_FLAG1_FROSTFIRE_BOLT  = 0x00001000;
constexpr uint32 WARRIOR_FLAG0_REND         = 0x00000020;
constexpr uint32 HUNTER_FLAG0_SERPENT_STING = 0x00004000;

bool IsMageSpell(SpellInfo const* spellInfo, uint32 flags0, uint32 flags1 = 0)
{
    return spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_MAGE
        && ((spellInfo->SpellFamilyFlags[0] & flags0) || (spellInfo->SpellFamilyFlags[1] & flags1));
}

bool IsArcaneWrath(SpellInfo const* spellInfo)
{
    switch (spellInfo->Id)
    {
        case 1326079: case 1326080: case 1326081: case 1326082:
            return true;
        default:
            return false;
    }
}

// Number of missiles an Arcane Missiles channel fires: duration / period of its periodic trigger effect (3 for rank 1, 5 later).
uint32 GetArcaneMissilesBolts(SpellInfo const* channel)
{
    if (channel)
        for (SpellEffectInfo const& effect : channel->GetEffects())
            if (effect.IsAura(SPELL_AURA_PERIODIC_TRIGGER_SPELL) && effect.ApplyAuraPeriod > 0 && channel->GetMaxDuration() > 0)
                return std::max<uint32>(1, uint32(channel->GetMaxDuration()) / effect.ApplyAuraPeriod);
    return 5;
}
}

/*######
## Mage
######*/

// 400588 - Missile Barrage: "Gives your Arcane Blast spell a $m1% chance, and your Arcane Wrath, Fireball, Frostbolt, and Frostfire Bolt
// spells a ${$m1/2}% chance to [grant 400589]". $m1 is effect 0 (20/40 from the rank curve). Arcane Missiles never rolls (70888 tooltip).
// spell_proc: harmful damage spell hits of those spells; the roll per spell is here.
class classic_spell_mage_missile_barrage : public AuraScript
{
    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo || spellInfo->SpellFamilyName != SPELLFAMILY_MAGE)
            return false;

        float const chance = float(aurEff->GetAmount());
        if (IsMageSpell(spellInfo, MAGE_FLAG0_ARCANE_BLAST))
            return roll_chance(chance);

        if (IsArcaneWrath(spellInfo) || IsMageSpell(spellInfo, MAGE_FLAG0_FIREBALL | MAGE_FLAG0_FROSTBOLT, MAGE_FLAG1_FROSTFIRE_BOLT))
            return roll_chance(chance / 2.0f);

        return false;
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_spell_mage_missile_barrage::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 11213 - Arcane Concentration: "Gives you a $m1% chance of entering a Clearcasting state after any damage spell hits a target." $m1 is
// effect 0 (2/4/6/8/10). Blizzard notes of 10 November 2026: "Arcane Concentration's activation chance now correctly splits across the
// cast and every bolt" of Arcane Missiles. So the channel hit and each missile roll $m1 / (missiles + 1); every other direct damage spell
// rolls $m1. Triggered spells other than the missiles still do not roll (spell_proc lets triggered spells reach the script only for that).
class classic_spell_mage_arcane_concentration : public AuraScript
{
    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo)
            return false;

        float const chance = float(aurEff->GetAmount());
        bool const isChannel = IsMageSpell(spellInfo, MAGE_FLAG0_AM_CHANNEL);
        bool const isMissile = IsMageSpell(spellInfo, MAGE_FLAG0_AM_MISSILE);
        if (isChannel || isMissile)
        {
            SpellInfo const* channel = spellInfo;
            if (isMissile)
            {
                Spell const* current = GetTarget()->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
                channel = current ? current->GetSpellInfo() : nullptr;
            }
            return roll_chance(chance / float(GetArcaneMissilesBolts(channel) + 1));
        }

        Spell const* procSpell = eventInfo.GetProcSpell();
        if (procSpell && procSpell->IsTriggered())
            return false;

        if (!(eventInfo.GetSpellTypeMask() & PROC_SPELL_TYPE_DAMAGE))
            return false;

        return roll_chance(chance);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_spell_mage_arcane_concentration::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 11180 - Winter's Chill: "Gives your Frostbolt and Fingers of Frost empowered Ice Lance spells a $m2% chance to apply Winter's Chill
// [12579] ... stacking up to $s1 times." Effect 0 is the proc trigger (amount 5 = stacks), effect 1 the chance (20..100 by rank). The
// client gives the talent one proc charge, which would remove the passive talent on its first proc: it keeps none. spell_proc: Frostbolt
// damage hits (family flag 0x20). The empowered Ice Lance part is not handled here.
class classic_spell_mage_winters_chill : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_WINTERS_CHILL }) && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetAura()->SetCharges(0);
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        AuraEffect const* chance = GetEffect(EFFECT_1);
        Unit* target = eventInfo.GetActionTarget();
        if (!chance || !target || target == GetTarget() || !roll_chance(float(chance->GetAmount())))
            return;

        GetTarget()->CastSpell(target, SPELL_MAGE_WINTERS_CHILL, true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(classic_spell_mage_winters_chill::HandleApply, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        OnEffectProc += AuraEffectProcFn(classic_spell_mage_winters_chill::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Cone of Cold (120, 8492, 10159, 10160, 10161) with the caster's Winter's Chill 12579 on the target: effect 0 of the debuff (damage taken
// from the caster's Cone of Cold, per stack) is the core's; effect 1 adds that % of the caster's Frost spell power, and the Cone of Cold
// spends the debuff ("your next Cone of Cold").
class classic_spell_mage_cone_of_cold_winters_chill : public SpellScript
{
    void HandleCalcDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit* victim, int32& /*damage*/, int32& flatMod, float& /*pctMod*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !victim)
            return;

        if (AuraEffect const* bonus = victim->GetAuraEffect(SPELL_MAGE_WINTERS_CHILL, EFFECT_1, caster->GetGUID()))
            flatMod += int32(float(caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FROST)) * float(bonus->GetAmount()) / 100.0f);
    }

    void SpendWintersChill()
    {
        Unit* target = GetHitUnit();
        if (target && target->HasAura(SPELL_MAGE_WINTERS_CHILL, GetCaster()->GetGUID()))
            target->RemoveAurasDueToSpell(SPELL_MAGE_WINTERS_CHILL, GetCaster()->GetGUID());
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(classic_spell_mage_cone_of_cold_winters_chill::HandleCalcDamage);
        AfterHit += SpellHitFn(classic_spell_mage_cone_of_cold_winters_chill::SpendWintersChill);
    }
};

// 29074 - Master of Elements: "Your Fire and Frost critical strikes will refund $s1% of their base mana cost." Effect 0 is a dummy with
// the percent (10/20/30). spell_proc: Fire and Frost harmful spell critical damage hits; the mana the spell cost is given back.
class classic_spell_mage_master_of_elements : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell const* spell = eventInfo.GetProcSpell();
        return spell && spell->GetPowerTypeCostAmount(POWER_MANA).value_or(0) > 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        int32 const cost = eventInfo.GetProcSpell()->GetPowerTypeCostAmount(POWER_MANA).value_or(0);
        int32 const amount = CalculatePct(cost, aurEff->GetAmount());
        if (amount > 0)
            GetTarget()->EnergizeBySpell(GetTarget(), GetSpellInfo(), amount, POWER_MANA);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_spell_mage_master_of_elements::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_spell_mage_master_of_elements::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 29441 - Magic Absorption: "Increases all your resistances by $s2 and causes all spells you fully resist to restore $s1% of your total
// mana. Cannot trigger more often than 1 time per sec." Effect 0 is a dummy with the percent (1/2), effect 1 the resistances (core).
// spell_proc: harmful spells taken that were fully resisted (client cooldown 1 s); 29442 is the mana energize.
class classic_spell_mage_magic_absorption : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGE_MAGIC_ABSORPTION_MANA });
    }

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return GetTarget()->GetPowerType() == POWER_MANA || GetTarget()->GetMaxPower(POWER_MANA) > 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        Unit* mage = GetTarget();
        int32 const amount = CalculatePct(mage->GetMaxPower(POWER_MANA), aurEff->GetAmount());
        if (amount > 0)
            mage->EnergizeBySpell(mage, sSpellMgr->AssertSpellInfo(SPELL_MAGE_MAGIC_ABSORPTION_MANA, DIFFICULTY_NONE), amount, POWER_MANA);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_spell_mage_magic_absorption::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_spell_mage_magic_absorption::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

/*######
## Generic: proc trigger whose talent chance is the trigger effect's own amount
######*/

// 1224716 Puncturing Wounds (effect 1, Backstab extra combo point), 440529 Resourcefulness (effect 1), 16850 Improved Starfire (effect 1,
// stun), 18119 Aftermath (effect 0, daze on Conflagrate): the client proc chance is 100 and the rank chance is the amount of the proc
// trigger effect itself. Each proc trigger effect rolls its own amount; spell_proc limits the spells.
class classic_spell_proc_trigger_amount_chance : public AuraScript
{
    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        return roll_chance(float(aurEff->GetAmount()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_spell_proc_trigger_amount_chance::CheckProc, EFFECT_ALL, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

/*######
## Shaman
######*/

// 16176 - Ancestral Healing: "Increases your target's armor value by $m1% for $16177d after getting a critical effect from one of your
// healing spells." Effect 0 is a dummy with the percent (8/17/25) and 16177 as trigger spell that nothing cast. spell_proc: critical
// direct heals (triggered heals too); 16177 is cast on the healed target with the talent amount.
class classic_spell_sha_ancestral_healing : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SHAMAN_ANCESTRAL_FORTITUDE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        HealInfo* healInfo = eventInfo.GetHealInfo();
        return healInfo && healInfo->GetHeal() && eventInfo.GetActionTarget() && (eventInfo.GetHitMask() & PROC_HIT_CRITICAL);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (aurEff->GetAmountAsInt() <= 0)
            return;

        GetTarget()->CastSpell(eventInfo.GetActionTarget(), SPELL_SHAMAN_ANCESTRAL_FORTITUDE, CastSpellExtraArgs(aurEff).AddSpellBP0(aurEff->GetAmountAsInt()));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_spell_sha_ancestral_healing::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_spell_sha_ancestral_healing::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

/*######
## Warlock
######*/

// 18094 - Nightfall: "Gives your Corruption, Drain Soul, Drain Life, and Wrack spells a $m1% chance to cause you to enter a Shadow Trance
// after damaging the opponent." Effect 0 is a dummy with the chance (2/4); nothing cast Shadow Trance 17941. Each periodic damage tick of
// the four spells is one roll (Corruption [0] 0x2, Drain Life [0] 0x8, Drain Soul [0] 0x4000, Wrack 1316697 [1] 0x40000).
class classic_talent_warl_nightfall : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_SHADOW_TRANCE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK
            && ((spellInfo->SpellFamilyFlags[0] & 0x400A) || (spellInfo->SpellFamilyFlags[1] & 0x40000));
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        if (!roll_chance(float(aurEff->GetAmount())))
            return;

        GetTarget()->CastSpell(GetTarget(), SPELL_WARLOCK_SHADOW_TRANCE, CastSpellExtraArgs(aurEff));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_talent_warl_nightfall::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_talent_warl_nightfall::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 18073 - Pyroclasm: "Gives your Soul Fire spell a $s1% chance to Stun the target for $18093d, and your Rain of Fire and Hellfire spells a
// $s1% chance over their duration to Stun targets they damage." Effect 0 is a dummy with the chance (13/26); effect 1 triggers 18350, a
// dummy-only spell nothing handled (disabled in spell_proc). Soul Fire and Hellfire share family flag 0x40, Rain of Fire is 0x20.
class classic_talent_warl_pyroclasm : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_PYROCLASM_STUN });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellInfo->SpellFamilyFlags[0] & 0x60)
            && eventInfo.GetActionTarget() && eventInfo.GetActionTarget() != GetTarget();
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        if (!roll_chance(float(aurEff->GetAmount())))
            return;

        GetTarget()->CastSpell(eventInfo.GetActionTarget(), SPELL_WARLOCK_PYROCLASM_STUN, CastSpellExtraArgs(aurEff));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_talent_warl_pyroclasm::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_talent_warl_pyroclasm::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

/*######
## Druid
######*/

// 417051 - Natural Reaction: "Increases your dodge chance by $m1%, and gives you a $m2% chance to gain ${$417053m1/10} Rage each time you
// dodge." $m2 is effect 1, a dummy with no handler. spell_proc: melee swings and melee abilities taken that were dodged; 417053 gives
// the rage.
class classic_talent_dru_natural_reaction : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_DRUID_NATURAL_REACTION_RAGE }) && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        return (eventInfo.GetHitMask() & PROC_HIT_DODGE) && roll_chance(float(aurEff->GetAmount()));
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_DRUID_NATURAL_REACTION_RAGE, CastSpellExtraArgs(aurEff));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_talent_dru_natural_reaction::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(classic_talent_dru_natural_reaction::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

/*######
## Priest
######*/

// Flash Heal (2061, 9472, 9473, 9474, 10915, 10916, 10917) with 1328569 Holy Binding: "Each time you heal a target other than yourself with
// Flash Heal, you are healed for $s1% as much. This healing generates no threat." The self heal is 1328570 (spell_threat pctMod 0).
class classic_spell_pri_holy_binding : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PRIEST_HOLY_BINDING, SPELL_PRIEST_HOLY_BINDING_HEAL });
    }

    void HandleAfterHit()
    {
        Unit* priest = GetCaster();
        Unit* target = GetHitUnit();
        if (!priest || !target || target == priest || GetHitHeal() <= 0)
            return;

        AuraEffect const* talent = priest->GetAuraEffect(SPELL_PRIEST_HOLY_BINDING, EFFECT_0);
        if (!talent)
            return;

        int32 const amount = CalculatePct(GetHitHeal(), talent->GetAmount());
        if (amount > 0)
            priest->CastSpell(priest, SPELL_PRIEST_HOLY_BINDING_HEAL, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(amount));
    }

    void Register() override
    {
        AfterHit += SpellHitFn(classic_spell_pri_holy_binding::HandleAfterHit);
    }
};

/*######
## Paladin
######*/

// 415059 - Guarded by the Light: "You gain $415058m1% of your maximum mana every $415058t1 sec for $415058d after Blocking a melee attack.
// This effect is cancelled by casting Flash of Light, Holy Light, or Holy Shock." The talent is a dummy aura; spell_proc: melee swings and
// abilities taken that were blocked. 415058 is the mana aura.
class classic_spell_pal_guarded_by_the_light : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_GUARDED_MANA });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_GUARDED_MANA, CastSpellExtraArgs(aurEff));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(classic_spell_pal_guarded_by_the_light::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Flash of Light, Holy Light and Holy Shock ranks: casting one cancels the Guarded by the Light mana aura 415058.
class classic_spell_pal_guarded_by_the_light_cancel : public SpellScript
{
    void HandleCast()
    {
        GetCaster()->RemoveAurasDueToSpell(SPELL_PALADIN_GUARDED_MANA);
    }

    void Register() override
    {
        OnCast += SpellCastFn(classic_spell_pal_guarded_by_the_light_cancel::HandleCast);
    }
};

// 20210 - Illumination: "After getting a critical effect from your Flash of Light, Holy Light, Light's Vigil, or Holy Shock heal spell you
// have a $m1% chance to gain Mana equal to $m3% of the base cost of the spell." $m1 is effect 0 (20..100 by rank, a proc trigger of the
// dummy-only 18350), $m3 is effect 2 (50). spell_proc: critical heals of those spells. The mana is given with 20272 as the source spell
// (energize only; its 70888 regen aura is not applied). A heal with no mana cost of its own (triggered Holy Shock heal) gives nothing.
class classic_spell_pal_illumination : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_ILLUMINATION_MANA }) && ValidateSpellEffect({ { spellInfo->Id, EFFECT_2 } });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Spell const* spell = eventInfo.GetProcSpell();
        return spell && spell->GetPowerTypeCostAmount(POWER_MANA).value_or(0) > 0 && roll_chance(float(aurEff->GetAmount()));
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        AuraEffect const* pct = GetEffect(EFFECT_2);
        int32 const cost = eventInfo.GetProcSpell()->GetPowerTypeCostAmount(POWER_MANA).value_or(0);
        int32 const amount = CalculatePct(cost, pct ? pct->GetAmount() : 50.0f);
        if (amount > 0)
            GetTarget()->EnergizeBySpell(GetTarget(), sSpellMgr->AssertSpellInfo(SPELL_PALADIN_ILLUMINATION_MANA, DIFFICULTY_NONE), amount, POWER_MANA);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_spell_pal_illumination::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(classic_spell_pal_illumination::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

/*######
## Warrior
######*/

// 12834 - Deep Wounds: "Your critical strikes cause your opponent to Bleed, dealing $m1% of your melee weapon's average damage over
// $412609d." Effect 0 is a proc trigger of 12162 (a dummy nothing finished) with the percent (20/40/60). spell_proc: melee swing and
// melee ability critical hits. The bleed 412609 (periodic dummy, 12 s) gets the percent of the main hand average damage spread over
// its ticks; a new critical strike refreshes it (no stacking). Each tick deals its amount through 412613. Vanilla: VMaNGOS.
class classic_spell_warr_deep_wounds : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_DEEP_WOUND, SPELL_WARRIOR_DEEP_WOUND_DAMAGE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* victim = eventInfo.GetActionTarget();
        return victim && victim != GetTarget() && victim->IsAlive() && (eventInfo.GetHitMask() & PROC_HIT_CRITICAL);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* warrior = GetTarget();
        Unit* victim = eventInfo.GetActionTarget();
        SpellInfo const* bleed = sSpellMgr->GetSpellInfo(SPELL_WARRIOR_DEEP_WOUND, DIFFICULTY_NONE);
        if (!victim || !bleed || aurEff->GetAmountAsInt() <= 0)
            return;

        int32 const period = int32(bleed->GetEffect(EFFECT_0).ApplyAuraPeriod);
        if (period <= 0 || bleed->GetMaxDuration() <= 0)
            return;

        float minDamage = 0.0f;
        float maxDamage = 0.0f;
        warrior->CalculateMinMaxDamage(BASE_ATTACK, false, true, minDamage, maxDamage);
        float const total = (minDamage + maxDamage) * 0.5f * float(aurEff->GetAmountAsInt()) / 100.0f;
        int32 const ticks = std::max(1, bleed->GetMaxDuration() / period);
        int32 const perTick = std::max(1, int32(total / float(ticks)));

        victim->RemoveAurasDueToSpell(SPELL_WARRIOR_DEEP_WOUND, warrior->GetGUID());
        warrior->CastSpell(victim, SPELL_WARRIOR_DEEP_WOUND, CastSpellExtraArgs(aurEff).AddSpellBP0(perTick));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(classic_spell_warr_deep_wounds::CheckProc);
        OnEffectProc += AuraEffectProcFn(classic_spell_warr_deep_wounds::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 412609 - Deep Wound (bleed): every tick deals the aura amount through 412613.
class classic_spell_warr_deep_wound_bleed : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_DEEP_WOUND_DAMAGE });
    }

    void HandleTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        Unit* warrior = GetCaster();
        Unit* victim = GetTarget();
        if (!warrior || !victim->IsAlive() || aurEff->GetAmountAsInt() <= 0)
            return;

        warrior->CastSpell(victim, SPELL_WARRIOR_DEEP_WOUND_DAMAGE, CastSpellExtraArgs(aurEff).AddSpellBP0(aurEff->GetAmountAsInt()));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(classic_spell_warr_deep_wound_bleed::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 1289682 - Bloodthrill: "Grants your Main Hand melee attacks against enemies afflicted by your Rend a $s1% chance to allow an activation
// of Overpower." Effect 0 is the proc trigger of 1282733 (the Overpower activation the dodge state uses) with the chance (10/20). It now
// needs the warrior's own Rend (family flag 0x20) on the target and a main hand attack, and rolls the amount.
class classic_spell_warr_bloodthrill : public AuraScript
{
    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Unit* target = eventInfo.GetActionTarget();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!target || target == GetTarget() || !damageInfo || damageInfo->GetAttackType() != BASE_ATTACK)
            return false;

        if (!target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARRIOR, flag128(WARRIOR_FLAG0_REND, 0, 0, 0), GetTarget()->GetGUID()))
            return false;

        return roll_chance(float(aurEff->GetAmount()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(classic_spell_warr_bloodthrill::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

/*######
## Hunter
######*/

// 415405 - Rapid Killing: "when you kill a non-trivial enemy or it dies while afflicted by your Serpent Sting, you gain Rapid Killing,
// increasing the damage of your next Shot ability within $415407d by $m2%." Effect 1 is that bonus as a dummy; nothing gave 415407. A kill
// by the hunter or the hunter's pet, or the death of a target with the hunter's Serpent Sting (family flag 0x4000), casts 415407 with the
// talent amount. Non-trivial: a player, or a creature that gives the hunter experience. (415407 is spent by its own proc data and the
// existing classic_spell_hun_rapid_killing_consumed script.)
class classic_unit_hun_rapid_killing : public UnitScript
{
public:
    classic_unit_hun_rapid_killing() : UnitScript("classic_unit_hun_rapid_killing") { }

    static void TryGrant(Player* hunter, Unit* victim)
    {
        if (!hunter || !victim || hunter == victim)
            return;

        AuraEffect const* talent = hunter->GetAuraEffect(SPELL_HUNTER_RAPID_KILLING, EFFECT_1);
        if (!talent || talent->GetAmountAsInt() <= 0)
            return;

        if (!victim->IsPlayer())
        {
            Creature* creature = victim->ToCreature();
            if (!creature || !Trinity::XP::Gain(hunter, creature))
                return;
        }

        hunter->CastSpell(hunter, SPELL_HUNTER_RAPID_KILLING_BUFF, CastSpellExtraArgs(talent).AddSpellBP0(talent->GetAmountAsInt()));
    }

    void OnUnitDeath(Unit* victim, Unit* killer) override
    {
        if (!victim)
            return;

        Player* killingHunter = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        TryGrant(killingHunter, victim);

        for (AuraEffect const* periodic : victim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
        {
            SpellInfo const* sting = periodic->GetSpellInfo();
            if (sting->SpellFamilyName != SPELLFAMILY_HUNTER || !(sting->SpellFamilyFlags[0] & HUNTER_FLAG0_SERPENT_STING))
                continue;

            Player* stingHunter = ObjectAccessor::GetPlayer(*victim, periodic->GetCasterGUID());
            if (stingHunter && stingHunter != killingHunter)
            {
                TryGrant(stingHunter, victim);
                break;
            }
        }
    }
};

void AddSC_classic_talents_forever_procs()
{
    RegisterSpellScript(classic_spell_mage_missile_barrage);
    RegisterSpellScript(classic_spell_mage_arcane_concentration);
    RegisterSpellScript(classic_spell_mage_winters_chill);
    RegisterSpellScript(classic_spell_mage_cone_of_cold_winters_chill);
    RegisterSpellScript(classic_spell_mage_master_of_elements);
    RegisterSpellScript(classic_spell_mage_magic_absorption);
    RegisterSpellScript(classic_spell_proc_trigger_amount_chance);
    RegisterSpellScript(classic_spell_sha_ancestral_healing);
    RegisterSpellScript(classic_talent_warl_nightfall);
    RegisterSpellScript(classic_talent_warl_pyroclasm);
    RegisterSpellScript(classic_talent_dru_natural_reaction);
    RegisterSpellScript(classic_spell_pri_holy_binding);
    RegisterSpellScript(classic_spell_pal_guarded_by_the_light);
    RegisterSpellScript(classic_spell_pal_guarded_by_the_light_cancel);
    RegisterSpellScript(classic_spell_pal_illumination);
    RegisterSpellScript(classic_spell_warr_deep_wounds);
    RegisterSpellScript(classic_spell_warr_deep_wound_bleed);
    RegisterSpellScript(classic_spell_warr_bloodthrill);
    new classic_unit_hun_rapid_killing();
}