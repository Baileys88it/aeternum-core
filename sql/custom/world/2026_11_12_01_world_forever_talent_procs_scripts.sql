-- Classic 1.60.1.70888 talent procs that need the classic_talents_forever_procs.cpp scripts (and a few data-only proc rows).
-- Requires a worldserver built with AddSC_classic_talents_forever_procs. Rank chances are the aura amounts (trait rank curves); the
-- scripts roll them, so the rows use Chance 100. Columns: school, family, family masks 0-3, proc flags, flags 2, spell type,
-- phase, hit mask, attributes, disabled effects, ppm, chance, cooldown, charges. Safe to run again.

SET @IDS := '11213,400588,11180,29074,29441,16176,18094,17941,18073,417051,415059,20210,12834,1289682,1224716,440529,16850,18119,20049,20375,20915,20918,20919,20920,1310687';
DELETE FROM `spell_proc` WHERE FIND_IN_SET(`SpellId`, @IDS);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
-- Arcane Concentration: mage damage spell hits, plus the Arcane Missiles channel (no damage) and its triggered missiles; the script
-- splits the rank chance across the channel and every missile and skips other triggered spells. Client cooldown 1 s.
(11213,  0, 3, 0, 0, 0, 0, 0x10000, 0, 5, 2, 0, 2, 0x2, 0, 100, 1000, 0),
-- Missile Barrage: Arcane Blast, Arcane Wrath, Fireball, Frostbolt, Frostfire Bolt damage hits; the script rolls full / half chance
(400588, 0, 3, 0x20000021, 0x9000, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0x2, 0, 100, 0, 0),
-- Winter's Chill: Frostbolt damage hits; the dummies (effects 1, 2) do not proc on their own
(11180,  0, 3, 0x20, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0x6, 0, 100, 0, 0),
-- Master of Elements: Fire and Frost mage spell critical damage hits
(29074, 20, 3, 0, 0, 0, 0, 0x10000, 0, 1, 2, 0x2, 0, 0, 0, 100, 0, 0),
-- Magic Absorption: harmful spells taken that were fully resisted (client cooldown 1 s)
(29441,  0, 0, 0, 0, 0, 0, 0x20000, 0, 7, 0, 0x8, 0, 0, 0, 100, 0, 0),
-- Ancestral Healing: critical heals (triggered heals too)
(16176,  0, 0, 0, 0, 0, 0, 0x4400, 0, 2, 2, 0x2, 0x2, 0, 0, 100, 0, 0),
-- Nightfall: periodic damage of Corruption, Drain Life, Drain Soul, Wrack
(18094, 32, 5, 0x400A, 0x40000, 0, 0, 0x40000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
-- Shadow Trance: used by the next Shadow Bolt it makes instant (one charge, only a spell the mod changes)
(17941,  0, 5, 0x1, 0, 0, 0, 0x10000, 0, 7, 1, 0, 0x8, 0, 0, 100, 0, 1),
-- Pyroclasm: Soul Fire, Hellfire, Rain of Fire damage hits and ticks; effect 1 (dummy trigger 18350) disabled
(18073,  4, 5, 0x60, 0, 0, 0, 0x50000, 0, 1, 2, 0, 0, 0x2, 0, 100, 0, 0),
-- Natural Reaction: melee swings and abilities taken that were dodged; the script rolls effect 1
(417051, 0, 0, 0, 0, 0, 0, 0x28, 0, 7, 0, 0x10, 0, 0xC, 0, 100, 0, 0),
-- Guarded by the Light: melee swings and abilities taken that were blocked
(415059, 0, 0, 0, 0, 0, 0, 0x28, 0, 7, 0, 0x40, 0, 0, 0, 100, 0, 0),
-- Illumination: critical heals of Flash of Light, Holy Light, Holy Shock, Light's Vigil; dummies (effects 1-3) disabled
(20210,  0, 10, 0x206000, 0, 0, 0x4000000, 0x4400, 0, 2, 2, 0x2, 0x2, 0xE, 0, 100, 0, 0),
-- Deep Wounds: melee swing and melee ability critical hits
(12834,  0, 0, 0, 0, 0, 0, 0x14, 0, 1, 2, 0x2, 0, 0, 0, 100, 0, 0),
-- Bloodthrill: melee swings and abilities that land; the script needs the warrior's Rend on the target and a main hand attack
(1289682, 0, 0, 0, 0, 0, 0, 0x14, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
-- Puncturing Wounds: Backstab hits (extra combo point, effect 1 amount)
(1224716, 0, 8, 0x4, 0, 0, 0, 0x10, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
-- Resourcefulness: melee and ranged ability critical strikes (effect 1 amount)
(440529, 0, 0, 0, 0, 0, 0, 0x110, 0, 1, 2, 0x2, 0, 0, 0, 100, 0, 0),
-- Improved Starfire: Starfire damage hits (stun, effect 1 amount)
(16850,  0, 7, 0x4, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
-- Aftermath: Conflagrate damage hits (daze, effect 0 amount)
(18119,  0, 5, 0x200, 0, 0, 0, 0x10000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
-- Vengeance: non-periodic critical strikes only (client proc flags and chance)
(20049,  0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0x2, 0, 0, 0, 0, 0, 0),
-- Seal of Command ranks: 7 procs per minute (vanilla rate; the client chance is 100 with a 1 s cooldown)
(20375,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0),
(20915,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0),
(20918,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0),
(20919,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0),
(20920,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0),
-- Sniper Shot: the range bonus is used by the next 3 shots it changed (client charges)
(1310687, 0, 9, 399873, 0, 0, 0, 0x140, 0, 7, 1, 0, 0x8, 0, 0, 100, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (11213, 400588) AND `ScriptName` = 'classic_spell_weapon_extra_attack_chance';
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('classic_spell_mage_missile_barrage','classic_spell_mage_arcane_concentration',
'classic_spell_mage_winters_chill','classic_spell_mage_cone_of_cold_winters_chill','classic_spell_mage_master_of_elements',
'classic_spell_mage_magic_absorption','classic_spell_proc_trigger_amount_chance','classic_spell_sha_ancestral_healing',
'classic_talent_warl_nightfall','classic_talent_warl_pyroclasm','classic_talent_dru_natural_reaction','classic_spell_pri_holy_binding',
'classic_spell_pal_guarded_by_the_light','classic_spell_pal_guarded_by_the_light_cancel','classic_spell_pal_illumination',
'classic_spell_warr_deep_wounds','classic_spell_warr_deep_wound_bleed','classic_spell_warr_bloodthrill');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(400588, 'classic_spell_mage_missile_barrage'),
(11213, 'classic_spell_mage_arcane_concentration'),
(11180, 'classic_spell_mage_winters_chill'),
(120, 'classic_spell_mage_cone_of_cold_winters_chill'), (8492, 'classic_spell_mage_cone_of_cold_winters_chill'),
(10159, 'classic_spell_mage_cone_of_cold_winters_chill'), (10160, 'classic_spell_mage_cone_of_cold_winters_chill'),
(10161, 'classic_spell_mage_cone_of_cold_winters_chill'),
(29074, 'classic_spell_mage_master_of_elements'),
(29441, 'classic_spell_mage_magic_absorption'),
(1224716, 'classic_spell_proc_trigger_amount_chance'), (440529, 'classic_spell_proc_trigger_amount_chance'),
(16850, 'classic_spell_proc_trigger_amount_chance'), (18119, 'classic_spell_proc_trigger_amount_chance'),
(16176, 'classic_spell_sha_ancestral_healing'),
(18094, 'classic_talent_warl_nightfall'),
(18073, 'classic_talent_warl_pyroclasm'),
(417051, 'classic_talent_dru_natural_reaction'),
(2061, 'classic_spell_pri_holy_binding'), (9472, 'classic_spell_pri_holy_binding'), (9473, 'classic_spell_pri_holy_binding'),
(9474, 'classic_spell_pri_holy_binding'), (10915, 'classic_spell_pri_holy_binding'), (10916, 'classic_spell_pri_holy_binding'),
(10917, 'classic_spell_pri_holy_binding'),
(415059, 'classic_spell_pal_guarded_by_the_light'),
(19750, 'classic_spell_pal_guarded_by_the_light_cancel'), (19939, 'classic_spell_pal_guarded_by_the_light_cancel'),
(19940, 'classic_spell_pal_guarded_by_the_light_cancel'), (19941, 'classic_spell_pal_guarded_by_the_light_cancel'),
(19942, 'classic_spell_pal_guarded_by_the_light_cancel'), (19943, 'classic_spell_pal_guarded_by_the_light_cancel'),
(635, 'classic_spell_pal_guarded_by_the_light_cancel'), (639, 'classic_spell_pal_guarded_by_the_light_cancel'),
(647, 'classic_spell_pal_guarded_by_the_light_cancel'), (1026, 'classic_spell_pal_guarded_by_the_light_cancel'),
(1042, 'classic_spell_pal_guarded_by_the_light_cancel'), (3472, 'classic_spell_pal_guarded_by_the_light_cancel'),
(10328, 'classic_spell_pal_guarded_by_the_light_cancel'), (10329, 'classic_spell_pal_guarded_by_the_light_cancel'),
(25292, 'classic_spell_pal_guarded_by_the_light_cancel'), (20473, 'classic_spell_pal_guarded_by_the_light_cancel'),
(20929, 'classic_spell_pal_guarded_by_the_light_cancel'), (20930, 'classic_spell_pal_guarded_by_the_light_cancel'),
(1311606, 'classic_spell_pal_guarded_by_the_light_cancel'),
(20210, 'classic_spell_pal_illumination'),
(12834, 'classic_spell_warr_deep_wounds'),
(412609, 'classic_spell_warr_deep_wound_bleed'),
(1289682, 'classic_spell_warr_bloodthrill');

-- Holy Binding self heal: "This healing generates no threat"
DELETE FROM `spell_threat` WHERE `entry` = 1328570;
INSERT INTO `spell_threat` (`entry`, `flatMod`, `pctMod`, `apPctMod`) VALUES (1328570, 0, 0, 0);