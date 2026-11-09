-- Rollback for 2026_11_09_21_world_forever_upstream_0134_breadcrumb_guard.sql
UPDATE `quest_template_addon` SET `BreadcrumbForQuestId`=96606 WHERE `ID`=96630 AND `BreadcrumbForQuestId`=0;