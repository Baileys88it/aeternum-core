-- Forsaken Paladin: "A Lesson in Divinity" (94438) - Deathguard Falgan (257663) can be resurrected
-- with the Symbol of Life (item 6866, spell 8593) and then accepts the turn-in / offers 94440.
--
-- Root cause (world, 2026-11-09):
--  1) Spell 8593 has one DUMMY effect with ImplicitTarget 38 (TARGET_UNIT_NEARBY_ENTRY, wago.tools SpellEffect 687546).
--     Its targets come only from conditions (SourceType 13), which listed 17542/6172/6177 but not Falgan,
--     so the Symbol of Life never found him.
--  2) sql/custom spawned 20 quest NPCs (guid 26300010-26300029,
--     Falgan = 26300028) with explicit npcflag/unit_flags/unit_flags2/unit_flags3 = 0 instead of NULL.
--     TrinityCore treats a non-NULL creature.npcflag as an override, so all 20 lost the template questgiver flag
--     (template npcflag 2/3) and the template unit_flags (768 = immune to PC/NPC).
--  3) Falgan had no AI: he stood alive and never reacted to the spell.
--
-- Fix:
--  1) Condition: spell 8593 can target 257663 (same pattern as Henze Faulk 6172).
--  2) Spawn overrides back to NULL for the 20 spawns (template values, VerifiedBuild 70124). Needs a restart
--     (creature spawn data is loaded only at startup).
--  3) SmartAI for Falgan, modelled on classic_npc_henze_faulk (VMaNGOS port): lies dead without npcflag
--     (the 70124 sniff template has npcflag 0), stands up and becomes questgiver when hit by 8593,
--     lies down again after 120 s (Henze's timer; Wowhead comments confirm he "dies again" after a while).

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceGroup`=1 AND `SourceEntry`=8593 AND `ConditionTypeOrReference`=31 AND `ConditionValue2`=257663;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`ConditionStringValue1`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(13,1,8593,0,3,31,0,3,257663,0,'',0,0,0,'','Symbol of Life can be casted on Deathguard Falgan');

UPDATE `creature` SET `npcflag`=NULL, `unit_flags`=NULL, `unit_flags2`=NULL, `unit_flags3`=NULL
 WHERE `guid` IN (26300010,26300011,26300012,26300013,26300014,26300015,26300016,26300017,26300018,26300019,26300020,26300021,26300022,26300023,26300024,26300025,26300026,26300027,26300028,26300029)
   AND `npcflag`=0 AND `unit_flags`=0 AND `unit_flags2`=0 AND `unit_flags3`=0;

UPDATE `creature_template` SET `AIName`='SmartAI', `ScriptName`='' WHERE `entry`=257663;
DELETE FROM `smart_scripts` WHERE `entryorguid`=257663 AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(257663,0,0,1,'',25,0,100,0,0,0,0,0,0,'',90,7,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Reset - Set Stand State Dead'),
(257663,0,1,0,'',61,0,100,0,0,0,0,0,0,'',81,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Reset - Set Npc Flags None'),
(257663,0,2,3,'',8,0,100,0,8593,0,0,0,0,'',90,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Spellhit Symbol of Life - Set Stand State Stand'),
(257663,0,3,4,'',61,0,100,0,0,0,0,0,0,'',81,2,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Spellhit Symbol of Life - Set Npc Flag Questgiver'),
(257663,0,4,0,'',61,0,100,0,0,0,0,0,0,'',67,1,120000,120000,0,0,100,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Spellhit Symbol of Life - Create Timed Event 1 (120 s)'),
(257663,0,5,6,'',59,0,100,0,1,0,0,0,0,'',90,7,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Timed Event 1 - Set Stand State Dead'),
(257663,0,6,0,'',61,0,100,0,0,0,0,0,0,'',81,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Deathguard Falgan - On Timed Event 1 - Set Npc Flags None');
