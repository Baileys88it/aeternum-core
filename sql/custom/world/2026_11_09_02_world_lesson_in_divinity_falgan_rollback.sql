-- Rollback of 2026_11_09_02_world_lesson_in_divinity_falgan.sql (values as found on 2026-11-09).
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceGroup`=1 AND `SourceEntry`=8593 AND `ConditionTypeOrReference`=31 AND `ConditionValue2`=257663;
UPDATE `creature` SET `npcflag`=0, `unit_flags`=0, `unit_flags2`=0, `unit_flags3`=0 WHERE `guid` IN (26300010,26300011,26300012,26300013,26300014,26300015,26300016,26300017,26300018,26300019,26300020,26300021,26300022,26300023,26300024,26300025,26300026,26300027,26300028,26300029);
DELETE FROM `smart_scripts` WHERE `entryorguid`=257663 AND `source_type`=0;
UPDATE `creature_template` SET `AIName`='', `ScriptName`='' WHERE `entry`=257663;
