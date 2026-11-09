-- Classic 1.60.1.70707+ (70790/70862 too): SpellDuration.db2 has DurationPerLevel again as its second field (layout hash 0x4FDF0951).
-- The engine already selects it (HOTFIX_SEL_SPELL_DURATION) but sql/base/dev/hotfixes_database.sql has no such column, so a fresh
-- hotfixes database could not prepare the statement. Idempotent: does nothing when the column already exists.
SET @sql := IF((SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'spell_duration' AND COLUMN_NAME = 'DurationPerLevel') = 0,
    'ALTER TABLE `spell_duration` ADD COLUMN `DurationPerLevel` int NOT NULL DEFAULT ''0'' AFTER `Duration`', 'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
