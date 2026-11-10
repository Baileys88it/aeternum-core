-- Aeternum: selected WoW Forever world fixes from the upstream Forever branch sync of 2026-11-09.
-- Source: https://github.com/Karrz1/TrinityCore (formerly advocaite/TrinityCore), branch forever, commit f3dab8fec6
-- ("Sync with WoW Forever 0.1.34", ff699fb5cf), sql/custom/world/<file> as named in each section. Repository license GPL-2.0.
-- Data credits as given by each section: Questie team (Questie/QuestieDB, GPL-3.0), players' Forever client caches (build 70862),
-- Blizzard client data 70685/70790/70862.
-- Aeternum selection rules: only Forever-live data and plain bug fixes; nothing marked INFERRED/guessed, no VMaNGOS conversions,
-- no placeholder quests; every section was test-applied in a transaction against world first.
-- Left out of 2026_11_08_12: part 2 (Bloodied Insignia droppers with an INFERRED 4% chance).
-- Rollback: sql/custom/rollback/world/2026_11_09_20_world_forever_upstream_0134_selected_rollback.sql (exact pre-image rows).

-- ====================================================================
-- upstream sql/custom/world/2026_11_06_10_world_forever_questie_1105_fixes.sql
-- ====================================================================
-- Questie Forever corrections (Questie team, Questie/QuestieDB, master 80866960, 2026-11-05, GPLv3), hand fixes checked
-- against our world data.
-- A Light in the Darkness (98389, Aramis Hammerhand in Deathknell): Questie's traces show it only after The Damned (376).
-- We had no addon row, so it was offered before 376.
INSERT INTO `quest_template_addon` (`ID`,`PrevQuestID`) VALUES (98389,376) ON DUPLICATE KEY UPDATE `PrevQuestID`=376;
-- Divine Grace (94773-94779) and Confounding Flash (94817, 94821-94826) are one-of-a-set quests. Only 94774 and 94817
-- exist in our data; give them their group now so the rest of each set joins it when added.
UPDATE `quest_template_addon` SET `ExclusiveGroup`=94773 WHERE `ID`=94774;
UPDATE `quest_template_addon` SET `ExclusiveGroup`=94817 WHERE `ID`=94817;


-- ====================================================================
-- upstream sql/custom/world/2026_11_06_40_world_forever_shaman_totem_spells.sql
-- ====================================================================
-- Shaman totems (Discord, brignar 2026-11-06, Dwarf shaman on 0.1.30.1): Strength of Earth and Stoneskin Totem come up but give no
-- buff. Not a faction thing: the totem NPC casts the spell in its creature_template_spell row, and our world data has that row
-- only for Earthbind, Tremor, Grounding, Searing, Magma and Healing Stream. Every other shaman totem (Strength of Earth, Stoneskin,
-- Stoneclaw, Mana Spring, Mana Tide, Windfury, Grace of Air, Flametongue, the resistance totems, Windwall, Poison and Disease
-- Cleansing) stood there doing nothing, for players and bots of both factions. Searing, Magma and Healing Stream had a row, but every
-- rank cast the rank 1 spell (3606, 8188, 5672), so higher ranks hit and healed like rank 1. Grounding cast its magnet twice
-- directly (8178), so it took one spell and never came back; it now casts Grounding Totem Passive 8179, which puts the magnet back
-- every 10 seconds as in Classic.
-- One row per totem NPC, its own rank's spell, matched on the 70685 client data (the summon spell's NPC and the totem spell of the
-- same name and rank). Earthbind, Tremor, Sentry and the boss totems keep their rows. Safe to run again.
DELETE FROM `creature_template_spell` WHERE `CreatureID` IN (5873,5919,5920,7366,7367,7368,5874,5921,5922,7403,15464,3579,3911,3912,3913,7398,7399,3527,3906,3907,3908,3909,3573,7414,7415,7416,10467,11100,11101,5923,5924,5927,7424,7425,2523,3902,3903,3904,7400,7402,5929,7464,7465,7466,5926,7412,7413,5950,6012,7423,10557,6112,7483,7484,7486,7487,15463,7467,7468,7469,9687,9688,9689,5925);
INSERT INTO `creature_template_spell` (`CreatureID`,`Index`,`Spell`,`VerifiedBuild`) VALUES
(5873,0,8072,0),
(5919,0,8156,0),
(5920,0,8157,0),
(7366,0,10403,0),
(7367,0,10404,0),
(7368,0,10405,0),
(5874,0,8076,0),
(5921,0,8162,0),
(5922,0,8163,0),
(7403,0,10441,0),
(15464,0,25362,0),
(3579,0,5728,0),
(3911,0,6397,0),
(3912,0,6398,0),
(3913,0,6399,0),
(7398,0,10425,0),
(7399,0,10426,0),
(3527,0,5672,0),
(3906,0,6371,0),
(3907,0,6372,0),
(3908,0,10460,0),
(3909,0,10461,0),
(3573,0,5677,0),
(7414,0,10491,0),
(7415,0,10493,0),
(7416,0,10494,0),
(10467,0,16191,0),
(11100,0,17355,0),
(11101,0,17360,0),
(5923,0,8167,0),
(5924,0,8172,0),
(5927,0,8185,0),
(7424,0,10534,0),
(7425,0,10535,0),
(2523,0,3606,0),
(3902,0,6350,0),
(3903,0,6351,0),
(3904,0,6352,0),
(7400,0,10435,0),
(7402,0,10436,0),
(5929,0,8188,0),
(7464,0,10582,0),
(7465,0,10583,0),
(7466,0,10584,0),
(5926,0,8182,0),
(7412,0,10476,0),
(7413,0,10477,0),
(5950,0,8230,0),
(6012,0,8250,0),
(7423,0,10521,0),
(10557,0,15036,0),
(6112,0,8515,0),
(7483,0,10609,0),
(7484,0,10612,0),
(7486,0,8836,0),
(7487,0,10626,0),
(15463,0,25360,0),
(7467,0,10596,0),
(7468,0,10598,0),
(7469,0,10599,0),
(9687,0,15108,0),
(9688,0,15109,0),
(9689,0,15110,0),
(5925,0,8179,0);


-- ====================================================================
-- upstream sql/custom/world/2026_11_06_60_world_forever_quest_items_in_bags.sql
-- ====================================================================
-- Skyborne start zone (Zephras Isle) report by sombi4, 11-06: quest items never reached the bags, and the
-- objectives did not count, on Harvesting Windstones 93552, Aggressive Encroachment 92473, Foul Matriarch 92470,
-- The Problem With Prideclaws 92515, A Little Beauty 93951, Restocking the Larders 92553, WANTED: Vulgara 93318,
-- The Criminal Element 92517, Stolen Supplies 92551, Havoc in the Highlands 92550 and A Last Request 93927.
-- Cause: these item objectives have Flags2 bit 1 (quest-bound item, from the 70124 sniff). With that bit the core never
-- creates the item; it only bumps a counter in the quest log. The 1.60 client keeps quest items in the bags like
-- classic does, so the counter did not show and nothing appeared. Clearing the bit makes the looted item go to the
-- bags and count from there, as on the vanilla quests. Same bit on every other item objective (186 on live quests,
-- the rest retail rows nobody can take), so it is cleared everywhere.
-- Bugged 93949 (Valanaar, kill 8 Enchanted Skyhoppers): its credit creature 257066 has no template and nothing gives
-- it, so kills stayed at 0/8. The objective now counts Skyhopper 251727, and the second Skyhopper template 251314
-- (spawned in the same place) gives the same credit.
-- Found and tested by sombi4 (Discord #bug-report, 11-06). Safe to run twice.
UPDATE `quest_objectives` SET `Flags2` = `Flags2` & ~1 WHERE `Type` = 1 AND (`Flags2` & 1) <> 0;
UPDATE `quest_objectives` SET `ObjectID` = 251727 WHERE `ID` = 468000 AND `QuestID` = 93949 AND `ObjectID` = 257066;
UPDATE `creature_template` SET `KillCredit1` = 251727 WHERE `entry` = 251314 AND `KillCredit1` = 0;


-- ====================================================================
-- upstream sql/custom/world/2026_11_06_75_world_forever_totem_names_no_zzold.sql
-- ====================================================================
-- Shaman totem NPCs of the old ranks carry the data name prefix zzOLD (e.g. 'zzOLDStrength of Earth Totem V'). Bots and players summon those ranks
-- (the totem spell of that rank summons that entry), so the totem showed that name over its head. Remove the prefix from the totem NPC
-- names and from their locale rows; the spells, ranks and auras are not touched. Safe to run again.
UPDATE `creature_template` SET `name` = TRIM(SUBSTRING(`name`, 6)) WHERE `entry` IN (3902,3903,3904,3906,3907,3908,3909,3911,3912,3913,5879,5919,5920,5921,5922,6012,6110,6111,7366,7367,7368,7398,7399,7400,7402,7403,7412,7413,7414,7415,7416,7423,7424,7425,7464,7465,7466,7468,7469,7483,7484,7486,7487,7845,9687,9688,9689,10557,11100,11101,15463,15464,15470,15474,15478,15479,15480,15482,15484,15485,15486,15487,15488,15489,15490,15492,15496,15497,17061,22970,22971,30647,30652,30653,30654,31120,31121,31122,31129,31132,31133,31158,31162,31164,31165,31166,31167,31169,31170,31171,31172,31173,31174,31175,31176,31181,31182,31185,31186,31189,31190,32775,32776) AND `name` LIKE 'zzOLD%';
UPDATE `creature_template_locale` SET `Name` = TRIM(SUBSTRING(`Name`, 6)) WHERE `entry` IN (3902,3903,3904,3906,3907,3908,3909,3911,3912,3913,5879,5919,5920,5921,5922,6012,6110,6111,7366,7367,7368,7398,7399,7400,7402,7403,7412,7413,7414,7415,7416,7423,7424,7425,7464,7465,7466,7468,7469,7483,7484,7486,7487,7845,9687,9688,9689,10557,11100,11101,15463,15464,15470,15474,15478,15479,15480,15482,15484,15485,15486,15487,15488,15489,15490,15492,15496,15497,17061,22970,22971,30647,30652,30653,30654,31120,31121,31122,31129,31132,31133,31158,31162,31164,31165,31166,31167,31169,31170,31171,31172,31173,31174,31175,31176,31181,31182,31185,31186,31189,31190,32775,32776) AND `Name` LIKE 'zzOLD%';


-- ====================================================================
-- upstream sql/custom/world/2026_11_06_76_world_forever_totem_name_7844_no_zzold.sql
-- ====================================================================
-- Fire Nova Totem IV (7844) was left out of 2026_11_06_75: its esES, esMX, itIT, koKR, ptBR and zhTW names still had the zzOLD prefix. Safe to run again.
UPDATE `creature_template_locale` SET `Name` = TRIM(SUBSTRING(`Name`, 6)) WHERE `entry` = 7844 AND `Name` LIKE 'zzOLD%';


-- ====================================================================
-- upstream sql/custom/world/2026_11_06_77_world_forever_totem_names_frfr_no_vieux.sql
-- ====================================================================
-- French names of old totem ranks carry the data prefix [VIEUX] (the frFR form of zzOLD), so the totem showed it over its head. Remove it from these totem NPCs. Safe to run again.
UPDATE `creature_template_locale` SET `Name` = TRIM(SUBSTRING(`Name`, 8)) WHERE `entry` IN (5879,6110,6111,7844,7845,15482,30652,30653,32775,32776) AND `locale` = 'frFR' AND `Name` LIKE '[VIEUX]%';


-- ====================================================================
-- upstream sql/custom/world/2026_11_07_10_world_forever_lights_hope_sentries_walk.sql
-- ====================================================================
-- Light's Hope Chapel: the Argent Sentries (16378) swam in the air. The retail difficulty row they keep from TDB has
-- CREATURE_STATIC_FLAG_FLOATING (0x20000000), which turns gravity off and plays the hover animation (lying flat while they
-- walk, legs dangling while they stand). They are plain guards on foot, as in vanilla, so the flag goes.
-- Checked: of the vanilla creatures that are spawned and carry this flag, the sentry is the only humanoid; the rest are birds,
-- bats, wisps, dragons, gargoyles and invisible triggers, which are right to float.
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` & ~0x20000000 WHERE `Entry` = 16378;


-- ====================================================================
-- upstream sql/custom/world/2026_11_07_61_world_forever_exploring_the_alliance_talk_flags.sql
-- ====================================================================
-- Exploring the Alliance (93963, Skyborne): "Speak with High Tinker Mekkatorque / King Magni Bronzebeard / Tyrande
-- Whisperwind" could not be done (Discord, Russian player 2026-11-07: "can't click to talk to Mekkatorque and Bronzebeard").
-- Cause: the three have only the quest-giver flag (2). With no quest to offer, a click opens nothing, so the talk is never
-- sent and the talk-to objective is never credited. Same fault the Horde side had (Vol'jin, Nazgrel: 2026_10_27_16).
-- Fix: add the gossip flag. A click then always opens their window (their quests stay in it) and credits the objective.
-- These were the only Forever talk-to objectives left on NPCs without the gossip flag (checked against the 10-27 world).
-- Safe to run again.
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` IN (7937, 2784, 7999);


-- ====================================================================
-- upstream sql/custom/world/2026_11_07_90_world_forever_quest_chains.sql
-- ====================================================================
-- Quest chains: Forever quests could all be taken at once because the quest before them was never required.
-- Player report 2026-11-07 (#общий, alexia5746): in the Westfall Moonbrook/Deadmines chain 92745-92753 every quest can be
-- taken without the one before, and 92750 Detonation at a Distance too. The restore of Forever quest data (2026_11_05_99)
-- brought these quests back from data that never had their chain links, so PrevQuestID stayed 0 and no condition was set.
-- Checked every quest 80000-99999 that has a giver here against the Questie project's Forever quest data
-- (github.com/Questie/QuestieDB, src/corrections/Forever and data/Forever, GPLv3, chains recorded from the live game),
-- and, where Questie has no chain for a quest, against Wowhead quest series and the quest texts. Real starters stay open.
-- Only quests with no chain link at all get one; existing links are left as they are, except Tears of the Lady, Feathers
-- for Binding and Unwanted and Unworthy, which now also accept A Sacrifice in Vain (the other way into the same story).
-- One previous quest: PrevQuestID. All of several: conditions in one group. Any one of several: conditions in separate groups.
-- Safe to run again.

-- 1) One previous quest, from Questie's Forever data
INSERT INTO `quest_template_addon` (`ID`,`PrevQuestID`) VALUES
(91208,90902),
(91316,91294),
(91724,91723),
(92422,99156),
(92468,92467),
(92597,92461),
(92698,92679),
(92741,92699),
(92747,92745),
(92748,92747),
(92749,92748),
(92750,92748),
(92751,92750),
(92753,92752),
(92911,92910),
(93736,92700),
(94473,94466),
(95216,451),
(95803,91317),
(95805,755),
(95981,482),
(96630,921),
(96652,794),
(96659,757),
(96897,96895),
(96898,96895),
(97977,458),
(97979,456),
(98046,935),
(98065,98046),
(98386,89),
(98435,760),
(99049,99048),
(99051,99049),
(99052,99051),
(99073,99050),
(99129,99128),
(99152,96896),
(99153,96896),
(99160,99159),
(99161,99160),
(99162,99159)
ON DUPLICATE KEY UPDATE `PrevQuestID` = VALUES(`PrevQuestID`);
--   91208 Coming to Terms  <-  90902 Rediscovering the Light
--   91316 Making Repairs  <-  91294 Touring the Grounds
--   91724 Delicate Instruments  <-  91723 Delicate Instruments
--   92422 The Wrath of Rath'mael  <-  99156 Rear Guard Patrol
--   92468 Call of Earth  <-  92467 Call of Earth
--   92597 Reading the Ley Lines  <-  92461 Harmony in Balance
--   92698 What Is My Purpose?  <-  92679 Blood Tithe
--   92741 Unwelcome Visitors  <-  92699 The Supreme Magister
--   92747 Moonbrook Espionage  <-  92745 The State of the Mines
--   92748 Explosive Consultation  <-  92747 Moonbrook Espionage
--   92749 A Dynamite Plan  <-  92748 Explosive Consultation
--   92750 Detonation at a Distance  <-  92748 Explosive Consultation
--   92751 Detonation at a Distance  <-  92750 Detonation at a Distance
--   92753 Destruction in Deadmines  <-  92752 Explosive Consultation
--   92911 Harvesting the Harvesters  <-  92910 Harvesting the Harvesters
--   93736 Unwelcome Spirits  <-  92700 The Grand Skyseer
--   94473 Fire Sapta  <-  94466 Call of Fire
--   95216 The New Plague  <-  451 A Recipe For Death
--   95803 A Token of Good Faith  <-  91317 The Tarnished
--   95805 Grace of An'she and Mu'sha  <-  755 Rites of the Earthmother
--   95981 Watching the Roads  <-  482 Dalaran's Intentions
--   96630 The Adventurer  <-  921 Crown of the Earth
--   96652 The Adventurer  <-  794 Burning Blade Medallion
--   96659 The Adventurer  <-  757 Rite of Strength
--   96897 The Cult of the Damned  <-  96895 The Argent Emissary
--   96898 Remnants of War  <-  96895 The Argent Emissary
--   97977 Nature's Call  <-  458 The Woodland Protector
--   97979 The Goddess Provides  <-  456 The Balance of Nature
--   98046 Crown of the Earth  <-  935 Crown of the Earth
--   98065 Crown of the Earth  <-  98046 Crown of the Earth
--   98386 Alther's Mill  <-  89 The Everstill Bridge
--   98435 Thunderhorn's Report  <-  760 Wildmane Cleansing
--   99049 Threat from Below  <-  99048 A Missing Hand
--   99051 Threat from Below  <-  99049 Threat from Below
--   99052 Threat from Below  <-  99051 Threat from Below
--   99073 Easing Suffering  <-  99050 The Great Tree Provides
--   99129 A Man About a Murloc  <-  99128 Slimy Menace
--   99152 As Above, So Below  <-  96896 A Righteous Cause
--   99153 The One That Got Away  <-  96896 A Righteous Cause
--   99160 Rime's Wrath  <-  99159 Finding Warmth
--   99161 Rime's Wrath  <-  99160 Rime's Wrath
--   99162 Treacherous Cold  <-  99159 Finding Warmth

-- 2) One previous quest, from Wowhead quest series or the quest text (Questie has no chain for these)
INSERT INTO `quest_template_addon` (`ID`,`PrevQuestID`) VALUES
(88758,87318),
(91285,91282),
(91860,91859),
(91861,91860),
(91862,91861),
(93035,93034),
(93128,93062),
(93227,93224),
(94495,94494),
(94872,93225),
(94873,93225),
(94914,94912),
(95111,95034),
(95125,95111),
(95126,95125),
(95140,95126),
(95508,95621),
(96204,91862),
(98219,98191),
(98221,473),
(98300,98297),
(98394,98393),
(98396,98394),
(98404,98341),
(98731,98396)
ON DUPLICATE KEY UPDATE `PrevQuestID` = VALUES(`PrevQuestID`);
--   88758 Sentiments of the Lost  <-  87318 Hearts of the Lost: Wowhead series Hearts of the Lost (Accessories of the Lost between them is not in the game here)
--   91285 Murlocs at the Gates  <-  91282 A Second Home: turn-in NPC of A Second Home gives it; its follow-up Touring the Grounds already needs it
--   91860 A Grim Fate  <-  91859 A Curious Pair: Wowhead series Diplomatic Incident step 3
--   91861 Into Fenris Keep  <-  91860 A Grim Fate: Wowhead series step 4
--   91862 Lumina Windsinger  <-  91861 Into Fenris Keep: Wowhead series step 5
--   93035 Ravaging Ravagers  <-  93034 Valley of Bones: Wowhead series: after Valley of Bones, next to Outing Outcasts which already needs it
--   93128 Khan Jehn  <-  93062 Bone Burying: Wowhead series Bone Burying > Checking Things Out (not in the game here) > Khan Jehn
--   93227 Buying Friendship: Gelkis  <-  93224 Carved Centaur Totems: needs the bundle of carved totems from Carved Centaur Totems
--   94495 Call of Water  <-  94494 Call of Water: Wowhead series Call of Water, Norric takes the first one in
--   94872 Buying Friendship: Magram  <-  93225 Carved Centaur Totems: needs the bundle of carved totems from Carved Centaur Totems
--   94873 Buying Friendship: Gelkis  <-  93225 Carved Centaur Totems: needs the bundle of carved totems from Carved Centaur Totems
--   94914 Moonglade  <-  94912 Child of Nature: the Alliance Child of Nature ends at this giver; the Horde Moonglade already needs its Child of Nature
--   95111 An Underrated Talent  <-  95034 The Debt: Wowhead series The Debt (A Moon-Kissed Blade between them is not in the game here)
--   95125 Ott's Masterwork  <-  95111 An Underrated Talent: Wowhead series
--   95126 The Moonsilver Blade  <-  95125 Ott's Masterwork: Wowhead series
--   95140 Old Fire-Eye  <-  95126 The Moonsilver Blade: text: 'I see that you have your weapon now' after The Moonsilver Blade
--   95508 Unwelcome Guests  <-  95621 Trouble in the Valley: text: the deserter story from Trouble in the Valley
--   96204 The Windshaper's Wrath  <-  91862 Lumina Windsinger: Wowhead series step 6
--   98219 The Shamed Lieutenant  <-  98191 A Friend of the Family: text: 'Thank you again for delivering the statuette'
--   98221 From the Ashes  <-  473 Report to Captain Stoutfist: Wowhead series: next to War Banners, which already needs it
--   98300 Word to Ironforge  <-  98297 Stopping the Cycle: text: last part of Howin's story after Stopping the Cycle
--   98394 The Great Cat Spirit  <-  98393 The Great Cat Spirit: Wowhead series (Alliance cat form, same as the Horde chain)
--   98396 The Great Cat Spirit  <-  98394 The Great Cat Spirit: Wowhead series
--   98404 The Great Windborne Cat Spirit  <-  98341 The Great Windborne Cat Spirit: Wowhead series (Windborne cat form)
--   98731 Blessings of the Great Cat Spirit  <-  98396 The Great Cat Spirit: Wowhead series

-- 3) Several previous quests, as conditions (19 = quest available, 8 = quest rewarded)
UPDATE `quest_template_addon` SET `PrevQuestID` = 0 WHERE `ID` IN (87760,92515,92516,92553,92745,92752,93165,93318,93319,93951,94485,94486,94487,94490,96057,96899,97286,97925,98285);
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `ConditionTypeOrReference` = 8 AND `SourceEntry` IN (87760,92515,92516,92553,92745,92752,93165,93318,93319,93951,94485,94486,94487,94490,96057,96899,97286,97925,98285);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`) VALUES
(19,0,87760,0,0,8,0,4722,0,0,0,'Holy Diver: needs Beached Sea Turtle (4722) and the rest of its group'),
(19,0,87760,0,0,8,0,4723,0,0,0,'Holy Diver: needs Beached Sea Creature (4723) and the rest of its group'),
(19,0,87760,0,0,8,0,4725,0,0,0,'Holy Diver: needs Beached Sea Turtle (4725) and the rest of its group'),
(19,0,87760,0,0,8,0,4727,0,0,0,'Holy Diver: needs Beached Sea Turtle (4727) and the rest of its group'),
(19,0,87760,0,0,8,0,4728,0,0,0,'Holy Diver: needs Beached Sea Creature (4728) and the rest of its group'),
(19,0,87760,0,0,8,0,4730,0,0,0,'Holy Diver: needs Beached Sea Creature (4730) and the rest of its group'),
(19,0,87760,0,0,8,0,4731,0,0,0,'Holy Diver: needs Beached Sea Turtle (4731) and the rest of its group'),
(19,0,87760,0,0,8,0,4732,0,0,0,'Holy Diver: needs Beached Sea Turtle (4732) and the rest of its group'),
(19,0,87760,0,0,8,0,4733,0,0,0,'Holy Diver: needs Beached Sea Creature (4733) and the rest of its group'),
(19,0,92745,0,0,8,0,92742,0,0,0,'The State of the Mines: needs Testing the Wells (92742) and the rest of its group'),
(19,0,92745,0,0,8,0,92744,0,0,0,'The State of the Mines: needs Murloc Gills (92744) and the rest of its group'),
(19,0,92752,0,0,8,0,92749,0,0,0,'Explosive Consultation: needs A Dynamite Plan (92749) and the rest of its group'),
(19,0,92752,0,0,8,0,92751,0,0,0,'Explosive Consultation: needs Detonation at a Distance (92751) and the rest of its group'),
(19,0,94490,0,0,8,0,94485,0,0,0,'Ripped Missive: needs Tears of the Lady (94485) and the rest of its group'),
(19,0,94490,0,0,8,0,94486,0,0,0,'Ripped Missive: needs Feathers for Binding (94486) and the rest of its group'),
(19,0,94490,0,0,8,0,94487,0,0,0,'Ripped Missive: needs Unwanted and Unworthy (94487) and the rest of its group'),
(19,0,96899,0,0,8,0,96897,0,0,0,'Bandarion Keep: needs The Cult of the Damned (96897) and the rest of its group'),
(19,0,96899,0,0,8,0,96898,0,0,0,'Bandarion Keep: needs Remnants of War (96898) and the rest of its group'),
(19,0,92515,0,0,8,0,92514,0,0,0,'The Problem With Prideclaws: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,92515,0,1,8,0,93461,0,0,0,'The Problem With Prideclaws: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,92516,0,0,8,0,92514,0,0,0,'Hippogryph Harassment: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,92516,0,1,8,0,93461,0,0,0,'Hippogryph Harassment: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,92553,0,0,8,0,92514,0,0,0,'Restocking the Larders: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,92553,0,1,8,0,93461,0,0,0,'Restocking the Larders: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,93165,0,0,8,0,94484,0,0,0,'Mercy Falls on Deaf Ears: needs any one of its group, here Unnerving Silence (94484)'),
(19,0,93165,0,1,8,0,94493,0,0,0,'Mercy Falls on Deaf Ears: needs any one of its group, here A Sacrifice in Vain (94493)'),
(19,0,93318,0,0,8,0,92514,0,0,0,'WANTED: Vulgara the Insatiable: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,93318,0,1,8,0,93461,0,0,0,'WANTED: Vulgara the Insatiable: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,93319,0,0,8,0,92514,0,0,0,'Pilfered Windstones: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,93319,0,1,8,0,93461,0,0,0,'Pilfered Windstones: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,93951,0,0,8,0,92514,0,0,0,'A Little Beauty: needs any one of its group, here Welcome to Shen''dar Village (92514)'),
(19,0,93951,0,1,8,0,93461,0,0,0,'A Little Beauty: needs any one of its group, here Welcome to Shen''dar Village (93461)'),
(19,0,94485,0,0,8,0,94484,0,0,0,'Tears of the Lady: needs any one of its group, here Unnerving Silence (94484)'),
(19,0,94485,0,1,8,0,94493,0,0,0,'Tears of the Lady: needs any one of its group, here A Sacrifice in Vain (94493)'),
(19,0,94486,0,0,8,0,94484,0,0,0,'Feathers for Binding: needs any one of its group, here Unnerving Silence (94484)'),
(19,0,94486,0,1,8,0,94493,0,0,0,'Feathers for Binding: needs any one of its group, here A Sacrifice in Vain (94493)'),
(19,0,94487,0,0,8,0,94484,0,0,0,'Unwanted and Unworthy: needs any one of its group, here Unnerving Silence (94484)'),
(19,0,94487,0,1,8,0,94493,0,0,0,'Unwanted and Unworthy: needs any one of its group, here A Sacrifice in Vain (94493)'),
(19,0,96057,0,0,8,0,95998,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (95998)'),
(19,0,96057,0,1,8,0,96101,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96101)'),
(19,0,96057,0,2,8,0,96604,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96604)'),
(19,0,96057,0,3,8,0,96605,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96605)'),
(19,0,96057,0,4,8,0,96606,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96606)'),
(19,0,96057,0,5,8,0,96607,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96607)'),
(19,0,96057,0,6,8,0,96608,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96608)'),
(19,0,97286,0,0,8,0,78124,0,0,0,'Research Access: needs any one of its group, here Nar''thalas Almanac (78124)'),
(19,0,97286,0,1,8,0,78127,0,0,0,'Research Access: needs any one of its group, here The Dalaran Digest (78127)'),
(19,0,97286,0,2,8,0,78142,0,0,0,'Research Access: needs any one of its group, here Bewitchments and Glamours (78142)'),
(19,0,97286,0,3,8,0,78143,0,0,0,'Research Access: needs any one of its group, here ? (78143)'),
(19,0,97286,0,4,8,0,78145,0,0,0,'Research Access: needs any one of its group, here Arcanic Systems Manual (78145)'),
(19,0,97286,0,5,8,0,78146,0,0,0,'Research Access: needs any one of its group, here Goaz Scrolls (78146)'),
(19,0,97286,0,6,8,0,78147,0,0,0,'Research Access: needs any one of its group, here Crimes Against Anatomy (78147)'),
(19,0,97286,0,7,8,0,78148,0,0,0,'Research Access: needs any one of its group, here Runes of the Sorcerer-Kings (78148)'),
(19,0,97286,0,8,8,0,78149,0,0,0,'Research Access: needs any one of its group, here Fury of the Land (78149)'),
(19,0,97286,0,9,8,0,79091,0,0,0,'Research Access: needs any one of its group, here Archmage Antonidas: The Unabridged Autobiography (79091)'),
(19,0,97286,0,10,8,0,79092,0,0,0,'Research Access: needs any one of its group, here Archmage Theocritus'' Research Journal (79092)'),
(19,0,97286,0,11,8,0,79093,0,0,0,'Research Access: needs any one of its group, here Rumi of Gnomeregan: The Collected Works (79093)'),
(19,0,97286,0,12,8,0,79094,0,0,0,'Research Access: needs any one of its group, here The Lessons of Ta''zo (79094)'),
(19,0,97286,0,13,8,0,79095,0,0,0,'Research Access: needs any one of its group, here The Apothecary''s Metaphysical Primer (79095)'),
(19,0,97286,0,14,8,0,79096,0,0,0,'Research Access: needs any one of its group, here ? (79096)'),
(19,0,97286,0,15,8,0,79097,0,0,0,'Research Access: needs any one of its group, here Baxtan: On Destructive Magics (79097)'),
(19,0,97286,0,16,8,0,79535,0,0,0,'Research Access: needs any one of its group, here Basilisks: Should Petrification be Feared? (79535)'),
(19,0,97286,0,17,8,0,79947,0,0,0,'Research Access: needs any one of its group, here Geomancy: The Stone-Cold Truth (79947)'),
(19,0,97286,0,18,8,0,79948,0,0,0,'Research Access: needs any one of its group, here Defensive Magics 101 (79948)'),
(19,0,97286,0,19,8,0,79949,0,0,0,'Research Access: needs any one of its group, here A Web of Lies: Debunking Myths and Legends (79949)'),
(19,0,97286,0,20,8,0,79950,0,0,0,'Research Access: needs any one of its group, here Demons and You (79950)'),
(19,0,97286,0,21,8,0,79951,0,0,0,'Research Access: needs any one of its group, here Mummies: A Guide to the Unsavory Undead (79951)'),
(19,0,97286,0,22,8,0,79952,0,0,0,'Research Access: needs any one of its group, here RwlRwlRwlRwl! (79952)'),
(19,0,97286,0,23,8,0,79953,0,0,0,'Research Access: needs any one of its group, here ? (79953)'),
(19,0,97286,0,24,8,0,81947,0,0,0,'Research Access: needs any one of its group, here Sanguine Sorcery (81947)'),
(19,0,97286,0,25,8,0,81949,0,0,0,'Research Access: needs any one of its group, here Legends of the Tidesages (81949)'),
(19,0,97286,0,26,8,0,81951,0,0,0,'Research Access: needs any one of its group, here ? (81951)'),
(19,0,97286,0,27,8,0,81952,0,0,0,'Research Access: needs any one of its group, here Everyday Etiquette (81952)'),
(19,0,97286,0,28,8,0,81953,0,0,0,'Research Access: needs any one of its group, here Stonewrought Design (81953)'),
(19,0,97286,0,29,8,0,81954,0,0,0,'Research Access: needs any one of its group, here Venomous Journeys (81954)'),
(19,0,97286,0,30,8,0,81955,0,0,0,'Research Access: needs any one of its group, here A Mind of Metal (81955)'),
(19,0,97286,0,31,8,0,81956,0,0,0,'Research Access: needs any one of its group, here Conjurer''s Codex (81956)'),
(19,0,97286,0,32,8,0,84395,0,0,0,'Research Access: needs any one of its group, here Undead Potatoes (84395)'),
(19,0,97286,0,33,8,0,84396,0,0,0,'Research Access: needs any one of its group, here Magma or Lava? (84396)'),
(19,0,97286,0,34,8,0,84397,0,0,0,'Research Access: needs any one of its group, here Northern Kalimdor - A Comprehensive Guide (84397)'),
(19,0,97286,0,35,8,0,84398,0,0,0,'Research Access: needs any one of its group, here A Study of the Light (84398)'),
(19,0,97286,0,36,8,0,84399,0,0,0,'Research Access: needs any one of its group, here Ka-Boom! (84399)'),
(19,0,97286,0,37,8,0,84400,0,0,0,'Research Access: needs any one of its group, here The Knight and the Lady (84400)'),
(19,0,97286,0,38,8,0,84401,0,0,0,'Research Access: needs any one of its group, here Scourge: Undead Menace or Misunderstood? (84401)'),
(19,0,97286,0,39,8,0,84402,0,0,0,'Research Access: needs any one of its group, here Necromancy 101 (84402)'),
(19,0,97925,0,0,8,0,95998,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (95998)'),
(19,0,97925,0,1,8,0,96101,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96101)'),
(19,0,97925,0,2,8,0,96604,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96604)'),
(19,0,97925,0,3,8,0,96605,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96605)'),
(19,0,97925,0,4,8,0,96606,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96606)'),
(19,0,97925,0,5,8,0,96607,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96607)'),
(19,0,97925,0,6,8,0,96608,0,0,0,'Camping 101: Tailoring: needs any one of its group, here The Great Outdoors (96608)'),
(19,0,98285,0,0,8,0,95998,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (95998)'),
(19,0,98285,0,1,8,0,96101,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96101)'),
(19,0,98285,0,2,8,0,96604,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96604)'),
(19,0,98285,0,3,8,0,96605,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96605)'),
(19,0,98285,0,4,8,0,96606,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96606)'),
(19,0,98285,0,5,8,0,96607,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96607)'),
(19,0,98285,0,6,8,0,96608,0,0,0,'Camping 101: Engineering: needs any one of its group, here The Great Outdoors (96608)');

-- 4) Unnerving Silence and A Sacrifice in Vain are two ways into the same Nightclaw Den story: only one of them (Questie: exclusiveTo).
INSERT INTO `quest_template_addon` (`ID`,`ExclusiveGroup`) VALUES (94484,94484),(94493,94484)
ON DUPLICATE KEY UPDATE `ExclusiveGroup` = VALUES(`ExclusiveGroup`);


-- ====================================================================
-- upstream sql/custom/world/2026_11_07_96_world_forever_questie_1107_quest_links.sql
-- ====================================================================
-- Quest order fixes from the Questie project's hand-written Forever corrections (Questie team, Questie/QuestieDB,
-- src/corrections/Forever/foreverQuestFixes.lua, master 5fba5d86 / corrections b5a03ff, 2026-11-07, GPLv3).
-- Only chains still missing here after 2026_11_07_90 are set. Safe to run again.

-- 1) Ivar the Foul chain, Silverpine Forest: Return to Quinn (430) -> Wild Eyes (91920) -> Return to Quinn (Again) (91921, already linked)
--    -> Ivar the Foul (425). Wild Eyes and Ivar the Foul could be taken without the quest before.
INSERT INTO `quest_template_addon` (`ID`,`PrevQuestID`) VALUES
(91920,430),
(425,91921)
ON DUPLICATE KEY UPDATE `PrevQuestID`=VALUES(`PrevQuestID`);

-- 2) The Adventurer -> The Great Outdoors, one pair per starting area. Questie: each The Adventurer is a breadcrumb to its
--    The Great Outdoors (not a required quest), the seven The Great Outdoors are one-of-a-set, and so are the seven
--    The Adventurer. Here every The Great Outdoors needed its The Adventurer as PrevQuestID, so a player who skipped the
--    breadcrumb could never get it, and nothing stopped doing the same quest in a second starting area.
--    The Great Outdoors -> ExclusiveGroup 95998, no PrevQuestID; The Adventurer -> breadcrumb for its quest, ExclusiveGroup 96627.
--    The Adventurer keeps its own PrevQuestID.
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=95998 AND `PrevQuestID` IN (0,96627);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96101 AND `PrevQuestID` IN (0,96638);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96604 AND `PrevQuestID` IN (0,96652);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96605 AND `PrevQuestID` IN (0,96659);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96606 AND `PrevQuestID` IN (0,96630);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96607 AND `PrevQuestID` IN (0,96656);
UPDATE `quest_template_addon` SET `PrevQuestID`=0, `ExclusiveGroup`=95998 WHERE `ID`=96608 AND `PrevQuestID` IN (0,96628);
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=95998, `ExclusiveGroup`=96627 WHERE `ID`=96627;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96101, `ExclusiveGroup`=96627 WHERE `ID`=96638;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96604, `ExclusiveGroup`=96627 WHERE `ID`=96652;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96605, `ExclusiveGroup`=96627 WHERE `ID`=96659;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96606, `ExclusiveGroup`=96627 WHERE `ID`=96630;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96607, `ExclusiveGroup`=96627 WHERE `ID`=96656;
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96608, `ExclusiveGroup`=96627 WHERE `ID`=96628;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_12_world_forever_questie_1108_fixes.sql
-- ====================================================================
-- Fixes from the Questie project's hand-written Forever corrections (Questie team, Questie/QuestieDB,
-- src/corrections/Forever, master aa1d7e72, 2026-11-08, GPLv3). Safe to run again.

-- 1) Silverpine Forest: the Forever Arugal's Folly (98298) and Stop the Spread (98299) follow the classic Arugal's Folly (99)
--    at Dalar Dawnweaver. Ours had 98298 after Prove Your Worth (421), taken from a Wowhead page, and 98299 open to anyone.
INSERT INTO `quest_template_addon` (`ID`,`PrevQuestID`) VALUES
(98298,99),
(98299,99)
ON DUPLICATE KEY UPDATE `PrevQuestID`=VALUES(`PrevQuestID`);

-- Aeternum: part 2 (Bloodied Insignia droppers, INFERRED 4% chance) left out: no real drop rate.


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_22_world_forever_questie_quest_xp.sql
-- ====================================================================
-- Quest levels and XP from the Questie project's quest traces (Questie team, Questie/QuestieDB,
-- support/Forever/QuestXP/xpDB-traces.lua, master 3a621efa, 2026-11-08, GPLv3): the level and XP that players got from
-- 724 Forever quests in the live game. Compared with ours through the client's QuestXP table (1.15 / 1.60 columns).
-- 493 already match in XP. Changed here:
-- 1) Quest level: the level the live game sends for the quest (the XP and gray-quest level), where ours differs.
-- 2) XP: only where the live value is higher than ours. A lower live value is left alone: players above the quest's level
--    get 80/60/40/20/10% of it, so a low trace can be that cut and not the quest's real XP.
--    The difficulty column is the one that gives the live XP exactly at the live level; multiplier 1.
-- Safe to run again.

-- 1) quest level (ours -> live)
UPDATE `quest_classic_level` SET `QuestLevel`=27 WHERE `ID`=65610; -- Wish You Were Here: 20 -> 27
UPDATE `quest_classic_level` SET `QuestLevel`=27 WHERE `ID`=79363; -- Silvia's Sword: 26 -> 27
UPDATE `quest_classic_level` SET `QuestLevel`=10 WHERE `ID`=86667; -- Snowbound: 20 -> 10
UPDATE `quest_classic_level` SET `QuestLevel`=30 WHERE `ID`=86757; -- Slowing the Orc Advance: 32 -> 30
UPDATE `quest_classic_level` SET `QuestLevel`=8 WHERE `ID`=91723; -- Delicate Instruments: 10 -> 8
UPDATE `quest_classic_level` SET `QuestLevel`=7 WHERE `ID`=91732; -- Good Steel: 10 -> 7
UPDATE `quest_classic_level` SET `QuestLevel`=11 WHERE `ID`=91740; -- Croaky's Head: 10 -> 11
UPDATE `quest_classic_level` SET `QuestLevel`=12 WHERE `ID`=91746; -- Elmpaw's Head: 10 -> 12
UPDATE `quest_classic_level` SET `QuestLevel`=8 WHERE `ID`=91751; -- Rough Wolf Pelts: 7 -> 8
UPDATE `quest_classic_level` SET `QuestLevel`=15 WHERE `ID`=92747; -- Moonbrook Espionage: 16 -> 15
UPDATE `quest_classic_level` SET `QuestLevel`=13 WHERE `ID`=95328; -- Whispering Horror Residue: 10 -> 13
UPDATE `quest_classic_level` SET `QuestLevel`=5 WHERE `ID`=96130; -- Chakuyak: 8 -> 5
UPDATE `quest_classic_level` SET `QuestLevel`=25 WHERE `ID`=96137; -- Ira's Dagger: 24 -> 25
UPDATE `quest_classic_level` SET `QuestLevel`=30 WHERE `ID`=96138; -- Merrick's Bow: 28 -> 30
UPDATE `quest_classic_level` SET `QuestLevel`=23 WHERE `ID`=96139; -- The Valor Family: 22 -> 23
UPDATE `quest_classic_level` SET `QuestLevel`=29 WHERE `ID`=96241; -- Talkin' bout Toxins, Mon: 30 -> 29
UPDATE `quest_classic_level` SET `QuestLevel`=4 WHERE `ID`=96243; -- Call of Earth: 10 -> 4
UPDATE `quest_classic_level` SET `QuestLevel`=29 WHERE `ID`=96259; -- Defending the Dead: 30 -> 29
UPDATE `quest_classic_level` SET `QuestLevel`=29 WHERE `ID`=96260; -- Stronger than Steel: 30 -> 29
UPDATE `quest_classic_level` SET `QuestLevel`=7 WHERE `ID`=96822; -- For Honor: 6 -> 7
UPDATE `quest_classic_level` SET `QuestLevel`=12 WHERE `ID`=96896; -- A Righteous Cause: 13 -> 12
UPDATE `quest_classic_level` SET `QuestLevel`=9 WHERE `ID`=97223; -- Bloodtalon Matriarch: 8 -> 9
UPDATE `quest_classic_level` SET `QuestLevel`=15 WHERE `ID`=97253; -- Parts and Pieces: 14 -> 15
UPDATE `quest_classic_level` SET `QuestLevel`=10 WHERE `ID`=97282; -- Stormy Potential: 11 -> 10
UPDATE `quest_classic_level` SET `QuestLevel`=30 WHERE `ID`=98070; -- Stop the Screeching: 31 -> 30
UPDATE `quest_classic_level` SET `QuestLevel`=29 WHERE `ID`=98155; -- Pristine Pesterhide Pelts: 31 -> 29
UPDATE `quest_classic_level` SET `QuestLevel`=23 WHERE `ID`=98251; -- Never Coming Back: 22 -> 23
UPDATE `quest_classic_level` SET `QuestLevel`=12 WHERE `ID`=98545; -- Leonid's Letter: 13 -> 12
UPDATE `quest_classic_level` SET `QuestLevel`=9 WHERE `ID`=99048; -- A Missing Hand: 11 -> 9
UPDATE `quest_classic_level` SET `QuestLevel`=9 WHERE `ID`=99049; -- Threat from Below: 11 -> 9
UPDATE `quest_classic_level` SET `QuestLevel`=11 WHERE `ID`=99052; -- Threat from Below: 12 -> 11
UPDATE `quest_classic_level` SET `QuestLevel`=10 WHERE `ID`=99053; -- Escaping Ban'ethil: 9 -> 10
UPDATE `quest_classic_level` SET `QuestLevel`=11 WHERE `ID`=99080; -- Drive Them Out: 9 -> 11
UPDATE `quest_classic_level` SET `QuestLevel`=11 WHERE `ID`=99082; -- The High Chieftain: 9 -> 11
UPDATE `quest_classic_level` SET `QuestLevel`=12 WHERE `ID`=99152; -- As Above, So Below: 10 -> 12
UPDATE `quest_classic_level` SET `QuestLevel`=12 WHERE `ID`=99153; -- The One That Got Away: 10 -> 12

-- 2) XP (ours -> live)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=87760; -- Holy Diver: 0 -> 1550 (column 0 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91751; -- Rough Wolf Pelts: 475 -> 700 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91772; -- Shhh! We're Hunting Kobolds: 270 -> 355 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=92747; -- Moonbrook Espionage: 580 -> 800 (column 3 -> 4)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92911; -- Harvesting the Harvesters: 800 -> 1050 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95508; -- Unwelcome Guests: 1000 -> 1350 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95737; -- Seeking Caitlin: 630 -> 1250 (column 2 -> 3)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95805; -- Grace of An'she and Mu'sha: 180 -> 355 (column 3 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95810; -- Lost Relic Carry: 1228 -> 1250 (column 2 -> 3, multiplier 1.95 -> 1)
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96130; -- Chakuyak: 350 -> 335 (column 3 -> 4)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96821; -- Legging It: 405 -> 540 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96825; -- This Fruit Could Bite Back: 405 -> 540 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96874; -- This Is Spinal Axe: 630 -> 840 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96875; -- Beasts of Thunder Ridge: 630 -> 840 (column 4 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=96876; -- Ukor's Lost Pack: 530 -> 880 (column 4 -> 6)
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=96877; -- Halikor's Hoof: 680 -> 1150 (column 4 -> 6)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97894; -- Business in Auberdine: 0 -> 630 (column 0 -> 3)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97914; -- Expanding Horizons: 125 -> 630 (column 1 -> 3)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97926; -- Making Do: 125 -> 630 (column 1 -> 3)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98191; -- A Friend of the Family: 200 -> 2000 (column 1 -> 5)
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98319; -- Secure the Mountain: 175 -> 350 (column 2 -> 3)
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=99052; -- Threat from Below: 910 -> 1100 (column 5 -> 6)
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=99053; -- Escaping Ban'ethil: 0 -> 840 (column 0 -> 5)


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_23_world_forever_sleeper_has_awakened.sql
-- ====================================================================
-- The Sleeper Has Awakened (5321, Darkshore): the live quest has no objectives in the log; it completes when Kerlonian
-- Evershade reaches Liladris Moonriver (the escort script calls the quest event). Ours carried a "Horn of Awakening (13536)
-- 0/1" item objective from the vanilla conversion. The horn still comes from Kerlonian's Chest next to him and is still
-- what wakes him; it is just not a quest item to hand in. Questie project's Forever corrections (Questie team,
-- Questie/QuestieDB, master 3a621efa, 2026-11-08, GPLv3): objectives empty, escort south from his bag. Safe to run again.
DELETE FROM `quest_objectives` WHERE `QuestID`=5321 AND `Type`=1 AND `ObjectID`=13536;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_31_world_forever_gatehouse_goods_prerequisite.sql
-- ====================================================================
-- Gatehouse Goods (97222, Elaine Trias, Goldshire): needs only Philmor's Favor (97220) first.
-- 2026_11_05_99 made it wait for all of 97220, Yelmak's Medley (97242) and Favorite Food (97249), read from a Wowhead series list.
-- A series list only shows quests that belong together, not what must come first. Questie's Forever data (QuestieDB 5fba5d8,
-- player traces and fixes) has 97220 as the only quest before it, and a Russian player reported (2026-11-07) that the
-- quest never showed up after the chain. Safe to run again.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=19 AND `SourceEntry`=97222 AND `ConditionTypeOrReference`=8 AND `ConditionValue1` IN (97242,97249);
UPDATE `conditions` SET `Comment`='Gatehouse Goods - after Philmor''s Favor (97220, Questie Forever)'
 WHERE `SourceTypeOrReferenceId`=19 AND `SourceEntry`=97222 AND `ConditionTypeOrReference`=8 AND `ConditionValue1`=97220;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_50_world_forever_guard_roberts_healable.sql
-- ====================================================================
-- Garments of the Light (5624, human priest): Lesser Heal and Power Word: Fortitude on Guard Roberts (12423) landed on the priest
-- instead (by_hoy_, 2026-11-08). His template carried unit_flags 768 (immune to players and to NPCs), and a player cannot cast a
-- helpful spell on a unit that is immune to players, so the client cast it on the priest. The other Garments NPCs (Mountaineer
-- Dolf 12427, Deathguard Kel 12428, Sentinel Shaya 12429) have 0 there; Roberts now matches them. Spawn overrides cleared too.
-- Check in game: target Guard Roberts and cast Lesser Heal (Rank 2): he gets healed, thanks you and the quest counts. Safe to run again.
UPDATE `creature_template` SET `unit_flags` = `unit_flags` & ~768 WHERE `entry` = 12423;
UPDATE `creature` SET `unit_flags` = `unit_flags` & ~768 WHERE `id` = 12423 AND `unit_flags` IS NOT NULL;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_50_world_forever_ritual_fire_turn_in.sql
-- ====================================================================
-- Ritual Fire (660739), Mulgore, ends quest 95805 Grace of An'she and Mu'sha. A player reported (2026-11-08) that the quest
-- shows Ready for turn-in with the Pouch of Smoldering Incense in the bags, but the fire at -3371.9 -1004.1 cannot be clicked.
-- Its gameobject_template_addon row (from the 2026-11-05 quest data restore) had faction 12 and flags 4.
-- Faction template 12 is Stormwind (faction 72, Alliance group, enemy group Horde, FactionTemplate.db2), so for the Horde
-- races this quest is for, the fire is a hostile object. Flags 4 is GO_FLAG_INTERACT_COND (clickable only when the server
-- sends the activate flag). Faction 0 makes it neutral to everyone, and clearing flag 4 lets any player click it; it only
-- offers the turn-in to players who have the quest done. Safe to run again.
UPDATE `gameobject_template_addon` SET `faction`=0, `flags`=`flags` & ~4 WHERE `entry`=660739;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_52_world_forever_brs_retail_portal.sql
-- ====================================================================
-- Blackrock Spire: remove the retail Instance Portal inside the instance (guid 400651, map 229, near 63, -325, 54).
-- In retail it splits Lower and Upper Blackrock Spire into two maps; here both are one map, so the starry wall led nowhere
-- and players walked through it (reported 2026-11-08). The portals outside the instances stay.
DELETE FROM `gameobject` WHERE `guid`=400651 AND `map`=229;
DELETE FROM `gameobject_addon` WHERE `guid`=400651;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_53_world_forever_moonglade_druid_flights.sql
-- ====================================================================
-- Moonglade druid flights (Discord, crilico, 2026-11-08): Silva Fil'naveth (11800) "I'd like to fly to Rut'theran Village."
-- did nothing for a night elf druid on Back to Darnassus. Bunthen Plainswind (11798, to Thunder Bluff) uses the same script.
--
-- Checked and correct: menu 4041/4042 option 0 and its Alliance/Horde druid conditions, AIName SmartAI, the gossip select
-- events, spells 27998/28001 in the 70862 client data (Send Taxi, taxi paths 315 Nighthaven -> Rut'theran and
-- 316 Nighthaven -> Thunder Bluff, 31 and 78 path nodes, no cooldown, no area or aura requirement).
--
-- The flight master herself cast the flight spell on the player, a normal (not triggered) cast: it can fail on her side
-- without any message (range of 5 yards from her, line of sight on the roost, a spell already in progress), and then
-- only the gossip window closes. Now the player casts the flight spell on their own character, triggered, like the scripted
-- flight of Greer Orehammer, so nothing on the flight master's side can stop it. Safe to run again.

UPDATE `smart_scripts` SET `action_type` = 134, `action_param2` = 2,  -- SMART_ACTION_INVOKER_CAST, SMARTCAST_TRIGGERED
    `target_type` = 7,                                                -- SMART_TARGET_ACTION_INVOKER (the player is the target)
    `comment` = 'Silva Fil''naveth - On Gossip Option 0 Selected - Invoker Cast ''Flight Path'' (to Rut''theran Village)'
WHERE `entryorguid` = 11800 AND `source_type` = 0 AND `id` = 0 AND `event_type` = 62 AND `action_param1` = 27998;

UPDATE `smart_scripts` SET `action_type` = 134, `action_param2` = 2,
    `target_type` = 7,
    `comment` = 'Bunthen Plainswind - On Gossip Option 0 Selected - Invoker Cast ''Flight Path'' (to Thunder Bluff)'
WHERE `entryorguid` = 11798 AND `source_type` = 0 AND `id` = 0 AND `event_type` = 62 AND `action_param1` = 28001;


-- ====================================================================
-- upstream sql/custom/world/2026_11_08_80_world_forever_quest_xp_traces.sql
-- ====================================================================
-- Classic 1.60 (WoW Forever): quest XP and quest level from live player traces (Questie team, Questie/QuestieDB
-- support/Forever/QuestXP/xpDB-traces.lua, 2026-11-08, 724 quests). Every trace value is one QuestXP.db2 column at its level,
-- so each quest gets that column with multiplier 1 and the traced level. Quests that already give the traced XP are not
-- listed. Left out on purpose: traced XP that equals our XP cut by the over-level penalty (10-80%), and traces in the
-- lowest column for quests whose current values were not checked, because those can come from players far above the quest.
-- Rows that do not exist are not created. Safe to run again.
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=65610; -- Wish You Were Here: 2200 at level 27 (was 1550 at level 20)
UPDATE `quest_template_classic_level` SET `QuestLevel`=27 WHERE `ID`=65610;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 27 FROM `quest_template` WHERE `ID`=65610;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=78088; -- A Strange Artifact: 2450 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=78088;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=78088;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=78089; -- Advice From Stormwind: 1850 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=78089;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=78089;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=78092; -- It Must Be Destroyed: 3050 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=78092;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=78092;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=78261; -- The Horn of Xelthos: 1850 at level 23 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=23 WHERE `ID`=78261;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 23 FROM `quest_template` WHERE `ID`=78261;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=78676; -- The Eye of Bhossca: 3450 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=78676;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=78676;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=78823; -- Terror of the Desert Skies: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=78823;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=78823;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=78830; -- quest: 3900 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=78830;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=78830;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=79229; -- Highway Robbery: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79229;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79229;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=79235; -- quest: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79235;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79235;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=79236; -- quest: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79236;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79236;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=79242; -- quest: 3900 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79242;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79242;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=79363; -- Silvia's Sword: 2200 at level 27 (was 2100 at level 26)
UPDATE `quest_template_classic_level` SET `QuestLevel`=27 WHERE `ID`=79363;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 27 FROM `quest_template` WHERE `ID`=79363;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=79624; -- Anyone Can Cook: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79624;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79624;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=79677; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=79677;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=79677;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=86585; -- Banner of the Fallen: 1250 at level 17 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=17 WHERE `ID`=86585;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 17 FROM `quest_template` WHERE `ID`=86585;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=86667; -- Snowbound: 420 at level 10 (was 1550 at level 20)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=86667;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=86667;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=86757; -- Slowing the Orc Advance: 2450 at level 30 (was 2550 at level 32)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=86757;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=86757;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=87491; -- Spark of Freedom: 2450 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=87491;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=87491;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=87760; -- Holy Diver: 1550 at level 20 (was 0 at level 20)
UPDATE `quest_template_classic_level` SET `QuestLevel`=20 WHERE `ID`=87760;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 20 FROM `quest_template` WHERE `ID`=87760;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=88756; -- Bring Back a Bang: 2500 at level 31 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=31 WHERE `ID`=88756;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 31 FROM `quest_template` WHERE `ID`=88756;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=88757; -- Accessories of the Lost: 2650 at level 33 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=33 WHERE `ID`=88757;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 33 FROM `quest_template` WHERE `ID`=88757;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=91723; -- Delicate Instruments: 530 at level 8 (was 630 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=91723;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=91723;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=91732; -- Good Steel: 315 at level 7 (was 630 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=7 WHERE `ID`=91732;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 7 FROM `quest_template` WHERE `ID`=91732;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91740; -- Croaky's Head: 880 at level 11 (was 840 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=11 WHERE `ID`=91740;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 11 FROM `quest_template` WHERE `ID`=91740;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91746; -- Elmpaw's Head: 910 at level 12 (was 840 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=91746;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=91746;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91751; -- Rough Wolf Pelts: 700 at level 8 (was 475 at level 7)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=91751;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=91751;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=91772; -- Shhh! We're Hunting Kobolds: 355 at level 4 (was 270 at level 4)
UPDATE `quest_template_classic_level` SET `QuestLevel`=4 WHERE `ID`=91772;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 4 FROM `quest_template` WHERE `ID`=91772;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92743; -- A Conqueror's Crown: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=92743;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=92743;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=92747; -- Moonbrook Espionage: 800 at level 15 (was 580 at level 16)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=92747;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=92747;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=92750; -- Detonation at a Distance: 115 at level 16 (was 580 at level 16)
UPDATE `quest_template_classic_level` SET `QuestLevel`=16 WHERE `ID`=92750;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 16 FROM `quest_template` WHERE `ID`=92750;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=92751; -- Detonation at a Distance: 115 at level 16 (was 580 at level 16)
UPDATE `quest_template_classic_level` SET `QuestLevel`=16 WHERE `ID`=92751;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 16 FROM `quest_template` WHERE `ID`=92751;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=92755; -- Mysterious Mysticism: 2050 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=92755;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=92755;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=92814; -- I Need a Tailor: 2050 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=92814;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=92814;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=92817; -- Leather Technique: 2050 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=92817;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=92817;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92867; -- Ground to Keep: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=92867;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=92867;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92872; -- Dutiful Steps: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=92872;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=92872;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92911; -- Harvesting the Harvesters: 1050 at level 15 (was 800 at level 15)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=92911;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=92911;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=92914; -- quest: 3600 at level 43 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=43 WHERE `ID`=92914;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 43 FROM `quest_template` WHERE `ID`=92914;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93176; -- My Little Friends: 2750 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=93176;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=93176;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93429; -- Blood of My Blood: 2750 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=93429;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=93429;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=93430; -- Taking the Bait Back: 2050 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=93430;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=93430;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=93464; -- The Visiting Mage: 1450 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=93464;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=93464;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93644; -- quest: 3900 at level 45 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=45 WHERE `ID`=93644;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 45 FROM `quest_template` WHERE `ID`=93644;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93786; -- quest: 2800 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=93786;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=93786;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93787; -- quest: 2800 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=93787;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=93787;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=93788; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=93788;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=93788;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=94481; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94481;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94481;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94483; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=94483;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=94483;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94614; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94614;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94614;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=94615; -- quest: 1450 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94615;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94615;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94617; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94617;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94617;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=94618; -- quest: 1450 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94618;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94618;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94678; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94678;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94678;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94679; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94679;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94679;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94734; -- quest: 3300 at level 41 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=41 WHERE `ID`=94734;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 41 FROM `quest_template` WHERE `ID`=94734;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94745; -- quest: 3750 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=94745;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=94745;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94748; -- quest: 3750 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=94748;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=94748;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94775; -- Divine Grace: 210 at level 10 (was 85 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=94775;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=94775;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94776; -- Divine Grace: 210 at level 10 (was 85 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=94776;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=94776;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94777; -- Divine Grace: 210 at level 10 (was 85 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=94777;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=94777;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94820; -- Contingency Plan: 390 at level 20 (was 155 at level 20)
UPDATE `quest_template_classic_level` SET `QuestLevel`=20 WHERE `ID`=94820;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 20 FROM `quest_template` WHERE `ID`=94820;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94823; -- Confounding Flash: 210 at level 10 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=94823;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=94823;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94842; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94842;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94842;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=94843; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94843;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94843;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94861; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=94861;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=94861;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94877; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94877;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94877;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94879; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94879;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94879;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94881; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94881;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94881;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94885; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94885;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94885;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=94887; -- quest: 2100 at level 47 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=47 WHERE `ID`=94887;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 47 FROM `quest_template` WHERE `ID`=94887;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94907; -- quest: 3750 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=94907;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=94907;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=94923; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94923;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94923;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94924; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=94924;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=94924;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94925; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=94925;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=94925;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94931; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94931;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94931;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=94961; -- quest: 2350 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94961;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94961;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94963; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=94963;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=94963;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=94965; -- quest: 4650 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=94965;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=94965;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=94992; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=94992;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=94992;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=94999; -- quest: 700 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=94999;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=94999;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=95063; -- quest: 4850 at level 45 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=45 WHERE `ID`=95063;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 45 FROM `quest_template` WHERE `ID`=95063;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95064; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=95064;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=95064;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95065; -- Fishin' Time: 680 at level 12 (was 910 at level 12)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=95065;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=95065;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95119; -- quest: 3600 at level 43 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=43 WHERE `ID`=95119;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 43 FROM `quest_template` WHERE `ID`=95119;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95122; -- quest: 3750 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=95122;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=95122;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95123; -- quest: 3750 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=95123;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=95123;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95156; -- quest: 2100 at level 37 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=37 WHERE `ID`=95156;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 37 FROM `quest_template` WHERE `ID`=95156;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95162; -- quest: 3300 at level 41 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=41 WHERE `ID`=95162;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 41 FROM `quest_template` WHERE `ID`=95162;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95184; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95184;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95184;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95254; -- quest: 3450 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95254;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95254;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95288; -- quest: 3600 at level 43 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=43 WHERE `ID`=95288;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 43 FROM `quest_template` WHERE `ID`=95288;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95293; -- quest: 1700 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95293;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95293;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95328; -- Whispering Horror Residue: 910 at level 13 (was 840 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=13 WHERE `ID`=95328;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 13 FROM `quest_template` WHERE `ID`=95328;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95384; -- Partial Mystical Notes: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=95384;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=95384;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95426; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=95426;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=95426;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=95495; -- The Hermit Tanner: 135 at level 18 (was 340 at level 18)
UPDATE `quest_template_classic_level` SET `QuestLevel`=18 WHERE `ID`=95495;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 18 FROM `quest_template` WHERE `ID`=95495;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95507; -- Vrang's Game: 1000 at level 18 (was 1350 at level 18)
UPDATE `quest_template_classic_level` SET `QuestLevel`=18 WHERE `ID`=95507;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 18 FROM `quest_template` WHERE `ID`=95507;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95508; -- Unwelcome Guests: 1350 at level 18 (was 1000 at level 18)
UPDATE `quest_template_classic_level` SET `QuestLevel`=18 WHERE `ID`=95508;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 18 FROM `quest_template` WHERE `ID`=95508;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95534; -- Strahnbrad Mystery: 2800 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=95534;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=95534;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95561; -- quest: 1450 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95561;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95561;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95603; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95603;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95603;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95613; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95613;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95613;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95615; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95615;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95615;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95737; -- Seeking Caitlin: 1250 at level 31 (was 630 at level 31)
UPDATE `quest_template_classic_level` SET `QuestLevel`=31 WHERE `ID`=95737;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 31 FROM `quest_template` WHERE `ID`=95737;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95748; -- The Gurubashi Legends of Old: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=95748;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=95748;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95770; -- Locating the Lost City: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=95770;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=95770;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95773; -- Enemies in the Water: 2850 at level 37 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=37 WHERE `ID`=95773;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 37 FROM `quest_template` WHERE `ID`=95773;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95786; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=95786;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=95786;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95792; -- quest: 3450 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95792;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95792;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95805; -- Grace of An'she and Mu'sha: 355 at level 4 (was 180 at level 4)
UPDATE `quest_template_classic_level` SET `QuestLevel`=4 WHERE `ID`=95805;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 4 FROM `quest_template` WHERE `ID`=95805;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=95810; -- Lost Relic Carry: 1250 at level 31 (was 1229 at level 31)
UPDATE `quest_template_classic_level` SET `QuestLevel`=31 WHERE `ID`=95810;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 31 FROM `quest_template` WHERE `ID`=95810;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=95882; -- quest: 4450 at level 43 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=43 WHERE `ID`=95882;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 43 FROM `quest_template` WHERE `ID`=95882;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=95912; -- quest: 4300 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95912;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95912;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95913; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95913;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95913;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95934; -- quest: 3450 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95934;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95934;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=95943; -- quest: 2150 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95943;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95943;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=95951; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=95951;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=95951;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=95991; -- quest: 4300 at level 42 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=95991;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=95991;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96026; -- quest: 2450 at level 41 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=41 WHERE `ID`=96026;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 41 FROM `quest_template` WHERE `ID`=96026;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96031; -- Camping 101: Leatherworking: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96031;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96031;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96044; -- Camping 101: Blacksmithing: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96044;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96044;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96045; -- Camping 101: Alchemy: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96045;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96045;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96046; -- Camping 101: Mining: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96046;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96046;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96047; -- Camping 101: First Aid: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96047;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96047;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96050; -- Camping 101: Fishing: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96050;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96050;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96055; -- Camping 101: Herbalism: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96055;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96055;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96056; -- Camping 101: Skinning: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96056;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96056;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96058; -- Camping 101: Engineering: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96058;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96058;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96059; -- Camping 101: Enchanting: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96059;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96059;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96063; -- quest: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=96063;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=96063;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96067; -- quest: 2350 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=96067;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=96067;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96078; -- quest: 2350 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=96078;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=96078;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96081; -- quest: 1550 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=96081;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=96081;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=96103; -- quest: 710 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=96103;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=96103;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96104; -- quest: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=96104;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=96104;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96107; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=96107;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=96107;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96108; -- quest: 2350 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=96108;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=96108;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96130; -- Chakuyak: 335 at level 5 (was 350 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=5 WHERE `ID`=96130;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 5 FROM `quest_template` WHERE `ID`=96130;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96137; -- Ira's Dagger: 2000 at level 25 (was 1950 at level 24)
UPDATE `quest_template_classic_level` SET `QuestLevel`=25 WHERE `ID`=96137;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 25 FROM `quest_template` WHERE `ID`=96137;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96138; -- Merrick's Bow: 2450 at level 30 (was 2300 at level 28)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=96138;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=96138;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=96222; -- Khan Hratha: 4300 at level 42 (was 3450 at level 42)
UPDATE `quest_template_classic_level` SET `QuestLevel`=42 WHERE `ID`=96222;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 42 FROM `quest_template` WHERE `ID`=96222;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96241; -- Talkin' bout Toxins, Mon: 2350 at level 29 (was 2450 at level 30)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=96241;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=96241;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96243; -- Call of Earth: 180 at level 4 (was 420 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=4 WHERE `ID`=96243;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 4 FROM `quest_template` WHERE `ID`=96243;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96259; -- Defending the Dead: 2350 at level 29 (was 2450 at level 30)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=96259;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=96259;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96260; -- Stronger than Steel: 1750 at level 29 (was 2450 at level 30)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=96260;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=96260;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=96629; -- Camping 101: Cooking: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96629;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96629;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96781; -- What's the Big Idea?: 2800 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=96781;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=96781;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96783; -- A Way In: 2800 at level 36 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=36 WHERE `ID`=96783;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 36 FROM `quest_template` WHERE `ID`=96783;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96821; -- Legging It: 540 at level 6 (was 405 at level 6)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96821;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96821;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=96822; -- For Honor: 475 at level 7 (was 405 at level 6)
UPDATE `quest_template_classic_level` SET `QuestLevel`=7 WHERE `ID`=96822;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 7 FROM `quest_template` WHERE `ID`=96822;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96825; -- This Fruit Could Bite Back: 540 at level 6 (was 405 at level 6)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=96825;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=96825;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96874; -- This Is Spinal Axe: 840 at level 10 (was 630 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=96874;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=96874;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=96875; -- Beasts of Thunder Ridge: 840 at level 10 (was 630 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=96875;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=96875;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=96876; -- Ukor's Lost Pack: 880 at level 8 (was 530 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=96876;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=96876;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=96877; -- Halikor's Hoof: 1150 at level 12 (was 680 at level 12)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=96877;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=96877;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=96896; -- A Righteous Cause: 90 at level 12 (was 90 at level 13)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=96896;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=96896;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97047; -- The Cook in the Cage: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=97047;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=97047;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97048; -- Returning to Port: 2850 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=97048;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=97048;
UPDATE `quest_template` SET `RewardXPDifficulty`=2, `RewardXPMultiplier`=1 WHERE `ID`=97222; -- Gatehouse Goods: 270 at level 15 (was 800 at level 15)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=97222;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=97222;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97223; -- Bloodtalon Matriarch: 780 at level 9 (was 700 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=9 WHERE `ID`=97223;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 9 FROM `quest_template` WHERE `ID`=97223;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97237; -- Shelf Picked: 680 at level 18 (was 1000 at level 18)
UPDATE `quest_template_classic_level` SET `QuestLevel`=18 WHERE `ID`=97237;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 18 FROM `quest_template` WHERE `ID`=97237;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=97242; -- Yelmak's Medley: 105 at level 15 (was 800 at level 15)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=97242;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=97242;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=97246; -- Meal Appeal: 105 at level 15 (was 800 at level 15)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=97246;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=97246;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97253; -- Parts and Pieces: 1050 at level 15 (was 980 at level 14)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=97253;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=97253;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97282; -- Stormy Potential: 840 at level 10 (was 880 at level 11)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=97282;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=97282;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97326; -- Rocks to Rests: 540 at level 15 (was 800 at level 15)
UPDATE `quest_template_classic_level` SET `QuestLevel`=15 WHERE `ID`=97326;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 15 FROM `quest_template` WHERE `ID`=97326;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97328; -- Anything of Value: 2750 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=97328;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=97328;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97329; -- Where's the Key?: 2750 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=97329;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=97329;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97331; -- Zimmix's Great Escape: 2750 at level 35 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=35 WHERE `ID`=97331;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 35 FROM `quest_template` WHERE `ID`=97331;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97332; -- quest: 3300 at level 41 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=41 WHERE `ID`=97332;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 41 FROM `quest_template` WHERE `ID`=97332;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=97386; -- quest: 4650 at level 44 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=44 WHERE `ID`=97386;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 44 FROM `quest_template` WHERE `ID`=97386;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97453; -- quest: 1450 at level 38 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=38 WHERE `ID`=97453;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 38 FROM `quest_template` WHERE `ID`=97453;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=97884; -- quest: 3150 at level 40 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=40 WHERE `ID`=97884;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 40 FROM `quest_template` WHERE `ID`=97884;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97894; -- Business in Auberdine: 630 at level 17 (was 0 at level 17)
UPDATE `quest_template_classic_level` SET `QuestLevel`=17 WHERE `ID`=97894;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 17 FROM `quest_template` WHERE `ID`=97894;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97914; -- Expanding Horizons: 630 at level 17 (was 125 at level 17)
UPDATE `quest_template_classic_level` SET `QuestLevel`=17 WHERE `ID`=97914;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 17 FROM `quest_template` WHERE `ID`=97914;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97915; -- Camping 101: Alchemy: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97915;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97915;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97916; -- Camping 101: Blacksmithing: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97916;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97916;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97917; -- Camping 101: Enchanting: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97917;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97917;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97918; -- Camping 101: Engineering: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97918;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97918;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97919; -- Camping 101: First Aid: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97919;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97919;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97920; -- Camping 101: Fishing: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97920;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97920;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97921; -- Camping 101: Herbalism: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97921;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97921;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97922; -- Camping 101: Leatherworking: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97922;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97922;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97924; -- Camping 101: Skinning: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97924;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97924;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97926; -- Making Do: 630 at level 17 (was 125 at level 17)
UPDATE `quest_template_classic_level` SET `QuestLevel`=17 WHERE `ID`=97926;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 17 FROM `quest_template` WHERE `ID`=97926;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97930; -- Camping 101: Engineering: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97930;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97930;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97951; -- Camping 101: Alchemy: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97951;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97951;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97952; -- Camping 101: Blacksmithing: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97952;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97952;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97954; -- Camping 101: Engineering: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97954;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97954;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97958; -- Camping 101: Leatherworking: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97958;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97958;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=97959; -- Camping 101: Mining: 270 at level 6 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=97959;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=97959;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=98027; -- Gleaming Crescent Necklace: 1600 at level 26 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=26 WHERE `ID`=98027;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 26 FROM `quest_template` WHERE `ID`=98027;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98059; -- Hillsbrad's Hoard: 2200 at level 27 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=27 WHERE `ID`=98059;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 27 FROM `quest_template` WHERE `ID`=98059;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98069; -- Stolen Freewind Supplies: 2450 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=98069;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=98069;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98070; -- Stop the Screeching: 2450 at level 30 (was 2500 at level 31)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=98070;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=98070;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98071; -- Brewer's Trade: 2200 at level 27 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=27 WHERE `ID`=98071;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 27 FROM `quest_template` WHERE `ID`=98071;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98094; -- Scout Support: 1000 at level 25 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=25 WHERE `ID`=98094;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 25 FROM `quest_template` WHERE `ID`=98094;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98095; -- Valuable Vantages: 2000 at level 25 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=25 WHERE `ID`=98095;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 25 FROM `quest_template` WHERE `ID`=98095;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98155; -- Pristine Pesterhide Pelts: 2350 at level 29 (was 2500 at level 31)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=98155;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=98155;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98191; -- A Friend of the Family: 2000 at level 25 (was 200 at level 25)
UPDATE `quest_template_classic_level` SET `QuestLevel`=25 WHERE `ID`=98191;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 25 FROM `quest_template` WHERE `ID`=98191;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98192; -- quest: 3600 at level 43 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=43 WHERE `ID`=98192;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 43 FROM `quest_template` WHERE `ID`=98192;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=98208; -- Bloom of the Heavens: 1750 at level 29 (was 2350 at level 29)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=98208;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=98208;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98223; -- Old Habits: 1250 at level 31 (was 250 at level 31)
UPDATE `quest_template_classic_level` SET `QuestLevel`=31 WHERE `ID`=98223;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 31 FROM `quest_template` WHERE `ID`=98223;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=98230; -- This Land Was Their Land: 235 at level 29 (was 1200 at level 29)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=98230;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=98230;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98246; -- Trying Times: 2350 at level 29 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=29 WHERE `ID`=98246;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 29 FROM `quest_template` WHERE `ID`=98246;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98252; -- A Void Path: 920 at level 23 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=23 WHERE `ID`=98252;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 23 FROM `quest_template` WHERE `ID`=98252;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98319; -- Secure the Mountain: 350 at level 8 (was 175 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=98319;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=98319;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98403; -- Twisted Hatred: 910 at level 12 (was 1150 at level 12)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=98403;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=98403;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=98417; -- Vengeful Trail: 1700 at level 28 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=28 WHERE `ID`=98417;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 28 FROM `quest_template` WHERE `ID`=98417;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98418; -- Shared Fury: 2450 at level 30 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=30 WHERE `ID`=98418;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 30 FROM `quest_template` WHERE `ID`=98418;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=98517; -- Call of Fire: 420 at level 10 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=98517;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=98517;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=98545; -- Leonid's Letter: 90 at level 12 (was 90 at level 13)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=98545;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=98545;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=98903; -- The Battle for Darkspear Islands!: 3000 at level 39 (was not known here)
UPDATE `quest_template_classic_level` SET `QuestLevel`=39 WHERE `ID`=98903;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 39 FROM `quest_template` WHERE `ID`=98903;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=99048; -- A Missing Hand: 80 at level 9 (was 90 at level 11)
UPDATE `quest_template_classic_level` SET `QuestLevel`=9 WHERE `ID`=99048;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 9 FROM `quest_template` WHERE `ID`=99048;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=99049; -- Threat from Below: 390 at level 9 (was 880 at level 11)
UPDATE `quest_template_classic_level` SET `QuestLevel`=9 WHERE `ID`=99049;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 9 FROM `quest_template` WHERE `ID`=99049;
UPDATE `quest_template` SET `RewardXPDifficulty`=6, `RewardXPMultiplier`=1 WHERE `ID`=99052; -- Threat from Below: 1100 at level 11 (was 910 at level 12)
UPDATE `quest_template_classic_level` SET `QuestLevel`=11 WHERE `ID`=99052;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 11 FROM `quest_template` WHERE `ID`=99052;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=99053; -- Escaping Ban'ethil: 840 at level 10 (was 0 at level 9)
UPDATE `quest_template_classic_level` SET `QuestLevel`=10 WHERE `ID`=99053;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 10 FROM `quest_template` WHERE `ID`=99053;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=99080; -- Drive Them Out: 880 at level 11 (was 780 at level 9)
UPDATE `quest_template_classic_level` SET `QuestLevel`=11 WHERE `ID`=99080;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 11 FROM `quest_template` WHERE `ID`=99080;
UPDATE `quest_template` SET `RewardXPDifficulty`=1, `RewardXPMultiplier`=1 WHERE `ID`=99082; -- The High Chieftain: 90 at level 11 (was 80 at level 9)
UPDATE `quest_template_classic_level` SET `QuestLevel`=11 WHERE `ID`=99082;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 11 FROM `quest_template` WHERE `ID`=99082;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=99108; -- Sparring Match: 270 at level 6 (was 540 at level 6)
UPDATE `quest_template_classic_level` SET `QuestLevel`=6 WHERE `ID`=99108;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 6 FROM `quest_template` WHERE `ID`=99108;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=99127; -- A Net Disaster: 475 at level 7 (was 630 at level 7)
UPDATE `quest_template_classic_level` SET `QuestLevel`=7 WHERE `ID`=99127;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 7 FROM `quest_template` WHERE `ID`=99127;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=99143; -- Bottles and Baubles: 475 at level 7 (was 630 at level 7)
UPDATE `quest_template_classic_level` SET `QuestLevel`=7 WHERE `ID`=99143;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 7 FROM `quest_template` WHERE `ID`=99143;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=99152; -- As Above, So Below: 910 at level 12 (was 840 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=99152;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=99152;
UPDATE `quest_template` SET `RewardXPDifficulty`=5, `RewardXPMultiplier`=1 WHERE `ID`=99153; -- The One That Got Away: 910 at level 12 (was 840 at level 10)
UPDATE `quest_template_classic_level` SET `QuestLevel`=12 WHERE `ID`=99153;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 12 FROM `quest_template` WHERE `ID`=99153;
UPDATE `quest_template` SET `RewardXPDifficulty`=3, `RewardXPMultiplier`=1 WHERE `ID`=99159; -- Finding Warmth: 350 at level 8 (was 700 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=99159;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=99159;
UPDATE `quest_template` SET `RewardXPDifficulty`=4, `RewardXPMultiplier`=1 WHERE `ID`=99160; -- Rime's Wrath: 530 at level 8 (was 700 at level 8)
UPDATE `quest_template_classic_level` SET `QuestLevel`=8 WHERE `ID`=99160;
INSERT IGNORE INTO `quest_template_classic_level` (`ID`,`LevelType`,`QuestLevel`) SELECT `ID`, 2, 8 FROM `quest_template` WHERE `ID`=99160;


-- ====================================================================
-- upstream sql/custom/world/2026_11_09_10_world_forever_tameable_beasts_from_cache.sql
-- ====================================================================
-- Beasts players cannot tame or that Beast Lore shows without a pet family (Discord 11-09: elite bear Matriarch Bristlefur
-- 276189 in Elwynn, tameable on the official realm; 11-08 Beast Lore "not tameable"). Taming needs a beast with a family and
-- the tameable type flag (CreatureTemplate::IsTameable). Our 276189 has family 0 and TypeFlags 0; Blizzard's answer has family
-- 4 (bear) and the tameable flag.
-- Source: the creature records in the players' client caches of 11-08 (build 70862 and 70790, folders cache-1108 and cache-1108b),
-- every beast there with a pet family (250 creatures). Only gaps are filled: a family is set only where ours is 0, and only the
-- tameable and exotic bits are added to TypeFlags; nothing is taken away. A cache answer from our own realm is the same as ours,
-- so it changes nothing. Safe to run more than once.
DROP TEMPORARY TABLE IF EXISTS `tmp_cache_beasts`;
CREATE TEMPORARY TABLE `tmp_cache_beasts` (`entry` INT UNSIGNED NOT NULL PRIMARY KEY, `family` INT NOT NULL, `bits` INT UNSIGNED NOT NULL);
INSERT INTO `tmp_cache_beasts` (`entry`,`family`,`bits`) VALUES
(30,3,1),
(69,1,1),
(113,5,1),
(118,1,1),
(119,5,1),
(154,7,1),
(157,5,1),
(199,7,1),
(213,1,1),
(217,3,1),
(299,1,1),
(330,5,1),
(345,5,1),
(428,7,1),
(442,3,1),
(454,5,1),
(471,3,1),
(505,3,1),
(521,1,1),
(524,5,1),
(525,1,1),
(539,3,1),
(547,5,1),
(565,1,1),
(569,3,1),
(574,3,1),
(616,3,1),
(681,2,1),
(682,2,1),
(683,2,1),
(684,2,1),
(685,11,1),
(686,11,1),
(687,11,1),
(698,2,1),
(729,2,1),
(730,11,1),
(736,2,1),
(772,2,1),
(822,4,1),
(833,1,1),
(834,1,1),
(854,11,1),
(856,11,1),
(923,1,1),
(930,3,1),
(1015,11,1),
(1016,11,1),
(1017,11,1),
(1018,11,1),
(1020,11,1),
(1021,11,1),
(1022,11,1),
(1023,11,1),
(1085,2,1),
(1108,9,1),
(1109,7,1),
(1111,3,1),
(1114,9,1),
(1125,5,1),
(1126,5,1),
(1127,5,1),
(1128,4,1),
(1129,4,1),
(1131,1,1),
(1132,1,1),
(1133,1,1),
(1138,1,1),
(1150,6,1),
(1151,6,1),
(1152,6,1),
(1194,7,1),
(1196,4,1),
(1199,2,1),
(1201,2,1),
(1353,11,1),
(1400,6,1),
(1417,6,1),
(1689,5,1),
(1922,1,1),
(2069,2,1),
(2070,2,1),
(2071,2,1),
(2089,6,1),
(2163,4,1),
(2165,4,1),
(2172,12,1),
(2175,2,1),
(2231,8,1),
(2232,8,1),
(2233,8,1),
(2234,8,1),
(2235,8,1),
(2236,8,1),
(2237,2,1),
(2321,12,1),
(2322,12,1),
(2323,12,1),
(2579,7,1),
(2956,12,1),
(2957,12,1),
(2958,1,1),
(2959,1,1),
(2960,1,1),
(2969,7,1),
(2970,7,1),
(2971,7,1),
(3035,2,1),
(3068,12,1),
(3244,12,1),
(3245,12,1),
(3246,12,1),
(3247,27,1),
(3249,27,1),
(3255,11,1),
(3256,11,1),
(3425,2,1),
(3461,21,1),
(3566,2,1),
(3581,6,1),
(3630,27,1),
(3631,27,1),
(3632,11,1),
(3633,11,1),
(3634,11,1),
(3636,11,1),
(3812,8,1),
(3814,8,1),
(3819,3,1),
(3823,1,1),
(4005,3,1),
(4007,3,1),
(4011,27,1),
(4040,3,1),
(4067,2,1),
(4117,27,1),
(4118,27,1),
(4119,27,1),
(4124,2,1),
(4127,25,1),
(4128,25,1),
(4129,25,1),
(4139,20,1),
(4140,20,1),
(4142,21,1),
(4143,21,1),
(4144,21,1),
(4154,7,1),
(4158,7,1),
(4248,25,1),
(4249,25,1),
(4250,25,1),
(4263,3,1),
(4341,6,1),
(4342,6,1),
(4343,6,1),
(4344,6,1),
(4351,11,1),
(4352,11,1),
(4355,11,1),
(4380,3,1),
(4396,21,1),
(4397,21,1),
(4398,21,1),
(4399,21,1),
(4400,21,1),
(4411,3,1),
(4412,3,1),
(4413,3,1),
(4414,3,1),
(4415,3,1),
(4548,25,1),
(4662,25,1),
(4688,25,1),
(4689,25,1),
(4692,7,1),
(4693,7,1),
(4695,7,1),
(4696,20,1),
(4697,20,1),
(4841,6,1),
(4950,1,1),
(5056,27,1),
(5260,9,1),
(5262,9,1),
(5268,4,1),
(5272,4,1),
(5274,4,1),
(5286,1,1),
(5287,1,1),
(5288,1,1),
(5307,27,1),
(5308,27,1),
(5349,27,1),
(5352,4,1),
(5356,1,1),
(5422,20,1),
(5425,25,1),
(5428,7,1),
(5807,2,1),
(5856,3,1),
(5858,3,1),
(5865,2,1),
(5937,20,1),
(6788,4,1),
(8207,7,1),
(8208,25,1),
(9694,1,1),
(9695,20,1),
(9697,1,1),
(9698,20,1),
(10644,1,1),
(10992,2,0),
(14228,25,1),
(14232,11,1),
(14233,6,1),
(14234,27,1),
(165189,1,1),
(211967,3,0),
(253529,5,1),
(255851,5,1),
(255901,8,1),
(257017,1,1),
(258331,5,1),
(258522,1,1),
(259046,6,1),
(259267,1,1),
(259269,7,1),
(260157,2,1),
(260797,6,1),
(260803,3,1),
(260804,3,1),
(260809,21,1),
(262037,24,1),
(263343,7,1),
(263348,12,1),
(263380,6,1),
(264081,3,1),
(264083,3,1),
(268558,1,1),
(269057,27,1),
(269254,1,1),
(270664,3,1),
(271334,4,1),
(271898,3,1),
(271988,3,1),
(272384,6,1),
(272401,27,1),
(276189,4,1),
(277645,319,1);

UPDATE `creature_template` ct JOIN `tmp_cache_beasts` t ON t.`entry` = ct.`entry`
   SET ct.`family` = t.`family`
 WHERE ct.`type` = 1 AND ct.`family` = 0;

UPDATE `creature_template_difficulty` ctd JOIN `tmp_cache_beasts` t ON t.`entry` = ctd.`Entry`
  JOIN `creature_template` ct ON ct.`entry` = ctd.`Entry`
   SET ctd.`TypeFlags` = ctd.`TypeFlags` | t.`bits`
 WHERE ctd.`DifficultyID` = 0 AND ct.`type` = 1 AND t.`bits` <> 0 AND (ctd.`TypeFlags` & t.`bits`) <> t.`bits`;

DROP TEMPORARY TABLE IF EXISTS `tmp_cache_beasts`;


-- ====================================================================
-- upstream sql/custom/world/2026_11_09_14_world_forever_questie_1108b_fixes.sql
-- ====================================================================
-- Fixes from the Questie project's hand-written Forever corrections (Questie team, Questie/QuestieDB,
-- src/corrections/Forever, master c1b50fef, 2026-11-08, GPLv3), taken from what players saw in the live game.
-- Already matching here: Stepping Stones (79192) after either note quest, the 79980 -> 79974 -> 79975 -> 79976 chain,
-- Favorite Food (97249) after Meal Appeal (97246), Knowledge in the Deeps (971) from level 18, and the new object spawns.
-- Safe to run again.

-- 1) "... and that note you found" (79007 Barrens, 79008 Westfall) sends you to the other burned-out camp: 79007 starts at the
--    Barrens Burned-Out Remains (415106) and is handed in at the Westfall one (415107), 79008 the other way round. Stepping
--    Stones (79192) starts only at the Nailed Planks (417072 Barrens, 424010 Westfall), which hand in nothing. Ours (from the
--    AnoCore port) handed each note in at its own camp or its plank and also started 79192 at the remains.
-- Aeternum: part 1 (gameobject starters/enders 415106/415107/417072/424010 for 79007/79008/79192) left out:
-- our world has neither the Westfall camp objects nor 79008/79192, and it would remove our working 415106 -> 79007 turn-in.
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=2519 WHERE `ID`=98391;



-- ====================================================================
-- upstream sql/custom/world/2026_11_09_15_world_forever_searing_totem_ranks_1_2_attack.sql
-- ====================================================================
-- Searing Totem rank 1 does not attack (Discord, Zoryn 2026-11-09). Rank 2 has the same problem.
--
-- Cause: totems that attack are run by the core TotemAI, which casts the totem's spell (creature_template_spell, see
-- 2026_11_06_40_world_forever_shaman_totem_spells.sql) at the nearest enemy in range. But Searing Totem 2523 (rank 1) and
-- 3902 (rank 2) still had AIName 'SmartAI' from the world data, and an AIName wins over TotemAI:
--   2523: "Out of Combat - Cast 3606 on Self" (the totem tried to burn itself, which fails, and never picked an enemy)
--   3902: "In Combat - Cast 6350 on Victim" (a totem never enters combat, so this never ran)
-- Ranks 3 to 6 (3903, 3904, 7400, 7402) have no AIName and work. Checked in the dev DB dump of 2026-11-04.
-- Fix: give both back to TotemAI, like the other ranks. Their spells (3606, 6350) are already set per rank. Safe to run again.
UPDATE `creature_template` SET `AIName`='' WHERE `entry` IN (2523,3902) AND `AIName`='SmartAI';
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND `entryorguid` IN (2523,3902);


-- ====================================================================
-- upstream sql/custom/world/2026_11_09_16_world_forever_stoneclaw_totem_rank_1.sql
-- ====================================================================
-- Stoneclaw Totem rank 1 (3579) still had a SmartAI script from the world data: "Out of Combat - Cast 5729 on Self".
-- An AIName wins over the core TotemAI, so rank 1 never cast its own spell 5728 (set in
-- 2026_11_06_40_world_forever_shaman_totem_spells.sql) and never drew the enemies to it. Ranks 2 and up (3911, 3912, 3913,
-- 7398, 7399, 15478) have no AIName and work. Same fix as Searing Totem ranks 1 and 2: give it back to TotemAI.
-- Safe to run again.
UPDATE `creature_template` SET `AIName`='' WHERE `entry`=3579 AND `AIName`='SmartAI';
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND `entryorguid`=3579;


-- ====================================================================
-- Aeternum guard: the chains above were built against the upstream world. Our world lacks some of the quests they name,
-- so drop what would point at a quest that does not exist here (TrinityCore would only log these rows as errors).
-- ====================================================================
UPDATE `quest_template_addon` a LEFT JOIN `quest_template` q ON q.`ID`=ABS(a.`PrevQuestID`)
  SET a.`PrevQuestID`=0 WHERE a.`PrevQuestID`<>0 AND q.`ID` IS NULL;
DELETE a FROM `quest_template_addon` a LEFT JOIN `quest_template` q ON q.`ID`=a.`ID` WHERE q.`ID` IS NULL;
DELETE c FROM `conditions` c LEFT JOIN `quest_template` q ON q.`ID`=c.`SourceEntry`
  WHERE c.`SourceTypeOrReferenceId` IN (19,20) AND q.`ID` IS NULL;
DELETE c FROM `conditions` c LEFT JOIN `quest_template` q ON q.`ID`=c.`ConditionValue1`
  WHERE c.`SourceTypeOrReferenceId` IN (19,20) AND c.`ConditionTypeOrReference` IN (8,9,14,28,43,47) AND q.`ID` IS NULL;
