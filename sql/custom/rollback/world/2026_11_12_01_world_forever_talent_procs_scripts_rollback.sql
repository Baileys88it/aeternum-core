-- Rollback of the talent proc scripts data: back to the previous talent proc rows (generic chance script on Arcane Concentration and
-- Missile Barrage), no rows for the other talents. Use together with the previous worldserver binary.
DELETE FROM `spell_proc` WHERE `SpellId` IN (11213,400588,11180,29074,29441,16176,18094,17941,18073,417051,415059,20210,12834,1289682,1224716,440529,16850,18119,20049,20375,20915,20918,20919,20920,1310687);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(11213, 0, 3, 0, 0, 0, 0, 65536, 0, 1, 2, 0, 0, 2, 0, 100, 1000, 0),
(400588, 0, 3, 536870945, 4096, 0, 0, 65536, 0, 1, 2, 0, 0, 2, 0, 100, 0, 0);
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('classic_spell_mage_missile_barrage','classic_spell_mage_arcane_concentration',
'classic_spell_mage_winters_chill','classic_spell_mage_cone_of_cold_winters_chill','classic_spell_mage_master_of_elements',
'classic_spell_mage_magic_absorption','classic_spell_proc_trigger_amount_chance','classic_spell_sha_ancestral_healing',
'classic_talent_warl_nightfall','classic_talent_warl_pyroclasm','classic_talent_dru_natural_reaction','classic_spell_pri_holy_binding',
'classic_spell_pal_guarded_by_the_light','classic_spell_pal_guarded_by_the_light_cancel','classic_spell_pal_illumination',
'classic_spell_warr_deep_wounds','classic_spell_warr_deep_wound_bleed','classic_spell_warr_bloodthrill');
DELETE FROM `spell_script_names` WHERE `spell_id` IN (11213, 400588) AND `ScriptName` = 'classic_spell_weapon_extra_attack_chance';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (11213, 'classic_spell_weapon_extra_attack_chance'), (400588, 'classic_spell_weapon_extra_attack_chance');
DELETE FROM `spell_threat` WHERE `entry` = 1328570;