-- Practice Dummy (entry 5652, display 3019): vanilla straw dummy already used in Undercity.
-- Bind the existing npc_training_dummy AI (damage absorbed, combat ends 5s after last hit).
-- Faction 7 is attackable by both factions. Clear IMMUNE_TO_PC. Keep no-XP / no skill gains / civilian.
-- Existing Undercity War Quarter spawns (guid 20031899, 20038108, 20041842) are kept and inherit the template.
-- New spawns use free guids 26330001-26330015 (3 each in Stormwind, Ironforge, Darnassus, Orgrimmar, Thunder Bluff).

UPDATE `creature_template`
SET `faction` = 7,
    `unit_flags` = 0,
    `flags_extra` = 262210,
    `ScriptName` = 'npc_training_dummy',
    `AIName` = ''
WHERE `entry` = 5652;

DELETE FROM `creature` WHERE `guid` BETWEEN 26330001 AND 26330015;
INSERT INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,
   `modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,
   `currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`)
VALUES
-- Stormwind, Old Town courtyard below the warrior trainers (Command Center)
(26330001,5652,0,0,0,'0',0,0,0,-1,0,0,-8698.5,311.5,103.19,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330002,5652,0,0,0,'0',0,0,0,-1,0,0,-8692.5,311.0,103.19,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330003,5652,0,0,0,'0',0,0,0,-1,0,0,-8686.5,310.5,103.19,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
-- Ironforge, Hall of Arms floor near the warrior trainers
(26330004,5652,0,0,0,'0',0,0,0,-1,0,0,-5054.0,-1246.0,507.837,0.20,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330005,5652,0,0,0,'0',0,0,0,-1,0,0,-5054.0,-1239.0,507.837,0.20,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330006,5652,0,0,0,'0',0,0,0,-1,0,0,-5048.5,-1232.5,507.835,5.80,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
-- Darnassus, Warrior's Terrace south of the warrior trainers
(26330007,5652,1,0,0,'0',0,0,0,-1,0,0,9974.0,2302.0,1330.87,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330008,5652,1,0,0,'0',0,0,0,-1,0,0,9982.0,2300.0,1330.87,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330009,5652,1,0,0,'0',0,0,0,-1,0,0,9990.0,2301.0,1330.87,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
-- Orgrimmar, Valley of Honor south of the warrior trainers
(26330010,5652,1,0,0,'0',0,0,0,-1,0,0,1972.0,-4822.0,56.848,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330011,5652,1,0,0,'0',0,0,0,-1,0,0,1980.0,-4822.0,56.848,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330012,5652,1,0,0,'0',0,0,0,-1,0,0,1988.0,-4822.0,56.848,1.57,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
-- Thunder Bluff, Hunter Rise east of the warrior / hunter trainers
(26330013,5652,1,0,0,'0',0,0,0,-1,0,0,-1438.0,-90.0,159.101,3.14,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330014,5652,1,0,0,'0',0,0,0,-1,0,0,-1438.0,-82.0,159.101,3.14,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0),
(26330015,5652,1,0,0,'0',0,0,0,-1,0,0,-1442.0,-74.0,159.101,3.64,300,0,0,NULL,0,NULL,NULL,NULL,NULL,'',NULL,0);
