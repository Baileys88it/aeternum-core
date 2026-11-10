-- Aeternum arena (2026-10-28): per-bracket personal rating, seasons, pending rewards and end-of-season results.
-- Apply to: characters. Used by src/server/scripts/Custom/Classic/world/wf_arena.cpp.
-- bracket: 0 = 1v1, 1 = 2v2, 2 = 3v3. Times are unix timestamps. The script creates season 1 itself on first start.
CREATE TABLE IF NOT EXISTS `custom_arena_season` (
  `id` INT UNSIGNED NOT NULL,
  `start_time` INT UNSIGNED NOT NULL DEFAULT 0,
  `end_time` INT UNSIGNED NULL DEFAULT NULL COMMENT 'NULL = stagione in corso',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aeternum arena: stagioni';

CREATE TABLE IF NOT EXISTS `custom_arena_rating` (
  `guid` BIGINT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `bracket` TINYINT UNSIGNED NOT NULL COMMENT '0 = 1v1, 1 = 2v2, 2 = 3v3',
  `season` INT UNSIGNED NOT NULL,
  `rating` INT NOT NULL DEFAULT 1500,
  `wins` INT UNSIGNED NOT NULL DEFAULT 0,
  `losses` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `bracket`, `season`),
  KEY `idx_leaderboard` (`season`, `bracket`, `rating`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aeternum arena: rating personale (Elo) per bracket e stagione';

CREATE TABLE IF NOT EXISTS `custom_arena_pending_reward` (
  `guid` BIGINT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `honor` INT UNSIGNED NOT NULL DEFAULT 0,
  `titles` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'CharTitles ID separati da spazio',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aeternum arena: honor e titoli da consegnare fuori dall''arena';

CREATE TABLE IF NOT EXISTS `custom_arena_season_result` (
  `season` INT UNSIGNED NOT NULL,
  `bracket` TINYINT UNSIGNED NOT NULL,
  `rank` TINYINT UNSIGNED NOT NULL,
  `guid` BIGINT UNSIGNED NOT NULL,
  `rating` INT NOT NULL,
  `wins` INT UNSIGNED NOT NULL,
  `losses` INT UNSIGNED NOT NULL,
  `title` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'CharTitles ID assegnato, 0 = nessuno',
  PRIMARY KEY (`season`, `bracket`, `rank`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aeternum arena: classifica finale di ogni stagione (.arena season end)';
