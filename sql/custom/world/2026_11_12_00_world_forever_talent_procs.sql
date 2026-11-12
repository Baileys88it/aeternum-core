SET NAMES utf8mb4;
-- Forever 1.60.1.70888: talent proc data. In the 70888 client data these talents are single trait spells with ProcChance 100;
-- the real chance per rank is the amount of effect 0 (TraitDefinitionEffectPoints curve). Chance 100 here + the generic
-- amount-roll AuraScript classic_spell_weapon_extra_attack_chance; family masks restrict the triggering spells.
-- Buffs with charges and spell modifiers get REQ_SPELLMOD (8) so one use consumes them.
DELETE FROM `spell_proc` WHERE `SpellId` IN (11213,12536,400588,400589,11071,12289,12311,13754,13976,14156,19228,19407,15257,15326,16164,16246,16880,17079,17082,12043,16188,17116,18708,14177,20216,14751,18288);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
-- Mage
(11213,0,3,0,0,0,0,65536,0,1,2,0,0,2,0,100,1000,0),               -- Arcane Concentration: mage damage spell hits, 2/4/6/8/10%, 1 s
(12536,0,3,549591799,167936,268697664,540672,87376,0,7,1,0,8,0,0,0,0,1), -- Clearcasting: used by the next affected spell
(400588,0,3,536870945,4096,0,0,65536,0,1,2,0,0,2,0,100,0,0),       -- Missile Barrage: Arcane Blast, Fireball, Frostbolt, Frostfire Bolt, 20/40%
(400589,0,3,2048,0,0,0,65536,0,7,2,0,8,0,0,0,0,1),                 -- Missile Barrage buff: used by the next Arcane Missiles
(11071,0,3,1048576,0,0,0,65536,0,7,2,0,0,0,0,100,0,0),             -- Frostbite: Chill effects, 5/10/15%
(12043,0,3,1073741824,0,0,0,87376,0,7,1,0,8,0,0,0,0,1),            -- Presence of Mind: used by the next affected spell
-- Warrior
(12289,0,4,2,0,0,0,87376,0,7,2,0,0,0,0,100,0,0),                   -- Improved Hamstring: Hamstring, 5/10/15%
(12311,0,4,2048,0,0,0,87376,0,7,2,0,0,0,0,100,0,0),                -- Improved Shield Bash: Shield Bash, 50/100%
-- Rogue
(13754,0,8,16,0,0,0,87376,0,7,2,0,0,0,0,100,0,0),                  -- Improved Kick: Kick, 50/100%
(13976,0,8,1792,0,0,0,16,0,7,2,0,0,0,0,100,0,0),                   -- Initiative: Ambush, Garrote, Cheap Shot, 33/67/100%
(14156,0,8,4063232,0,0,0,4112,0,7,2,0,0,0,0,100,500,0),            -- Ruthlessness: finishing moves, 20/40/60%
(14177,0,8,131590,6,0,0,87376,0,7,2,0,8,0,0,0,0,1),                -- Cold Blood: used by the next affected ability
-- Hunter
(19228,0,9,64,0,0,0,87376,0,7,2,0,0,0,0,100,0,0),                  -- Improved Wing Clip: Wing Clip, 7/13/20%
(19407,0,9,512,0,0,0,87376,0,7,2,0,0,0,0,100,0,0),                 -- Improved Concussive Shot: Concussive Shot, 4/8/12/16/20%
-- Priest
(15257,32,6,0,0,0,0,65536,0,1,2,0,0,0,0,100,0,0),                  -- Shadow Weaving: Shadow damage spells, 33/67/100%
(15326,32,6,0,0,0,0,65536,0,1,2,0,0,0,0,100,0,0),                  -- Blackout: Shadow damage spells, 2/4/6/8/10%
(14751,0,6,3724017659,278758454,83952128,270533776,87376,0,7,2,0,8,0,0,0,0,1), -- Inner Focus: used by the next affected spell
-- Shaman
(16164,28,11,0,0,0,0,65536,0,1,2,0,0,0,0,100,0,0),                 -- Elemental Focus: Fire, Frost, Nature damage spells, 10%
(16246,0,11,2551185411,4096,0,0,87376,0,7,1,0,8,0,0,0,0,1),        -- Clearcasting (shaman): used by the next affected spell
(16188,0,11,2499,0,1048576,65536,87376,0,7,1,0,8,0,0,0,0,1),       -- Nature's Swiftness (shaman): used by the next affected spell
-- Druid
(16880,0,7,0,0,0,0,81920,0,3,2,2,0,0,0,0,0,0),                     -- Nature's Grace: druid spell critical hits only
(17079,0,7,524288,0,0,0,87376,0,7,2,0,0,0,0,0,0,0),                -- Improved Enrage (1): Enrage only
(17082,0,7,524288,0,0,0,87376,0,7,2,0,0,0,0,0,0,0),                -- Improved Enrage (2): Enrage only
(17116,0,7,16777825,33554432,33554432,0,87376,0,7,1,0,8,0,0,0,0,1),-- Nature's Swiftness (druid): used by the next affected spell
-- Warlock
(18708,0,5,536870912,0,0,0,87376,0,7,1,0,8,0,0,0,0,1),             -- Fel Domination: used by the next demon summon
(18288,0,5,32768,0,262144,0,87376,0,7,2,0,8,0,0,0,0,1),            -- Amplify Curse: used by the next affected curse
-- Paladin
(20216,0,10,2149605376,0,0,0,81920,0,7,2,0,8,0,0,0,0,1);           -- Divine Favor: used by the next affected heal

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'classic_spell_weapon_extra_attack_chance' AND `spell_id` IN (11213,400588,11071,12289,12311,13754,13976,14156,19228,19407,15257,15326,16164);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(11213,'classic_spell_weapon_extra_attack_chance'),(400588,'classic_spell_weapon_extra_attack_chance'),(11071,'classic_spell_weapon_extra_attack_chance'),
(12289,'classic_spell_weapon_extra_attack_chance'),(12311,'classic_spell_weapon_extra_attack_chance'),(13754,'classic_spell_weapon_extra_attack_chance'),
(13976,'classic_spell_weapon_extra_attack_chance'),(14156,'classic_spell_weapon_extra_attack_chance'),(19228,'classic_spell_weapon_extra_attack_chance'),
(19407,'classic_spell_weapon_extra_attack_chance'),(15257,'classic_spell_weapon_extra_attack_chance'),(15326,'classic_spell_weapon_extra_attack_chance'),
(16164,'classic_spell_weapon_extra_attack_chance');
