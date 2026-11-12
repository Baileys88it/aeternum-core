-- Hearthstone (item 6948, spell 8690): CategoryRecoveryTime 3600000 (1 hour) -> 1800000 (30 minutes).
-- Source: SpellCooldowns row 54105 in 1.60.1.70888 (wago.tools). RecoveryTime is 0; the cooldown is the category time.
-- ItemEffect 98264 has CoolDownMSec=-1 and CategoryCoolDownMSec=-1, so the item does not define a cooldown; only this row is required.
-- Spell category 89 ("Portal") is also used by 11365 (Bly's Band's Escape), 1225494 (Second Home) and 1315212 (a separate Hearthstone spell). Those rows are not changed.
-- TableHash 4193483863 = 0xF9F37C57 (SpellCooldowns). hotfix Id/UniqueId follow the 2110002 / 1464300002 precedent.

DELETE FROM `spell_cooldowns` WHERE `ID` = 54105 AND `VerifiedBuild` = 70862;
INSERT INTO `spell_cooldowns`
  (`ID`,`DifficultyID`,`CategoryRecoveryTime`,`RecoveryTime`,`StartRecoveryTime`,`AuraSpellID`,`SpellID`,`VerifiedBuild`)
VALUES
  (54105,0,1800000,0,1500,0,8690,70862);

DELETE FROM `hotfix_data` WHERE `TableHash` = 4193483863 AND `RecordId` = 54105;
INSERT INTO `hotfix_data` (`Id`,`UniqueId`,`TableHash`,`RecordId`,`Status`,`VerifiedBuild`)
VALUES (2110003,1464300003,4193483863,54105,1,70862);
