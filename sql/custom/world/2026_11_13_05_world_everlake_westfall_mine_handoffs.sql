-- ForeverDB player captures confirm these Westfall investigation prerequisites.
-- https://www.foreverdb.app/en/forever/quests/92742/testing-the-wells
UPDATE quest_template_addon a
JOIN quest_template previous ON previous.ID = CASE a.ID WHEN 92745 THEN 92744 WHEN 92752 THEN 92751 END
SET a.PrevQuestID = previous.ID
WHERE a.ID IN (92745, 92752) AND a.PrevQuestID = 0;
