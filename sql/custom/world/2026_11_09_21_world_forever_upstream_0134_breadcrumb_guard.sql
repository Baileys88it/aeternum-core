-- Follow-up guard for 2026_11_09_20_world_forever_upstream_0134_selected.sql
-- Source: Karrz1/TrinityCore (branch forever, "Sync with WoW Forever 0.1.34", ff699fb5cf), GPL-2.0.
-- The upstream addon row for 96630 "The Adventurer" points BreadcrumbForQuestId at 96606, a quest that
-- only exists upstream as a placeholder (not imported here). The worldserver ignores it and logs
-- "Quest 96630 is a breadcrumb for quest 96606, but no such quest exists". Clear it only while 96606 is missing.
UPDATE `quest_template_addon` a LEFT JOIN `quest_template` q ON q.`ID`=96606
SET a.`BreadcrumbForQuestId`=0 WHERE a.`ID`=96630 AND a.`BreadcrumbForQuestId`=96606 AND q.`ID` IS NULL;