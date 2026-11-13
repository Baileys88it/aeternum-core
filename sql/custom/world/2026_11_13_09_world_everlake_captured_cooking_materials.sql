-- Efficient Games (https://efficient.games), CC BY 4.0, build 70790 confirms both loot sources.
-- Rates from the Wowhead Forever samples bundled in ChunkLocked 0.14.0-forever.2.
INSERT INTO creature_loot_template
    (Entry, ItemType, Item, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment)
SELECT src.npc, 0, src.item, src.chance, 0, 1, 0, 1, 1, src.comment
FROM (
    SELECT 69 AS npc, 2672 AS item, 1.45675 AS chance, 'Timber Wolf - Stringy Wolf Meat' AS comment
    UNION ALL SELECT 250927, 6889, 44.15406, 'Shadowgale Ursera - Small Egg'
) src
JOIN creature_template_difficulty d ON d.Entry = src.npc AND d.DifficultyID = 0 AND d.LootID = src.npc
WHERE EXISTS (SELECT 1 FROM creature WHERE id = src.npc)
  AND NOT EXISTS (SELECT 1 FROM creature_template_difficulty WHERE LootID = src.npc AND Entry <> src.npc)
  AND NOT EXISTS (SELECT 1 FROM creature_loot_template WHERE Entry = src.npc AND ItemType = 0 AND Item = src.item);
