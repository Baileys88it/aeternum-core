-- Aeternum arena (2026-10-28): Arena Master (9500000) spawns in the capitals + Booty Bay.
-- Apply to: world (after 2026_10_28_02_world_arena.sql). Loaded on the next restart.
-- Positions checked with navcheck + vmcheck:
-- navmesh polygon underfoot in the same component as the reference battlemaster, not sunken,
-- ceiling > 4y, LOS ok, >= 3y from other NPCs, beside the battlemaster with the same orientation.
-- Faction 35 (template): friendly and usable by both factions in every city.
DELETE FROM `creature` WHERE `guid` BETWEEN 26300001 AND 26300007;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,
  `modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,
  `MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(26300001,9500000,1,0,0,'0',0,0,0,-1,0,0,1990.41,-4789.65,56.34,3.07,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),     -- Orgrimmar, Valley of Honor (accanto a Brakgul Deathbringer)
(26300002,9500000,0,0,0,'0',0,0,0,-1,0,0,-8457.09,321.33,121.27,0.70,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),     -- Stormwind, Champions' Hall (accanto a Elfarran)
(26300003,9500000,0,0,0,'0',0,0,0,-1,0,0,-5039.92,-1275.76,510.79,2.60,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),   -- Ironforge, Military Ward (accanto a Lylandris)
(26300004,9500000,0,0,0,'0',0,0,0,-1,0,0,-14461.40,493.82,16.01,4.00,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),    -- Booty Bay, molo
(26300005,9500000,0,0,0,'0',0,0,0,-1,0,0,1265.44,355.86,-63.29,5.46,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),      -- Undercity, War Quarter (accanto a Kurden Bloodclaw)
(26300006,9500000,1,0,0,'0',0,0,0,-1,0,0,-1385.16,-79.20,160.48,3.11,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0),     -- Thunder Bluff, Hunter Rise (accanto a Kergul Bloodaxe)
(26300007,9500000,1,0,0,'0',0,0,0,-1,0,0,9975.13,2316.00,1331.02,0.70,300,0,0,0,NULL,NULL,NULL,NULL,'',NULL,0);    -- Darnassus, Warrior's Terrace (accanto a Aethalas)
