-- Sen'jin Village (Durotar): remove the duplicate mount-vendor display raptors at Zjolnir (guid 20006403).
--
-- Evidence (world, 2026-11-09): 12 display raptors overlapped, two sets with the same models
-- within ~0-3 yd of each other:
--   Forever remaster set (keep):  267291/267297/267298 + 276800/276801/276803  guids 21200603-5, 21200668-70
--   Classic set (delete):         12346/12349/12350 + 14543/14544/14545        guids 20006406-8, 20095104-6
-- Result: 6 display mounts at Zjolnir (3 normal + 3 epic), the Forever remaster ones.
-- Idempotent: deletes by exact guid list only (creature rows also by entry).
-- No linked rows existed on 2026-11-09 (addon, formations, linked_respawn, pools, events, spawn groups,
-- smart_scripts, overrides); the deletes below keep the cleanup complete if any appear.

DELETE FROM `creature_addon` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `creature_formations` WHERE `memberGUID` IN (20006406,20006407,20006408,20095104,20095105,20095106)
   OR `leaderGUID` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `linked_respawn` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106)
   OR `linkedGuid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `pool_members` WHERE `type`=0 AND `spawnId` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `game_event_creature` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `game_event_model_equip` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `game_event_npc_vendor` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `game_event_npcflag` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `spawn_group` WHERE `spawnType`=0 AND `spawnId` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `creature_movement_override` WHERE `SpawnId` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `creature_static_flags_override` WHERE `SpawnId` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `vehicle_accessory` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106);
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND `entryorguid` IN (-20006406,-20006407,-20006408,-20095104,-20095105,-20095106);
DELETE FROM `creature` WHERE `guid` IN (20006406,20006407,20006408,20095104,20095105,20095106)
  AND `id` IN (12346,12349,12350,14543,14544,14545);
