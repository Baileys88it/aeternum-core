-- Rollback for 2026_11_12_00_world_forever_talent_procs.sql
-- Removes the rows that file added (none of their keys existed before). Requires a world server restart.
DELETE FROM `spell_proc` WHERE `SpellId` IN (11213,12536,400588,400589,11071,12289,12311,13754,13976,14156,19228,19407,15257,15326,16164,16246,16880,17079,17082,12043,16188,17116,18708,14177,20216,14751,18288);
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'classic_spell_weapon_extra_attack_chance' AND `spell_id` IN (11213,400588,11071,12289,12311,13754,13976,14156,19228,19407,15257,15326,16164);