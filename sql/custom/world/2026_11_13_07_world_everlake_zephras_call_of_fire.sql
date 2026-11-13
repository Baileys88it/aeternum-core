-- Efficient Games build 70862, CC BY 4.0 (https://efficient.games).
-- ChunkLocked Forever identifies Swart and Sessaria. Quest text also names Aarnor.
START TRANSACTION;

SET @add_call_of_fire := NOT EXISTS (SELECT 1 FROM quest_template WHERE ID = 98517)
 AND NOT EXISTS (SELECT 1 FROM quest_classic_level WHERE ID = 98517)
 AND NOT EXISTS (SELECT 1 FROM quest_template_classic_level WHERE ID = 98517)
 AND NOT EXISTS (SELECT 1 FROM quest_template_addon WHERE ID = 98517)
 AND NOT EXISTS (SELECT 1 FROM creature_queststarter WHERE quest = 98517)
 AND NOT EXISTS (SELECT 1 FROM creature_questender WHERE quest = 98517)
 AND EXISTS (SELECT 1 FROM creature WHERE id = 3173 AND map = 1)
 AND EXISTS (SELECT 1 FROM creature WHERE id = 252382 AND map = 2991)
 AND EXISTS (SELECT 1 FROM creature WHERE id = 254082 AND map = 2991);

INSERT INTO quest_template
    (ID, QuestType, QuestSortID, Flags, AllowableRaces, RewardXPDifficulty,
     RewardXPMultiplier, LogTitle, LogDescription, QuestDescription, VerifiedBuild)
SELECT 98517, 2, -82, 2097152, 8589934592, 3, 1, 'Call of Fire',
    'Report to Sessaria Skystride or Aarnor Galestrike on Zephras Isle to continue your training as a Shaman.\r\n\r\nYou can return to Zephras via the skycutter transport that docks in Mulgore at Skywatcher Plateau, northwest of Thunder Bluff.',
    'Greetings, young Skyborne. The spirits have spoken of you, and tell me that you are ready to heed the call of fire. However, I shall not be the one to start you on that Path. Return to your home in the elemental plane, seek your mentors there. They will tell you what must be done.',
    70862 WHERE @add_call_of_fire;

INSERT INTO quest_classic_level (ID, QuestLevel, MinLevel, MaxLevel)
SELECT 98517, 10, 10, 0 WHERE @add_call_of_fire;

INSERT INTO quest_template_classic_level (ID, LevelType, QuestLevel)
SELECT 98517, 2, 10 WHERE @add_call_of_fire;

INSERT INTO quest_template_addon (ID, AllowableClasses)
SELECT 98517, 64 WHERE @add_call_of_fire;

INSERT INTO creature_queststarter (id, quest)
SELECT 3173, 98517 WHERE @add_call_of_fire;

INSERT INTO creature_questender (id, quest)
SELECT id, 98517 FROM (SELECT 252382 AS id UNION ALL SELECT 254082) recipients
WHERE @add_call_of_fire;

COMMIT;
