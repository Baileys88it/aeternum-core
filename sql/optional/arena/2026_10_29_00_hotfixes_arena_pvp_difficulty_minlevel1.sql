-- Aeternum arena (2026-10-29): skirmishes open from level 1 (Aeternum.Arena.MinLevel = 1).
-- PvpDifficulty 1757 (Mak'gora Arena, map 2995) MinLevel 10 -> 1, matching the new queue: the custom matchmaking
-- always creates the arena on bracket 0 and does not filter on this row, but MinLevel shows in announcements and BG data.
-- Database: hotfixes. Loaded at worldserver start (effective after the next restart). No hotfix_data row:
-- the override stays server-side like the original 1757 (2026_10_28_01_hotfixes_arena_map.sql).
UPDATE `pvp_difficulty` SET `MinLevel` = 1 WHERE `ID` = 1757 AND `MapID` = 2995;