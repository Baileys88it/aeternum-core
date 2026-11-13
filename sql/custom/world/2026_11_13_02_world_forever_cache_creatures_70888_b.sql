-- WoW Forever: a Russian player's game cache folder of client 1.60.1.70888 (Cache\WDB\ruRU, Blizzard's live realm),
-- written 2026-11-12 00:27, sent as "Cache 4.rar". Read with tools/forever_cache. Blizzard's values, unchanged.
-- Russian texts: only text with Russian letters, only missing rows and empty fields are filled, existing text is kept,
-- only for quests, NPCs and objects our world has. Safe to run more than once.

-- 1. Creature type live sends.
UPDATE `creature_template` SET `type` = 12 WHERE `entry` = 7382; -- Orange Tabby, ours 1
UPDATE `creature_template` SET `type` = 12 WHERE `entry` = 7384; -- Cornish Rex, ours 1
UPDATE `creature_template` SET `type` = 6 WHERE `entry` = 272273; -- Lost Watcher, ours 7
UPDATE `creature_template` SET `type` = 6 WHERE `entry` = 272291; -- Lost Knight, ours 7

-- 2. Rank (elite, rare, rare elite) live sends.
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 1748; -- Highlord Bolvar Fordragon, ours 3
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 1756; -- Stormwind Royal Guard, ours 0

-- 3. NPCs ours had as warriors (no mana) that live gives a mana class.
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 248464; -- Nimsy
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 258139; -- Frothrik Saegrund
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 258203; -- Hervdana Saegrund
UPDATE `creature_template` SET `unit_class` = 8 WHERE `entry` = 258785; -- High Priestess Mims
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 271338; -- Ados
UPDATE `creature_template` SET `unit_class` = 4 WHERE `entry` = 271346; -- Dragonmaw Infiltrator
UPDATE `creature_template` SET `unit_class` = 8 WHERE `entry` = 271348; -- Dragonmaw Soulbinder
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 271460; -- Modr
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 271462; -- Golm
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 271486; -- Wendigo Shaman
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 272291; -- Lost Knight

-- 4. Models for NPCs that have none in our data. Only when the NPC has no model row at all.

-- (sections 5-7 Russian locale texts left out on purpose)
