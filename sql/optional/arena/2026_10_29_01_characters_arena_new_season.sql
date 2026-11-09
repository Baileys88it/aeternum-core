-- Aeternum arena (2026-10-29): fresh season for the switch to "rated at level 60 only".
-- Closes the current season WITHOUT final standings and WITHOUT titles (no rows in custom_arena_season_result,
-- no rewards in custom_arena_pending_reward) and opens the next one: ratings restart from Rating.Start because the
-- script only loads custom_arena_rating rows of the open season. Rows of the closed season are kept.
-- Rewards already pending (honor) are not touched.
--
-- Database: characters. Apply with the worldserver STOPPED: the script keeps the season in memory and only rereads it
-- at startup (LoadDatabase); while running it would keep writing ratings to the old season.
-- Backup: *_20261029arena copies of the 4 custom_arena_* tables. A second run stops at the first CREATE TABLE
-- (backup already exists) and therefore does not open another season.

CREATE TABLE `custom_arena_season_20261029arena` LIKE `custom_arena_season`;
INSERT INTO `custom_arena_season_20261029arena` SELECT * FROM `custom_arena_season`;
CREATE TABLE `custom_arena_rating_20261029arena` LIKE `custom_arena_rating`;
INSERT INTO `custom_arena_rating_20261029arena` SELECT * FROM `custom_arena_rating`;
CREATE TABLE `custom_arena_season_result_20261029arena` LIKE `custom_arena_season_result`;
INSERT INTO `custom_arena_season_result_20261029arena` SELECT * FROM `custom_arena_season_result`;
CREATE TABLE `custom_arena_pending_reward_20261029arena` LIKE `custom_arena_pending_reward`;
INSERT INTO `custom_arena_pending_reward_20261029arena` SELECT * FROM `custom_arena_pending_reward`;

START TRANSACTION;
SET @wf_now := UNIX_TIMESTAMP();
SET @wf_next := (SELECT IFNULL(MAX(`id`), 0) + 1 FROM `custom_arena_season`);
UPDATE `custom_arena_season` SET `end_time` = @wf_now WHERE `end_time` IS NULL;
INSERT INTO `custom_arena_season` (`id`, `start_time`) VALUES (@wf_next, @wf_now);
COMMIT;

SELECT `id`, `start_time`, `end_time` FROM `custom_arena_season` ORDER BY `id`;