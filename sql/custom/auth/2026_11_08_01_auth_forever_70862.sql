-- Forever client 1.60.1.70862. Same build_info scheme as 70790 / 70755.
-- Idempotent. sql/custom/auth is applied by the auth updater, so a fresh database gets the row.
-- No build_auth_key or build_executable_hash row. realmlist id 70 is set to 70862 so a fresh database matches live Aeternum.
DELETE FROM `build_info` WHERE `build` IN (70862);
INSERT INTO `build_info` (`build`,`majorVersion`,`minorVersion`,`bugfixVersion`,`hotfixVersion`) VALUES
(70862,1,60,1,NULL);
UPDATE `realmlist` SET `gamebuild`=70862 WHERE `id`=70;
