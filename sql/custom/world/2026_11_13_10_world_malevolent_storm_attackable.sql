-- Zephras Isle: Malevolent Storm (255830) loses immune-to-players so the first spire storms can be fought (players kill and loot them on live).
-- Source: buildthehomelab/wow-trinitycore-everlake b0af761752 (2026_11_12_80_world_zephras_scene_npcs.sql, part 2 only; EG Link deaths 70245/70291/70790, Icy Veins loot).
UPDATE `creature_template` SET `unit_flags` = `unit_flags` & ~0x100 WHERE `entry` = 255830;
