-- WoW Forever live data from a player's enUS cache folder (client 1.60.1.70888, written 2026-11-11), plus the same
-- creature records seen in the 2026-11-11 21:08 ruRU live capture (marked "sniff"). Blizzard's values, unchanged.
-- Every row below came from that cache or capture (source in the comment). Safe to run more than once.

-- Tameable flags and pet families from the same data are in 2026_11_11_70_world_forever_tameable_beasts_live_70888.sql
-- (tamed pets work). This file does not touch TypeFlags or family.

-- 1. Creature type live sends.
UPDATE `creature_template` SET `type` = 1 WHERE `entry` = 2671; -- Mechanical Squirrel, ours 12 (cache)
UPDATE `creature_template` SET `type` = 10 WHERE `entry` = 4059; -- Forest Spirit, ours 6 (cache)
UPDATE `creature_template` SET `type` = 12 WHERE `entry` = 7381; -- Silver Tabby, ours 1 (cache)
UPDATE `creature_template` SET `type` = 12 WHERE `entry` = 7385; -- Bombay, ours 1 (cache)
UPDATE `creature_template` SET `type` = 12 WHERE `entry` = 7555; -- Hawk Owl, ours 1 (cache)
UPDATE `creature_template` SET `type` = 8 WHERE `entry` = 9256; -- Farm Chicken, ours 12 (cache)
UPDATE `creature_template` SET `type` = 1 WHERE `entry` = 12207; -- Thessala Hydra, ours 10 (cache)

-- 2. Rank (elite, rare, rare elite) live sends.
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 471; -- Mother Fang, ours 4 (cache)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 2041; -- Ancient Protector, ours 0 (cache)
UPDATE `creature_template` SET `Classification` = 4 WHERE `entry` = 2753; -- Barnabus, ours 0 (sniff)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 4438; -- Razorfen Spearhide, ours 2 (sniff)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 9024; -- Pyromancer Loregrain, ours 2 (sniff)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 9041; -- Warder Stilgiss, ours 2 (sniff)
UPDATE `creature_template` SET `Classification` = 1 WHERE `entry` = 9042; -- Verek, ours 2 (sniff)
UPDATE `creature_template` SET `Classification` = 2 WHERE `entry` = 10820; -- Duke Ragereaver, ours 4 (sniff)
UPDATE `creature_template` SET `Classification` = 2 WHERE `entry` = 11383; -- High Priestess Hai'watna, ours 1 (sniff)

-- 3. NPCs ours had as warriors (no mana) that live gives a mana class.
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 2749; -- Siege Golem (sniff)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 4423; -- Darnassian Protector (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 8219; -- Zul'arek Hatefowler (sniff)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 247032; -- Lyn the Ignored (sniff)
UPDATE `creature_template` SET `unit_class` = 8 WHERE `entry` = 253272; -- Magram Necromancer (cache)
UPDATE `creature_template` SET `unit_class` = 8 WHERE `entry` = 255770; -- Redridge Mystic (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 255887; -- Slydris (sniff)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 255898; -- Murloc Oracle (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 256492; -- Fernfeather (sniff)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 258573; -- Mudfin Oracle (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 258574; -- Mudfin Warrior (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 259398; -- Galemender Delanea (sniff)
UPDATE `creature_template` SET `unit_class` = 4 WHERE `entry` = 268624; -- Razormane Raider (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 272203; -- Perith Stormhoof (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 274757; -- Temple Highguard (cache)
UPDATE `creature_template` SET `unit_class` = 8 WHERE `entry` = 274780; -- Damin Dawnshadow (cache)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 276105; -- Stormherald Ukta (sniff)
UPDATE `creature_template` SET `unit_class` = 2 WHERE `entry` = 276194; -- Dustwind Eggtender (sniff)

-- 4. Models for NPCs that have none in our data. Only when the NPC has no model row at all.
-- Section 4 (creature_template_model) left out: NPCs here that exist already have a model; the other 13 have no creature_template (orphans).
