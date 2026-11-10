-- WoW Forever (1.60) quest-source gaps, 2026-11-11.
-- Three quests could not be completed because their quest objects had no loot (or no template),
-- plus completion/progress texts and objectives that the world database lacked.
-- Sources (public, credited per GPL-3.0 / CC BY 4.0 terms):
--  * EG Link by Efficient Games: Data from Efficient Games (https://efficient.games), licensed under CC BY 4.0
--    (https://creativecommons.org/licenses/by/4.0/). Changed: selected per row and written as TrinityCore rows.
--    Used for: quest completion/progress texts, objectives, quest-object templates, their loot and quest items, sightings.
--  * forever-db / Polestar (https://github.com/fitzforge/polestar), GPL-3.0: spawn points (QuestieDB points),
--    objectives cross-check.
--  * QuestieDB (https://github.com/Questie/QuestieDB), GPL-3.0: spawn points via forever-db.
--  Rows were selected and rewritten by hand from the public sources above (snapshot 2026-11-08).
--  Game content such as names and text belongs to Blizzard Entertainment.
-- VerifiedBuild = the client build the source row was recorded at (70245/70334/70338 = Forever beta caches;
-- 70790/70840 = live caches). Rows with no recorded build keep 0.

-- ---------------------------------------------------------------------------------------------
-- P1a: 95507 "Vrang's Game" needs 8x item 269719 from "Sprung Trap" (640104).
-- The template and its 16 spawns exist, but Data1 (loot id) was 0 so nothing could be looted.
-- Loot id 139990 / quest item: EG Link (object data 70840, opening seen at 70338 beta). Existing spawns already
-- cover every recorded sighting (all within 1 yd), so no spawn is added.
UPDATE `gameobject_template` SET `Data1`=139990 WHERE `entry`=640104 AND `Data1` IN (0,139990);
DELETE FROM `gameobject_loot_template` WHERE `Entry`=139990;
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(139990,0,269719,100,1,1,0,1,1,'Sprung Trap (640104) - quest 95507');
DELETE FROM `gameobject_questitem` WHERE `GameObjectEntry`=640104;
INSERT INTO `gameobject_questitem` (`GameObjectEntry`,`Idx`,`ItemId`,`VerifiedBuild`) VALUES (640104,0,269719,70840);

-- P1b: 97253 "Parts and Pieces" needs 5x item 277331 from "Pile of Complicated Parts" (660930).
-- Same problem: 7 spawns, Data1=0. Loot id 142176 / quest item: EG Link 70790 (openings also seen on beta).
-- Existing spawns already cover every recorded sighting, so no spawn is added.
UPDATE `gameobject_template` SET `Data1`=142176 WHERE `entry`=660930 AND `Data1` IN (0,142176);
DELETE FROM `gameobject_loot_template` WHERE `Entry`=142176;
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(142176,0,277331,100,1,1,0,1,1,'Pile of Complicated Parts (660930) - quest 97253');
DELETE FROM `gameobject_questitem` WHERE `GameObjectEntry`=660930;
INSERT INTO `gameobject_questitem` (`GameObjectEntry`,`Idx`,`ItemId`,`VerifiedBuild`) VALUES (660930,0,277331,70790);

-- P1c: 96822 "For Honor" needs 275713, 275716 and 275717. The objects "Raider's Bow" (656180) and
-- "Raider's Battleaxe" (656181) were missing. Templates, loot and quest items: EG Link 70790/70840.
-- One spawn each at the forever-db/QuestieDB point confirmed by an EG Link live sighting (within 40 yd).
-- GUIDs 34020101-34020102: outside 34020001-34020100, which 2026_11_10_00_world_forever_player_reports(+rollback) clear.
-- PARTIAL: item 275717 (third objective) has no known source in any public data used here; no drop is invented,
-- so 96822 stays incompletable until a real source for 275717 is found.
DELETE FROM `gameobject_template` WHERE `entry` IN (656180,656181);
INSERT INTO `gameobject_template` (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,`Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,`Data8`,`Data9`,`Data10`,`Data11`,`Data12`,`Data13`,`Data14`,`Data15`,`Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,`Data24`,`Data25`,`Data26`,`Data27`,`Data28`,`Data29`,`Data30`,`Data31`,`Data32`,`Data33`,`Data34`,`ContentTuningId`,`RequiredLevel`,`AIName`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(656180,3,126298,'Raider''s Bow','','','',1,43,141537,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4097,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'','',NULL,70840),
(656181,3,126299,'Raider''s Battleaxe','','','',1.5,43,141538,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4097,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'','',NULL,70840);
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (141537,141538);
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(141537,0,275713,100,1,1,0,1,1,'Raider''s Bow (656180) - quest 96822'),
(141538,0,275716,100,1,1,0,1,1,'Raider''s Battleaxe (656181) - quest 96822');
DELETE FROM `gameobject_questitem` WHERE `GameObjectEntry` IN (656180,656181);
INSERT INTO `gameobject_questitem` (`GameObjectEntry`,`Idx`,`ItemId`,`VerifiedBuild`) VALUES
(656180,0,275713,70840),
(656181,0,275716,70840);
DELETE FROM `gameobject` WHERE `guid` IN (34020101,34020102);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(34020101,656180,1,0,0,'0',0,0,0,-1,-180.0,-5102.5,21.08,0,0,0,0,1,120,255,1,'',NULL,0),
(34020102,656181,1,0,0,'0',0,0,0,-1,-181.0,-4996.0,21.1,0,0,0,0,1,120,255,1,'',NULL,0);

-- ---------------------------------------------------------------------------------------------
-- P2a: quest_offer_reward - completion texts for 77 quests that had none (EG Link, text verbatim).
-- Only fills gaps: an existing row is never replaced.
INSERT IGNORE INTO `quest_offer_reward` (`ID`,`RewardText`,`VerifiedBuild`) VALUES
(79007,'As you stumble upon the remnants of yet another charred corpse, your gaze falls upon a message secured to a wooden plank.',70840), -- ... and that note you found
(79361,'I can feel their corruption from here. The twisted elementals the quilboar are summoning should be drawn to the cleansing ritual.',70338), -- Storm, Earth, and Fire (beta cache)
(86576,'Ah, you have brought sunstones that the harpies collect! The harpies love to gather objects that are shiny, but they do not know the true value of the stones. By capturing the dazzling light of the sun, these stones reflect its majesty and allow us to adorn our shrines and homes with tiny motes of light in its honor. The moon, too, illuminates them with its silver beauty at night.\r\n\r\nThese stones remind us of the connection we have to the lights of the skies.',70840), -- Bloodfury Trinkets
(91920,'The potion didn''t work? Of course it didn''t work. You brought me insufficient hearts. The recipe clearly calls for six. You brought three.\r\n\r\nHowever, I confess I hadn''t considered applications for murloc parts until now...\r\n\r\n<Renferrel takes the eyes.>\r\n\r\nI can afford to sacrifice a few of my own hearts to get your Deathstalker''s strength back.',70790), -- Wild Eyes
(91921,'<Quinn takes the potion and drinks it.>\n\nYes, I can feel it working. I will soon be able to travel.\n\nYou have done me a great favor, $N.',70338), -- Return to Quinn (Again) (beta cache)
(92484,'Can you hear it? The whispers of the winds? Doubtful, considering that the spirits stopped speaking with us when they left us those years ago. \r\n\r\nThose among us who would seek audience with the spirits of the four winds once more hone our understanding of the elements and the natural world, dearly hoping to reestablish the dialogue that was so abruptly ended. \r\n\r\nWould you walk this path and master the elements?\r\n\r\nSpeak with me again if you wish to take your next steps and I will train you.',70338), -- Embracing the Elements (beta cache)
(92596,'The High Order''s mission is vital to the survival of our people. We hold our elemental benefactors in the utmost regard, but we must face reality; they are gone now, and we are on our own.\r\n\r\nBetween threats from within such as the Al''Aketh, and uncertainties of our security on this plane, it is our destiny as Highborne descendants of our ancestral home of Eldre''Thalas to use our gifts to ensure that the shen''dorei can stand on our own once again.',70245), -- The High Order (beta cache)
(92701,'<Valennia''s expression darkens as she reads the letter.>\r\n\r\nAonda trusted you to deliver this and she speaks highly enough of you in this letter, so I''ll try to trust you too. Before we speak further, I need to consult with the few peacekeepers I have left here and formulate a plan in case the cult strikes here next. Enjoy your stay in Valanaar for now.\r\n\r\nIf you are looking to keep yourself busy, there are plenty of townsfolk that can likely use the help of a strong young adventurer like yourself.',70245), -- To Valanaar (beta cache)
(92706,'Finally, somebody killed that thing! Do you know how hard it is to get a crew in the water when everyone in town''s crying "Shark!" all the time?\r\n\r\nI might just put that monster''s fin into a soup for dinner tonight, just to have the last laugh.',70790), -- WANTED: Bruuz
(92709,'So do you now see? Rediscovering the ancient magic of our forebears is the key to unlocking the secrets of Zephras Isle and securing our future, but in order to save our home, we must rediscover what we''ve lost these past millennia.\r\n\r\nAzeroth is the key. The Kirin Tor will show us the way.\r\n\r\nWhile we make ready to leave, you should check in with the other members of the High Order here in Valanaar. Most of them have tasks that they could use help from one such as yourself to see completed.',70245), -- A Grand Adventure (beta cache)
(92849,'Th... thank you, stranger. I was returning from an assignment at the Overlook Standing Stones when I was suddenly ambushed and attacked by Windshapers. They forced me over to the mouth of the cave and left me there, barely conscious and bleeding. The last thing I remember before blacking out is a massive winged beast emerging from the cave and scooping me up.\r\n\r\nI think it''s safe to say that if you hadn''t come along, I''d have been dinner for that beast and its younglings.',70245), -- The Missing Scholar (beta cache)
(92850,'Thank you again, $N.',70245), -- The Missing Scholar (beta cache)
(92860,'I am sorry to hear about Belathaan. He was actually my cousin and he was a good mage and servant of Zephras. We will avenge him.\r\n\r\nYou seem eager to lend aid immediately. Come, let''s speak further on the current situation.',70245), -- In Service of Zephras (beta cache)
(93089,'It gladdens me to hear that we have High Elder Shadowsong''s blessing for our expedition. Now that the cult has been defeated and preparations have been made, I think it''s finally time.\r\n\r\nAnsirem has worked diligently to help us get stable skycutter transport going between the city of Dalaran and Zephras Isle. We''ve done as much testing as we can, and the time has come for our vanguard to depart.',70245), -- What Comes Next (beta cache)
(93461,'Checked in with Rathiril then? Great. Lots of tension between the High Order and the Windshapers these days, but they keep things civil here in Shen''dar. Well, at least in the village anyway. What happens out in the wilds stays in the wilds.\r\n\r\nNow that that''s done, you look like someone who might be looking for something to do with their hands. I''ve got a few things that need doing, if you are game for a bit of work?',70245), -- Welcome to Shen'dar Village (beta cache)
(93735,'<Riaani''s face is buried under the heaving armor plates of the construct, attempting to work on the mechanisms underneath. He does not notice your presence at all.>\r\n\r\nStorms curse--where is the core?! Who thought it would be a good idea to remove the Gyrozephyr as well? It''s a miracle this construct''s winds haven''t dissipated entirely by now!',70790), -- The Broken Construct
(93738,'Very interesting, so it was the High Order that was responsible for the construct breaking down?\r\n\r\nI know Belathaan. He''s Valennia Stormfist''s cousin and his mother was actually a Windshaper. It''s disappointing that he was involved in this.\r\n\r\nI need to reflect on this for a moment.',70790), -- The Broken Construct
(93739,'So you have returned. I hope that you learned much in your journey.\r\n\r\nIt is my understanding that your people are still establishing themselves in Mulgore. While you wait for them, I''d encourage you to explore the territory around Orgrimmar. The Barrens in particular is always in need of eager adventurers like yourself.',70790), -- Exploring the Horde
(93835,'I sent a few of our remaining mages to ensure that Valennia and her forces were able to escape as well. She should be resting at High Elder Shadowsong''s residence now.\r\n\r\nAnsirem had to return to Azeroth, but he and I discussed the next steps for the High Order before he departed. We survived the day, but Zephras is dangerously vulnerable now and we must make plans for what is to come.',70245), -- Confront Lorthuna (beta cache)
(94013,'Excellent, $N! You are quite a fast learner. Now that you know the basics, I can teach you how to forge a lasting bond with an animal companion.\r\n\r\nGo with this knowledge and thrive. Be the $c that Zephras Isle needs you to be.',70245), -- Taming the Beast (beta cache)
(94050,'A fledgling $c, I see. Yes, I can bestow you with the skills you need to train and guide your pet. Not only will you be able to teach your pet new abilities, you will now be able to feed your pet, as well as revive it, should it fall in battle.\n\nRemember, $N, always respect the balance and live in awe of nature around you. Now, go forth. We shall speak again, at a later date.',70245), -- Training the Beast (beta cache)
(94369,'<Talaanis Shadowsong listens intently as you tell him of what transpired above the Shrine of Akir, and of Elaadrin''s plan to leave Zephras Isle.>\r\n\r\nI need to reflect on this further, but looking at the upside, we have rid ourselves of Lorthuna and deprived the cult of their leader. Our home has not been invaded yet, and on the timeline of elemental demigods, it could be years... decades even, before Al''Akir deigns to turn his attention to us.\r\n\r\nThere is much to fear now, but much to celebrate as well.',70245), -- The Fate of Zephras (beta cache)
(94947,'Well met, $c. I am Bolvar Fordragon, regent to the throne of Stormwind.\r\n\r\nA Kirin Tor messenger told me to expect to receive members of the High Order, and here you stand. It''s been some time since a high elf has stood in this throne room, but I am glad to accept you now as a guest.\r\n\r\nOn behalf of the King, I welcome you to Stormwind and give you leave to travel freely through alliance lands. Welcome to Azeroth, Skyborne.',70245), -- Welcome to Azeroth (beta cache)
(94978,'Well tamed, $N! You are becoming adept already.',70245), -- Taming the Beast (beta cache)
(94979,'You''re back already! Impressive speed, $N!',70245), -- Taming the Beast (beta cache)
(95212,'These pelts will make excellent saddles. You''ve done the Alliance proud, $c!',70338), -- Never Saddle on Quality (beta cache)
(95216,'You killed it??\r\n\r\nHmmph. Lucky for you this should be enough ichor to reverse engineer its properties. You do not wish to be an enemy of the Royal Apothecary Society.\r\n\r\nGet out of my sight!',70790), -- The New Plague
(95494,'Yes, these look like they will do nicely. I have an arrangement with a local tanner to deliver these raw hides in exchange for various finished leather goods that I use as part of my trade. \r\n\r\nI''ve never come up short on my end of the arrangement, and even with my injury I do not intend to start now.',70338), -- Bruised Pride and Lion Hides (beta cache)
(95495,'Vrang sent you then? Very well. I appreciate you delivering these hides to me. It''s unfortunate that Vrang couldn''t make the journey himself. There is a matter I wish to speak with him about.',70338), -- The Hermit Tanner (beta cache)
(95507,'Thank you for your aid, $c. I was starting to wonder if I''d have to go to bed hungry tonight because this cursed wound on my leg.',70790), -- Vrang's Game
(95664,'It is a favored sign to have a stranger approach in this manner. I feel a power amidst you... what is this you have brought?',70334), -- Elder Knowledge (beta cache)
(95774,'I will place these adornments with Olgra''s memorial, so that I never forget her. My blood will always run hot with revenge from this day until my last day. Each swing of my blade and enemy felled will be in her name.\r\n\r\nYou have done me... done us... a great honor. \r\n\r\nLok-tar, $N.',70334), -- Her Name Is Olgra (beta cache)
(95981,'Ever since the barrier went down around Dalaran, the Kirin Tor have maintained a death grip on the roads. They''re paranoid about something, that much is certain.\r\n\r\nFor now, however, the why is irrelevant.',70790), -- Watching the Roads
(96101,'So now that we''ve learned the basics, why don''t you go and learn a tradeskill from the trainers in the village nearby. They will also teach you how to create a camp fixture suitable to your new profession.',70790), -- The Great Outdoors
(96241,'Heh... when I said unique properties, I wasn''t kiddin'' mon! You seem no worse for wear, so it be all good.\r\n\r\nThank you for your aid, $c. It be much appreciated.',70334), -- Talkin' bout Toxins, Mon (beta cache)
(96260,'These are excellent samples, and more than sufficient for my needs.\r\n\r\nHere, take this. I was able to craft a rather unique cloak with a weave I spun from the small sample of silk already in my possession. I am hopeful that you will find it useful in your travels.\r\n\r\nEarthmother watch over you, $c.',70334), -- Stronger than Steel (beta cache)
(96261,'Reanimated dead? The broodmother possessing a night elven form? These are troubling revelations. One has to wonder what it all means... I fear that must be a concern for a later time.\r\n\r\nThank you, $N. You''ve done the Earthen Ring and the people of Mulgore a great service.',70334), -- The Broodmother (beta cache)
(96390,'These Dark Irons may be our cousins but they ain''t family, that''s for sure. I shed no tears for any misfortune that befalls them that stray too far from that accursed mountain o'' theirs.\r\n\r\nHopefully this is enough to put ''em off of further incursions into these lands.',70338), -- Nip 'Em in the Bud (beta cache)
(96604,'So now that we''ve learned the basics, go learn a tradeskill from the trainers in the village nearby. They will also teach you how to create a camp fixture suitable to your new profession.',70840), -- The Great Outdoors
(96607,'So now that we''ve learned the basics, why don''t you go and learn a tradeskill from the trainers in the village nearby. They will also teach you how to create a camp fixture suitable to your new profession.',70790), -- The Great Outdoors
(96638,'Well met!',70790), -- The Adventurer
(96646,'With this, you''ll be able to set up a camp anywhere out in the wilderness. What you''ve learned is just the basics, so you won''t be able to place too many furnishings around it.\r\n\r\nRaise your cooking skill and you might just be able to learn to make even bigger fires.\r\n\r\nYou can also learn the basics of furnishing your campsites from other tradeskill trainers if you''re savvy in their skills.',70790), -- Camping 101: Cooking
(96656,'I''m not affiliated with the Scarlet Crusade, if you couldn''t tell. I''m just an adventurer stopping by before my next venture into the wilderness.\r\n\r\nHey, you''ve got the look of an adventurer yourself. How would you like to learn about camping?',70790), -- The Adventurer
(96658,'With this, you''ll be able to set up a camp anywhere out in the wilderness. What you''ve learned is just the basics, so you won''t be able to place too many furnishings around it.\r\n\r\nRaise your cooking skill and you might just be able to learn to make even bigger fires.\r\n\r\nYou can also learn the basics of furnishing your campsites from other tradeskill trainers if you''re savvy in their skills.',70245), -- Camping 101: Cooking (beta cache)
(96821,'The peons have been scared to travel south for some time, $N. Now that you''ve cleared the path a bit, we can get them back to work.',70840), -- Legging It
(96822,'These warriors will always be remembered, $N. You have made sure of that.',70840), -- For Honor
(96825,'I''ll use these to prepare something tasty, and save the seeds for planting. Someday we''ll have prickly pears growing all around Orgrimmar!',70790), -- This Fruit Could Bite Back
(96876,'My pack! When I dropped it to run from that huge spider, I thought I''d never see it again. You really are a hero!',70790), -- Ukor's Lost Pack
(96895,'Stay low and try not to be seen, $c. These cultists are not to be trifled with.',70790), -- The Argent Emissary
(96897,'Thank you for your aid, $c. If we do not wish to see Lordaeron''s history repeat itself, we''ve got to be vigilant when threats like this spring up.',70790), -- The Cult of the Damned
(96898,'Thank you. This should slow the efforts of the Scourge in this area. I suspect the denizens of Tirisfal will be pulling these crystals out of the ground for years, but removing the most conspicuous of them can only help.',70790), -- Remnants of War
(96899,'This place is full of memories for me. This place was once held by the Argent Dawn. It was from here that we prepared for many battles and skirmishes against the Scourge during their most recent push into Tirisfal Glades.\r\n\r\nWhen we withdrew to the Plaguelands I feared it would fall into disrepair. My fear was well-founded, but I am glad that Danitha and her paladins have taken over stewardship of it now.\r\n\r\nShe even named the keep after... a mutual friend.',70790), -- Bandarion Keep
(97003,'Impressive, $c. None will doubt my tales when I return to Orgrimmar with this grim proof of the existence of these abominations. Rest assured this glory is yours however, and I will speak loudly of your deeds.\r\n\r\nLok-tar!',70790), -- Chol'aruk the Ravener
(97223,'Thank you, $N. I am happy we will be able to preserve her bloodline and raise them to be our allies. You have done a great service for our tribe.',70840), -- Bloodtalon Matriarch
(97225,'You have done a great deed for our tribe today.\r\n\r\nUnlike some tribes, our kinship with our loa is one of reverence and respect, and we owe them much.\r\n\r\nI would not want the Darkspear to fall out of favor with them.',70840), -- Forgotten Loa Idols
(97253,'Whooooa! Nice one, pal! You saved me from another one of Gagsprocket''s unbearable lectures on the cost of metal. More importantly, I can jump right into building my next prototype.\r\n\r\nUhhh, I didn''t have a reward planned... Do you like boxes?',70790), -- Parts and Pieces
(97279,'Thank you for returning these weapons, $N. I will get to work repairing them for the next wave of recruits.',70840), -- Wayward Weapons
(97281,'Woah, where did you find a neat thing like that? Ya know it reminds me of the other thing my boss Gazlowe always used to tell me...',70790), -- A Simmering Storm
(97282,'Hey, thanks pal! You''ve been a really big help to me here and I appreciate it.\r\n\r\nHere''s to hoping I can get this all charged up using these organs. I bet it''ll make those no good harpies think twice about attacking my caravan again. I sure hope this works.\r\n\r\nEither way you''ve done your side of the deal. Take this for your trouble.',70790), -- Stormy Potential
(97288,'Oho! I believe I know this head. A cruel joke by our Dark Lady. A delicious one at that.\r\n\r\nI think I have a use for this.',70790), -- Unending Torment
(97289,'<The body twitches to life, panicking as it relives its final moments.>\r\n\r\nYou...',70790), -- Unending Torment
(97290,'Not cooperating, eh? Excellent. I always love a challenge.',70790), -- Unending Torment
(97292,'Excellent. We''ll let his flesh simmer for awhile. Give him time to acclimate to his new home.\r\n\r\nI have no further use for the wretch for now, but we shall keep an eye on him. I have many plans for Othmar''s future.',70790), -- Unending Torment
(97957,'There you have it, you can now craft your very own Incense Candle. Most folk don''t care much for camps being set up in the center of town, so you''ll need to head into the wilderness if you want to test it out.\r\n\r\nOh, and just remember, no camp is complete without a campfire. You''ll need one of those before you start up your camp. Most cooks can teach you to make a fire. If you haven''t already, you might want to check in with someone who can train you in the culinary arts to learn the basics.',70245), -- Camping 101: Herbalism (beta cache)
(97968,'There you have it, you can now craft your very own Incense Candle. Most folk don''t care much for camps being set up in the center of town, so you''ll need to head into the wilderness if you want to test it out.\r\n\r\nOh, and just remember, no camp is complete without a campfire. You''ll need one of those before you start up your camp. Most cooks can teach you to make a fire. If you haven''t already, you might want to check in with someone who can train you in the culinary arts to learn the basics.',70245), -- Camping 101: Herbalism (beta cache)
(97970,'There you have it, you can now craft your very own Lodestone. Most folk don''t care much for camps being set up in the center of town, so you''ll need to head into the wilderness if you want to test it out.\r\n\r\nOh, and just remember, no camp is complete without a campfire. You''ll need one of those before you start up your camp. Most cooks can teach you to make a fire. If you haven''t already, you might want to check in with someone who can train you in the culinary arts to learn the basics.',70235), -- Camping 101: Mining (beta cache)
(98340,'And so it continues, Father Cenarius.\r\n\r\n<Dendrite makes a quick, invisible gesture in the air above him. A slight aura of power falls over him.>\r\n\r\nGreetings again, $N. It is time for you to learn the next perspective of nature: the cat. I will put you on the path to understanding this aspect of druidic life, but much like the bear, it is you that must embrace this learning and propagate it.',70790), -- The Great Cat Spirit
(98341,'Greetings, young shen''dorei. I have come to you because I sense you are ready to undertake the trial to determine if you understand my spiritual nature or not.\r\n\r\nI will have you take the trial of the Great Cat Spirit of Azeroth, but as you are from Zephras, I wished to oversee your progress myself. After all, it is my shape you would take, not that of an Azerothian cat spirit.\r\n\r\nLet us begin.',70790), -- The Great Windborne Cat Spirit
(98362,'Welcome back, $N. I can sense you have met with the Great Cat Spirit and have successfully completed the trial. All druids must pass a trial given by this spirit, though the trials vary by generation. I am proud you have passed and have heeded the wisdom of the Great Cat Spirit. It is an ancient spirit that is as deadly as it is playful, though I can sense you understand the spirit''s nature already.\r\n\r\nI will now instruct you on how to take the shape of the cat.',70790), -- To Thunder Bluff
(98404,'Excellent. You have shown you understand the true nature of the cat, and are able to embody the cunning, stealth, and ferocity needed to take my shape.\r\n\r\nI now deem you worthy of my blessing, young $c.',70790), -- The Great Windborne Cat Spirit
(98575,'You have just glimpsed the power that can be yours. The feeble and foolish among us are afraid of the path that we tread, but with boldness and cunning we can decide our own fates.\r\n\r\nGuard your powers closely. The fearful lash out against that which they do not understand.\r\n\r\nNow you are one of us. If you need training in spells then return to me.',70790), -- Tainted Tablet
(98738,'<Dendrite makes another invisible gesture in the air above him as you approach. Another aura of power comes over him, and he smiles ever so slightly.>\r\n\r\nI sense the cunning and ferocity of the cat within you that was not present when I sent you to meet the Great Cat Spirit. In your eyes I can sense you now understand the stealth and speed needed to adopt this nature.\r\n\r\nNow you may learn to focus your cunning and adopt the true nature of the cat and call upon its spirit.',70790), -- Blessings of the Great Windborne Cat Spirit
(98823,'Ah... this shard hums with the voice of the world itself. The light within is a memory from the past, glimmering with flashes of history. This titan machine must have been waiting to be understood for lifetimes.\r\n\r\nYou''ve done wisely to bring it to us. The Earthen Ring will safeguard it, study it, and share the truths it keeps when we are able to receive its messages.\r\n\r\nYou honor the elements today, traveler.',70334), -- Earthen Echo (beta cache)
(99048,'Heglan''s body lies cold within the wreckage of the hut.\r\n\r\nWhatever killed her doesn''t seem to be human. There may be signs of what happened nearby.',70840), -- A Missing Hand
(99052,'Ha! I could hear his body fall from here!\r\n\r\nBe proud of your kill today; not many can claim they survived a fight with a sea giant.',70790), -- Threat from Below
(99142,'Most glorious indeed!\r\n\r\n<Junior Apothecary Holland inspects the weeds.>\r\n\r\nHmm, I had hoped they would have matured a bit more violently. Nevertheless, progress is progress!\r\n\r\nA great many will die as a result of your actions this day. You should be proud.',70245), -- Tomb Weed (beta cache)
(99156,'You are a testament to the cunning and coordination that all Forsaken possess--something the mindless Scourge will never have.\r\n\r\nPlease accept this as thanks for your efforts, as promised.',70790); -- Rear Guard Patrol

-- P2b: quest_request_items - progress texts for 41 quests that had none (EG Link, text verbatim).
INSERT IGNORE INTO `quest_request_items` (`ID`,`CompletionText`,`VerifiedBuild`) VALUES
(79361,'There are many who practice shamanism in this world. It is up to us to ensure that the land is protected from misuse.',70338), -- Storm, Earth, and Fire (beta cache)
(86576,'I have important matters to consider, $c. Be quick.',70840), -- Bloodfury Trinkets
(90902,'Mastery of the light will return, in time.',70334), -- Rediscovering the Light (beta cache)
(91920,'Oh. You''re back?',70790), -- Wild Eyes
(91921,'You return! I was beginning to come to terms with devolving into a ghoul.',70790), -- Return to Quinn (Again)
(92463,'Cirrusflies aren''t to be underestimated. They might not seem dangerous, but you have to watch out for the bigger ones. The bigger they are, the meaner!',70245), -- The Cirrusfly Queen (beta cache)
(92484,'Winds guide you, $N.',70338), -- Embracing the Elements (beta cache)
(92701,'A letter from Aonda. What''s that crafty highlander been up to?',70245), -- To Valanaar (beta cache)
(92850,'Those shrieklings are too dangerous to be left to nest this close to Valanaar. However... they are the least of our troubles.',70245), -- The Missing Scholar (beta cache)
(94013,'You will learn, $N, and one day you will be a great $c.',70245), -- Taming the Beast (beta cache)
(94978,'You will learn, $N, and one day you will be a great $c.',70245), -- Taming the Beast (beta cache)
(94979,'You will learn, $N, and one day you will be a great $c.',70245), -- Taming the Beast (beta cache)
(95212,'You look chilly, $c. How goes the hunt?',70338), -- Never Saddle on Quality (beta cache)
(95216,'Any fool can tell you the most potent transmission method will always be through the blood.\r\n\r\nI must see this strain for myself!',70790), -- The New Plague
(95494,'How goes the hunt?',70338), -- Bruised Pride and Lion Hides (beta cache)
(95495,'Greetings, $c. What brings you up to my property today?',70338), -- The Hermit Tanner (beta cache)
(95507,'<Vrang grunts at you as you approach.>',70790), -- Vrang's Game
(95664,'Spirits honor you. Have you come to share with us?',70334), -- Elder Knowledge (beta cache)
(95774,'I have only memories now, but I will build a tribute to Olgra so that they never fade.',70790), -- Her Name Is Olgra
(96241,'Did ya get what I need?',70334), -- Talkin' bout Toxins, Mon (beta cache)
(96260,'This silk''s remarkable properties could be used to make lightweight, durable cloth with remarkable defensive properties. I am eager to experiment with more of it.',70334), -- Stronger than Steel (beta cache)
(96822,'Have you done what I asked?',70840), -- For Honor
(96825,'Did you find the fruits?',70790), -- This Fruit Could Bite Back
(96876,'Something need doing?',70840), -- Ukor's Lost Pack
(96898,'I''d advise you not come into too much physical contact with those crystals. Even shattered and spent, the lingering magic in these necrotic crystals is highly dangerous.',70790), -- Remnants of War
(97003,'It took the strength of six skilled fighters to defeat one of these creatures, and it still managed to fell four of us before we could put it down.\r\n\r\nIf these monstrous beasts were to appear in greater numbers, the damage that the quilboar could inflict upon the land could be catastrophic.',70790), -- Chol'aruk the Ravener
(97223,'The Bloodtalon are known for their cunning and swiftness, and make fine companions when raised from egg to adult.\r\n\r\nUnfortunately adult raptors are very difficult to train, and I have tried to capture the Matriarch on more than one occasion. But she had become too dangerous of late.\r\n\r\nStill I hope to preserve her legacy by raising her children as companions here in Sen''Jin.',70840), -- Bloodtalon Matriarch
(97225,'Have you found our loa idols, $N? We must begin our rituals again soon.',70840), -- Forgotten Loa Idols
(97253,'Not another word out of you until your pockets are jingling with metal!',70790), -- Parts and Pieces
(97279,'Throm''ka, $c. Gorak told me I should be expecting you. It is good to see you in one piece!\r\n\r\nWere you able to collect the training weapons I requested?',70840), -- Wayward Weapons
(97282,'It might come as a surprise that with a name like thunder lizard they shoot lightning instead of roaring like thunder.\r\n\r\nWell, they do. Shoot lightning, I mean.\r\n\r\nTrust me, it''s a lesson you only get to learn once if you''re lucky.',70790), -- Stormy Potential
(97288,'What have we here?',70790), -- Unending Torment
(97289,'<The abomination remains lifeless without a head to control it.>',70790), -- Unending Torment
(97292,'For Othmar''s crimes against the Dark Lady, placing him under that oozing specimen is the least he deserves.\r\n\r\nAnd it''s funny...',70790), -- Unending Torment
(98389,'Have you freed the poor, unfortunate souls trapped in that cave?',70425), -- A Light in the Darkness (beta cache)
(98404,'Ah, you have returned. I do hope you have brought the relics with you?',70790), -- The Great Windborne Cat Spirit
(98575,'Ah, you received my message. Excellent. We orcs have a long and turbulent history with the entities of the Nether, but now we forge our own path, in command of our destiny. You, too, can seize the power to shape the future for your people. Where our leaders falter, we are ready to press forward and claim our rightful place. You will be of eminence among the trolls.',70790), -- Tainted Tablet
(98823,'Come. Seek and receive.',70334), -- Earthen Echo (beta cache)
(99052,'Give ''em blood and thunder!',70790), -- Threat from Below
(99142,'More interruptions! Wait, it''s you. Were you able to recover the tomb weed?',70790), -- Tomb Weed
(99156,'Have you dealt with our little problem?',70790); -- Rear Guard Patrol

-- P2c: quest_objectives for 4 quests that had none (EG Link object ids, cross-checked with forever-db).
-- Added only while the quest still has no objective at all.
-- Rite of Vision: item 4823 x1 (also the quest start item)
INSERT INTO `quest_objectives` (`ID`,`QuestID`,`Type`,`Order`,`StorageIndex`,`ObjectID`,`Amount`,`ConditionalAmount`,`Flags`,`Flags2`,`ProgressBarWeight`,`ParentObjectiveID`,`Visible`,`Description`,`VerifiedBuild`)
SELECT 480587,772,1,0,0,4823,1,0,32,0,0,0,1,'',70840 FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=772 OR `ID`=480587);
-- Crown of the Earth: item 5646 x1
INSERT INTO `quest_objectives` (`ID`,`QuestID`,`Type`,`Order`,`StorageIndex`,`ObjectID`,`Amount`,`ConditionalAmount`,`Flags`,`Flags2`,`ProgressBarWeight`,`ParentObjectiveID`,`Visible`,`Description`,`VerifiedBuild`)
SELECT 381475,934,1,0,0,5646,1,0,32,0,0,0,1,'',70840 FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=934 OR `ID`=381475);
-- Hidden Enemies: creature 3216 x1
INSERT INTO `quest_objectives` (`ID`,`QuestID`,`Type`,`Order`,`StorageIndex`,`ObjectID`,`Amount`,`ConditionalAmount`,`Flags`,`Flags2`,`ProgressBarWeight`,`ParentObjectiveID`,`Visible`,`Description`,`VerifiedBuild`)
SELECT 480284,5727,0,0,0,3216,1,0,32,0,0,0,1,'Gauge Neeru Fireblade''s reaction',70840 FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=5727 OR `ID`=480284);
-- Prompt Potion Runner: talk to 11044 (beta cache)
INSERT INTO `quest_objectives` (`ID`,`QuestID`,`Type`,`Order`,`StorageIndex`,`ObjectID`,`Amount`,`ConditionalAmount`,`Flags`,`Flags2`,`ProgressBarWeight`,`ParentObjectiveID`,`Visible`,`Description`,`VerifiedBuild`)
SELECT 477632,97891,3,0,0,11044,1,0,32,0,0,0,1,'Speak to Doctor Martin Felben',70245 FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=97891 OR `ID`=477632);
