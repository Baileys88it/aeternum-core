-- Rollback for 2026_11_12_00_hotfixes_forever_missile_barrage_period.sql
-- Removes the override row and its hotfix entry (client data value comes back). Requires a world server restart.
DELETE FROM `spell_effect` WHERE `ID` = 1045767 AND `VerifiedBuild` = 70862;
DELETE FROM `hotfix_data` WHERE `Id` = 2110002 AND `TableHash` = 4030871717 AND `RecordId` = 1045767;