-- Efficient Games, CC BY 4.0 (https://efficient.games), build 70790.
-- Thistle Boar yielded one Ruined Leather Scraps in all 13 captured skinning windows.
START TRANSACTION;
SET @add_thistle_skinning := EXISTS (
    SELECT 1 FROM creature_template_difficulty
    WHERE Entry = 1985 AND DifficultyID = 0 AND SkinLootID = 0
) AND NOT EXISTS (
    SELECT 1 FROM creature_template_difficulty WHERE SkinLootID = 1985
) AND NOT EXISTS (
    SELECT 1 FROM skinning_loot_template WHERE Entry = 1985
);

INSERT INTO skinning_loot_template
    (Entry, ItemType, Item, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment)
SELECT 1985, 0, 2934, 100, 0, 1, 0, 1, 1, 'Thistle Boar - Ruined Leather Scraps'
WHERE @add_thistle_skinning;

UPDATE creature_template_difficulty
SET SkinLootID = 1985
WHERE Entry = 1985 AND DifficultyID = 0 AND SkinLootID = 0 AND @add_thistle_skinning;
COMMIT;
