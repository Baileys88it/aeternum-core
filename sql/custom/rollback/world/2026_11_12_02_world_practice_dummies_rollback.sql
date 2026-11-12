-- Restore Practice Dummy template and remove the new capital spawns.
-- Existing Undercity guids 20031899, 20038108, 20041842 are left in place.

DELETE FROM `creature` WHERE `guid` BETWEEN 26330001 AND 26330015;

UPDATE `creature_template`
SET `faction` = 914,
    `unit_flags` = 256,
    `flags_extra` = 66,
    `ScriptName` = '',
    `AIName` = 'SmartAI'
WHERE `entry` = 5652;
