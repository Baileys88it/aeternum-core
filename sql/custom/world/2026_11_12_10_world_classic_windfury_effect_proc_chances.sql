-- Classic 1.60: Windfury Weapon (439431) casts the rank spell of the enchant on the swinging weapon, and talents whose proc chance
-- is an effect value scaled by the talent rank (SpellAuraOptions says 100%, so they fired on every hit) roll that chance.
-- Talents already covered by other scripts keep them. C++: Custom/Classic/world/classic_spell_scripts.cpp. Safe to run again.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('classic_spell_sha_windfury_weapon', 'classic_spell_proc_chance_from_effect');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(439431, 'classic_spell_sha_windfury_weapon'),
(11103, 'classic_spell_proc_chance_from_effect'),   -- Impact
(15270, 'classic_spell_proc_chance_from_effect'),   -- Spirit Tap
(14531, 'classic_spell_proc_chance_from_effect'),   -- Martyrdom
(14909, 'classic_spell_proc_chance_from_effect'),   -- Searing Light
(12298, 'classic_spell_proc_chance_from_effect'),   -- Shield Specialization (warrior)
(20150, 'classic_spell_proc_chance_from_effect');   -- Shield Specialization (paladin)