-- Forever (1.60) player report fixes, 2026-11-10. Data from Efficient Games caches (build 1.60.1.70790, CC BY 4.0) and the
-- official 70124 sniff templates already in the database. Z of new spawns = terrain height from the server map files.

-- Forgotten Loa Idols (97225): the quest object 'Forgotten Loa Idol' (660647, loot 142139 -> item 277135) was missing entirely.
-- Spawns sit on open ground: terrain height + 0.1, moved (max 7.5 yd) off spots under a structure (navmesh layer above),
-- off the navmesh, at a wall edge or in water.
DELETE FROM `gameobject_template` WHERE `entry`=660647;
INSERT INTO `gameobject_template` (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,`Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,`Data8`,`Data9`,`Data10`,`Data11`,`Data12`,`Data13`,`Data14`,`Data15`,`Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,`Data24`,`Data25`,`Data26`,`Data27`,`Data28`,`Data29`,`Data30`,`Data31`,`Data32`,`Data33`,`Data34`,`ContentTuningId`,`RequiredLevel`,`AIName`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(660647,3,127185,'Forgotten Loa Idol','','','',0.3,43,142139,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4097,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'','',NULL,70790);
DELETE FROM `gameobject_questitem` WHERE `GameObjectEntry`=660647;
INSERT INTO `gameobject_questitem` (`GameObjectEntry`,`Idx`,`ItemId`,`VerifiedBuild`) VALUES (660647,0,277135,70790);
DELETE FROM `gameobject_loot_template` WHERE `Entry`=142139;
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES (142139,0,277135,100,1,1,0,1,1,'Forgotten Loa Idol (660647) - quest 97225');
DELETE FROM `gameobject` WHERE `id`=660647 OR `guid` BETWEEN 34020001 AND 34020050;
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(34020001,660647,1,0,0,'0',0,0,0,-1,-1277.63,-5502.66,5.174,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020002,660647,1,0,0,'0',0,0,0,-1,-1272.70,-5504.00,5.216,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020003,660647,1,0,0,'0',0,0,0,-1,-1253.30,-5514.70,5.918,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020004,660647,1,0,0,'0',0,0,0,-1,-1249.67,-5518.71,6.058,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020005,660647,1,0,0,'0',0,0,0,-1,-1229.20,-5424.00,5.293,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020006,660647,1,0,0,'0',0,0,0,-1,-1221.10,-5428.70,5.880,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020007,660647,1,0,0,'0',0,0,0,-1,-1214.25,-5401.10,6.592,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020008,660647,1,0,0,'0',0,0,0,-1,-1210.95,-5408.90,5.965,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020009,660647,1,0,0,'0',0,0,0,-1,-1211.70,-5434.50,5.588,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020010,660647,1,0,0,'0',0,0,0,-1,-1207.60,-5570.80,8.552,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020011,660647,1,0,0,'0',0,0,0,-1,-1202.95,-5397.55,6.925,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020012,660647,1,0,0,'0',0,0,0,-1,-1201.80,-5437.70,5.596,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020013,660647,1,0,0,'0',0,0,0,-1,-1200.20,-5565.20,8.413,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020014,660647,1,0,0,'0',0,0,0,-1,-1195.95,-5599.90,7.207,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020015,660647,1,0,0,'0',0,0,0,-1,-1181.82,-5616.18,5.035,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020016,660647,1,0,0,'0',0,0,0,-1,-1182.50,-5416.30,5.072,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020017,660647,1,0,0,'0',0,0,0,-1,-1178.70,-5425.80,4.625,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020018,660647,1,0,0,'0',0,0,0,-1,-1141.83,-5500.03,5.940,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020019,660647,1,0,0,'0',0,0,0,-1,-1134.29,-5511.76,6.202,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020020,660647,1,0,0,'0',0,0,0,-1,-1111.38,-5522.89,8.824,0,0,0,0,1,120,255,1,'',NULL,70790),
(34020021,660647,1,0,0,'0',0,0,0,-1,-1084.83,-5548.47,10.472,0,0,0,0,1,120,255,1,'',NULL,70790);

-- Bloodtalon Matriarch (97223): creature 268530 (level 9 raptor, drops Eggs of the Bloodtalon Matriarch 277128) was missing.
-- Template copied from Bloodtalon Taillasher (3122) with the 1.60 values of 268530 (display 11316, CreatureDifficulty 352159).
DELETE FROM `creature_template` WHERE `entry`=268530;
INSERT INTO `creature_template` SELECT 268530,0,0,'Bloodtalon Matriarch',`femaleName`,`subname`,`TitleAlt`,`IconName`,`RequiredExpansion`,`VignetteID`,`faction`,`npcflag`,`speed_walk`,`speed_run`,`scale`,`Classification`,`dmgschool`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`unit_flags3`,`family`,`trainer_class`,`type`,`VehicleId`,'',`MovementType`,`ExperienceModifier`,`RacialLeader`,`movementId`,`WidgetSetID`,`WidgetSetUnitConditionID`,`RegenHealth`,`CreatureImmunitiesId`,`flags_extra`,'',NULL,70790 FROM `creature_template` WHERE `entry`=3122;
DELETE FROM `creature_template_difficulty` WHERE `Entry`=268530;
INSERT INTO `creature_template_difficulty` (`Entry`,`DifficultyID`,`LevelScalingDeltaMin`,`LevelScalingDeltaMax`,`ContentTuningID`,`HealthScalingExpansion`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`CreatureDifficultyID`,`TypeFlags`,`TypeFlags2`,`TypeFlags3`,`LootID`,`PickPocketLootID`,`SkinLootID`,`GoldMin`,`GoldMax`,`StaticFlags1`,`StaticFlags2`,`StaticFlags3`,`StaticFlags4`,`StaticFlags5`,`StaticFlags6`,`StaticFlags7`,`StaticFlags8`,`VerifiedBuild`)
SELECT 268530,0,0,0,`ContentTuningID`,`HealthScalingExpansion`,1,1,`ArmorModifier`,`DamageModifier`,352159,134217729,0,0,268530,0,`SkinLootID`,`GoldMin`,`GoldMax`,0,0,0,0,0,0,0,0,70790 FROM `creature_template_difficulty` WHERE `Entry`=3122 AND `DifficultyID`=0;
DELETE FROM `creature_template_model` WHERE `CreatureID`=268530;
INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`) VALUES (268530,0,11316,1,1,70790);
DELETE FROM `creature_loot_template` WHERE `Entry`=268530;
INSERT INTO `creature_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES (268530,0,277128,100,1,1,0,1,1,'Bloodtalon Matriarch - Eggs of the Bloodtalon Matriarch (quest 97223)');
DELETE FROM `creature_questitem` WHERE `CreatureEntry`=268530;
INSERT INTO `creature_questitem` (`CreatureEntry`,`DifficultyID`,`Idx`,`ItemId`,`VerifiedBuild`) VALUES (268530,0,0,277128,70790);
DELETE FROM `creature` WHERE `id`=268530 OR `guid`=26320001;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (26320001,268530,1,0,0,'0',0,0,0,-1,0,0,-727.80,-5600.50,25.040,0,300,10,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790);

-- Legging It (96821): Ridgeshade Creeper (266850) / Lurker (266849) had faction 35 (friendly); 1.60 shows them hostile, level 5-6.
-- Ridgeshade Creeper had no spawns at all: spawned at its observed 1.60 positions.
UPDATE `creature_template` SET `faction`=14 WHERE `entry` IN (266849,266850);
DELETE FROM `creature` WHERE `guid` BETWEEN 26320010 AND 26320059;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(26320010,266850,1,0,0,'0',0,0,0,-1,0,0,-440.90,-4698.60,37.753,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320011,266850,1,0,0,'0',0,0,0,-1,0,0,-431.50,-4703.80,38.284,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320012,266850,1,0,0,'0',0,0,0,-1,0,0,-363.40,-4741.40,38.781,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320013,266850,1,0,0,'0',0,0,0,-1,0,0,-357.10,-4722.90,38.438,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320014,266850,1,0,0,'0',0,0,0,-1,0,0,-342.20,-4733.60,37.143,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320015,266850,1,0,0,'0',0,0,0,-1,0,0,-341.20,-4769.90,36.533,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320016,266850,1,0,0,'0',0,0,0,-1,0,0,-315.20,-4729.90,35.022,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320017,266850,1,0,0,'0',0,0,0,-1,0,0,-313.10,-4738.10,35.196,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320018,266850,1,0,0,'0',0,0,0,-1,0,0,-291.00,-4683.00,41.894,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320019,266850,1,0,0,'0',0,0,0,-1,0,0,-220.10,-4682.20,38.054,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320020,266850,1,0,0,'0',0,0,0,-1,0,0,-213.80,-4698.40,30.840,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320021,266850,1,0,0,'0',0,0,0,-1,0,0,-209.10,-4712.60,29.010,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320022,266850,1,0,0,'0',0,0,0,-1,0,0,-207.30,-4700.60,30.016,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320023,266850,1,0,0,'0',0,0,0,-1,0,0,-187.70,-4582.70,56.287,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320024,266850,1,0,0,'0',0,0,0,-1,0,0,-181.10,-4680.70,32.574,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320025,266850,1,0,0,'0',0,0,0,-1,0,0,-112.70,-4646.50,34.863,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320026,266850,1,0,0,'0',0,0,0,-1,0,0,-86.60,-4649.00,36.042,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320027,266850,1,0,0,'0',0,0,0,-1,0,0,24.40,-4701.60,32.895,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320028,266850,1,0,0,'0',0,0,0,-1,0,0,37.00,-4689.30,34.301,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790),
(26320029,266850,1,0,0,'0',0,0,0,-1,0,0,41.60,-4641.00,45.936,0,300,5,0,NULL,1,NULL,NULL,NULL,NULL,'',NULL,70790);

-- Fixed 1.60 levels (creature_classic_level overrides ContentTuning):
--  Ridgeshade 5-6, Bloodtalon Matriarch 9, Whispering Horror 13 (Shadowvale elite: had no level row, so it scaled from the zone tuning
--  and showed as a skull). Imp Minion (12922) / Voidwalker Minion (8996) were fixed at 46/47, so summons of level 10-12 warlocks
--  (Dark Storms, Burning Blade) came out at 46-47; 1-60 lets a summon take its summoner's level (1.60: 9-12 in Durotar).
DELETE FROM `creature_classic_level` WHERE `entry` IN (266849,266850,268530,260396,12922,8996);
INSERT INTO `creature_classic_level` (`entry`,`level_min`,`level_max`) VALUES (266849,5,6),(266850,5,6),(268530,9,9),(260396,13,13),(12922,1,60),(8996,1,60);

-- Patience (99141): the report options cast spells 1319468/1319489/1319508 that give no item on this core.
-- The option now hands out the report through SmartAI (same items, same conditions on the option); Gordo also gets the gossip flag.
UPDATE `gossip_menu_option` SET `SpellID`=NULL WHERE (`MenuID`,`GossipOptionID`) IN ((44990,142723),(44991,142730),(44993,142737));
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry` IN (1496,251001) AND `ScriptName`='';
UPDATE `creature_template` SET `npcflag`=`npcflag`|1 WHERE `entry`=10666;
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND ((`entryorguid` IN (1496,251001) AND `id`=0) OR (`entryorguid`=10666 AND `id`=100));
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(1496,0,0,0,'',62,0,100,0,44990,6,0,0,0,'',56,286200,1,0,0,0,0,0,NULL,7,0,0,0,0,NULL,0,0,0,0,'Deathguard Dillinger - On Gossip Option 6 Selected - Add Item ''Dillinger''''s Report'' (Patience)'),
(251001,0,0,0,'',62,0,100,0,44991,6,0,0,0,'',56,286201,1,0,0,0,0,0,NULL,7,0,0,0,0,NULL,0,0,0,0,'Deathguard Kristof - On Gossip Option 6 Selected - Add Item ''Kristof''''s Report'' (Patience)'),
(10666,0,100,0,'',62,0,100,0,44993,0,0,0,0,'',56,286202,1,0,0,0,0,0,NULL,7,0,0,0,0,NULL,0,0,0,0,'Gordo - On Gossip Option 0 Selected - Add Item ''Gordo''''s Report'' (Patience)');

-- Seeking Refuge (99144): Bareth Dawnstone (275954) had 5 spawns (escort positions of the sniff), no escort script and no path.
-- Keep the quest giver spawn at the top of the tower, path through his sniffed positions, quest completes at the end (quest has
-- the completion-event flag).
DELETE FROM `creature` WHERE `id`=275954 AND `guid` IN (21200482,21200484,21200485,21200486);
DELETE FROM `waypoint_path` WHERE `PathId`=27595400; DELETE FROM `waypoint_path_node` WHERE `PathId`=27595400;
INSERT INTO `waypoint_path` (`PathId`,`MoveType`,`Flags`,`Velocity`,`Comment`) VALUES (27595400,0,0,NULL,'Bareth Dawnstone - Seeking Refuge escort');
INSERT INTO `waypoint_path_node` (`PathId`,`NodeId`,`PositionX`,`PositionY`,`PositionZ`,`Orientation`,`Delay`) VALUES
(27595400,1,2455.23,1598.33,40.317,NULL,0),
(27595400,2,2406.1,1569.38,31.695,NULL,0),
(27595400,3,2402.67,1552.81,31.526,NULL,0),
(27595400,4,2401.92,1529.49,31.872,NULL,0);
DELETE FROM `smart_scripts` WHERE `entryorguid`=275954 AND `source_type`=0 AND `id` BETWEEN 3 AND 9;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(275954,0,3,4,'',19,0,100,0,99144,0,0,0,0,'',1,0,0,1,0,0,0,0,NULL,7,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Quest ''Seeking Refuge'' Accepted - Say Line 0'),
(275954,0,4,0,'',61,0,100,0,0,0,0,0,0,'',53,0,27595400,0,99144,0,0,0,NULL,7,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Quest ''Seeking Refuge'' Accepted - Start Escort Path'),
(275954,0,5,0,'',40,0,100,0,1,27595400,0,0,0,'',1,1,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Waypoint 1 Reached - Say Line 1'),
(275954,0,6,0,'',40,0,100,0,2,27595400,0,0,0,'',1,2,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Waypoint 2 Reached - Say Line 2'),
(275954,0,7,0,'',40,0,100,0,3,27595400,0,0,0,'',1,3,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Waypoint 3 Reached - Say Line 3'),
(275954,0,8,9,'',58,0,100,0,4,27595400,0,0,0,'',1,4,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Path Ended - Say Line 4'),
(275954,0,9,0,'',61,0,100,0,0,0,0,0,0,'',41,5000,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Bareth Dawnstone - On Path Ended - Despawn (5 s)');

-- A Lesson in Divinity (94438): Deathguard Falgan (257663) lies dead from his template addon as well (not only from the SmartAI
-- reset event), so he never shows standing before the Symbol of Life (8593) hits him.
DELETE FROM `creature_template_addon` WHERE `entry`=257663;
INSERT INTO `creature_template_addon` (`entry`,`PathId`,`mount`,`MountCreatureID`,`StandState`,`AnimTier`,`VisFlags`,`SheathState`,`PvPFlags`,`emote`,`aiAnimKit`,`movementAnimKit`,`meleeAnimKit`,`visibilityDistanceType`,`auras`) VALUES (257663,0,0,0,7,0,0,1,0,0,0,0,0,0,'');
