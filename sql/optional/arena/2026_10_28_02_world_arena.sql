-- Aeternum arena (2026-10-28): Mak'gora Arena (BattlemasterList 1169, map 2995) for custom 1v1/2v2/3v3.
-- Apply to: world. Requires 2026_10_28_01_hotfixes_arena_map.sql (InstanceType 4) for battleground_scripts.

-- Start positions here are provisional; 2026_10_28_05_world_arena_start_locs.sql replaces them.
DELETE FROM `world_safe_locs` WHERE `ID` IN (1105000,1105001);
INSERT INTO `world_safe_locs` (`ID`,`MapID`,`LocX`,`LocY`,`LocZ`,`Facing`,`TransportSpawnId`,`Comment`) VALUES
(1105000,2995,5411.7,-2791.7,1455.3,0,NULL,'Aeternum arena - Mak''gora Arena - start gold (PROVISIONAL, .gps)'),
(1105001,2995,5471.7,-2791.7,1455.3,3.14159,NULL,'Aeternum arena - Mak''gora Arena - start green (PROVISIONAL, .gps)');

-- StartMaxDist 5: during preparation the core returns anyone who moves more than 5 yards to the start point.
DELETE FROM `battleground_template` WHERE `ID` = 1169;
INSERT INTO `battleground_template` (`ID`,`AllianceStartLoc`,`HordeStartLoc`,`StartMaxDist`,`Weight`,`ScriptName`,`Comment`) VALUES
(1169,1105000,1105001,5,1,'','Aeternum - Mak''gora Arena');

-- Map script (FFA hostility between the two teams, leash, rating). BattlemasterListId 0 = valid for any BG.
DELETE FROM `battleground_scripts` WHERE `MapId` = 2995;
INSERT INTO `battleground_scripts` (`MapId`,`BattlemasterListId`,`ScriptName`) VALUES
(2995,0,'arena_wf_makgora');

-- Core arena season (Arena.ArenaSeason.ID = 32 in the runtime conf): without an event the log shows
-- "ArenaSeason (32) must be an existing Arena Season". Always-active event, no content.
DELETE FROM `game_event_arena_seasons` WHERE `eventEntry` = 200;
DELETE FROM `game_event` WHERE `eventEntry` = 200;
INSERT INTO `game_event` (`eventEntry`,`start_time`,`end_time`,`occurence`,`length`,`holiday`,`holidayStage`,`WorldStateId`,`description`,`world_event`,`announce`) VALUES
(200,NULL,NULL,5184000,2592000,0,0,NULL,'Aeternum - Arena Season 32',0,2);
INSERT INTO `game_event_arena_seasons` (`eventEntry`,`season`) VALUES
(200,32);

-- NPC "Arena Master" 9500000: copy of Brakgul Deathbringer (3890, model 15032) so it does not depend on the columns of
-- creature_template; name, faction (35, friendly to all), npcflag (gossip only) and ScriptName change.
DELETE FROM `creature_template_model` WHERE `CreatureID` = 9500000;
DELETE FROM `creature_template_difficulty` WHERE `Entry` = 9500000;
DELETE FROM `creature_template` WHERE `entry` = 9500000;

DROP TEMPORARY TABLE IF EXISTS `tmp_wf_arena_npc`;
CREATE TEMPORARY TABLE `tmp_wf_arena_npc` SELECT * FROM `creature_template` WHERE `entry` = 3890;
UPDATE `tmp_wf_arena_npc` SET `entry` = 9500000, `name` = 'Arena Master', `subname` = 'Mak''gora Arena', `faction` = 35, `npcflag` = 1, `ScriptName` = 'npc_wf_arena_master';
INSERT INTO `creature_template` SELECT * FROM `tmp_wf_arena_npc`;
DROP TEMPORARY TABLE `tmp_wf_arena_npc`;

DROP TEMPORARY TABLE IF EXISTS `tmp_wf_arena_npc`;
CREATE TEMPORARY TABLE `tmp_wf_arena_npc` SELECT * FROM `creature_template_model` WHERE `CreatureID` = 3890;
UPDATE `tmp_wf_arena_npc` SET `CreatureID` = 9500000;
INSERT INTO `creature_template_model` SELECT * FROM `tmp_wf_arena_npc`;
DROP TEMPORARY TABLE `tmp_wf_arena_npc`;

DROP TEMPORARY TABLE IF EXISTS `tmp_wf_arena_npc`;
CREATE TEMPORARY TABLE `tmp_wf_arena_npc` SELECT * FROM `creature_template_difficulty` WHERE `Entry` = 3890;
UPDATE `tmp_wf_arena_npc` SET `Entry` = 9500000;
INSERT INTO `creature_template_difficulty` SELECT * FROM `tmp_wf_arena_npc`;
DROP TEMPORARY TABLE `tmp_wf_arena_npc`;
