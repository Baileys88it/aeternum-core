-- Forever does not ship the retail/TBC+ All Arenas queue.
-- Battleground template 6 (BATTLEGROUND_AA) is a container. Its maps are Nagrand Arena (4, map 559),
-- Blade's Edge Arena (5, map 562), Ruins of Lordaeron (8, map 572), Dalaran Sewers (10, map 617)
-- and Ring of Valor (11, map 618). After those maps were removed from this data set the container
-- stayed registered with no map ids, and an arena join logged
-- "template bg (all arenas) has no map ids, arena join by <name> ignored".
--
-- Mak'gora (battleground 1169, map 2995, battleground_scripts arena_wf_makgora) is not in these lists.
-- Idempotent: deleting rows that phase 2b already removed is a no-op.

-- Battlemaster gossip that only opens one of these queues. Menus still used by a kept battleground stay.
DELETE loc FROM `gossip_menu_option_locale` loc
INNER JOIN `gossip_menu_option` o ON o.`MenuID` = loc.`MenuID` AND o.`OptionID` = loc.`OptionID`
INNER JOIN `creature_template_gossip` g ON g.`MenuID` = o.`MenuID`
INNER JOIN `battlemaster_entry` b ON b.`entry` = g.`CreatureID`
WHERE o.`OptionNpc` = 9
  AND b.`bg_template` IN (4, 5, 6, 8, 10, 11)
  AND NOT EXISTS (
      SELECT 1
      FROM `creature_template_gossip` g2
      INNER JOIN `battlemaster_entry` b2 ON b2.`entry` = g2.`CreatureID`
      WHERE g2.`MenuID` = o.`MenuID`
        AND b2.`bg_template` NOT IN (4, 5, 6, 8, 10, 11)
  );

DELETE o FROM `gossip_menu_option` o
INNER JOIN `creature_template_gossip` g ON g.`MenuID` = o.`MenuID`
INNER JOIN `battlemaster_entry` b ON b.`entry` = g.`CreatureID`
WHERE o.`OptionNpc` = 9
  AND b.`bg_template` IN (4, 5, 6, 8, 10, 11)
  AND NOT EXISTS (
      SELECT 1
      FROM `creature_template_gossip` g2
      INNER JOIN `battlemaster_entry` b2 ON b2.`entry` = g2.`CreatureID`
      WHERE g2.`MenuID` = o.`MenuID`
        AND b2.`bg_template` NOT IN (4, 5, 6, 8, 10, 11)
  );

-- Drop the battlemaster flag before the entry, so startup does not log
-- "has UNIT_NPC_FLAG_BATTLEMASTER, but no data in the battlemaster_entry table".
UPDATE `creature_template` c
INNER JOIN `battlemaster_entry` b ON b.`entry` = c.`entry`
SET c.`npcflag` = c.`npcflag` & ~0x00100000
WHERE b.`bg_template` IN (4, 5, 6, 8, 10, 11);

DELETE FROM `battlemaster_entry` WHERE `bg_template` IN (4, 5, 6, 8, 10, 11);
DELETE FROM `battleground_template` WHERE `ID` IN (4, 5, 6, 8, 10, 11);
DELETE FROM `battleground_scripts` WHERE `MapId` IN (559, 562, 572, 617, 618);

-- sourceType 3 = DISABLE_TYPE_BATTLEGROUND. Template 6 exists in BattlemasterList, so this row is valid.
-- The loader also refuses to register these ids; the disable stops a reinserted row from being queued
-- if that check is ever removed. Do not disable 1169.
DELETE FROM `disables` WHERE `sourceType` = 3 AND `entry` = 6;
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(3, 6, 0, '', '', 'Forever: retail All Arenas (BATTLEGROUND_AA) is not available');
