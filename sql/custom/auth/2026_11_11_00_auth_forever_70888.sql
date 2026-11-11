-- Forever client 1.60.1.70888. Same build_info scheme as 70862 / 70790.
-- Idempotent. No build_auth_key or build_executable_hash row is added.
DELETE FROM `build_info` WHERE `build` IN (70888);
INSERT INTO `build_info` (`build`,`majorVersion`,`minorVersion`,`bugfixVersion`,`hotfixVersion`) VALUES
(70888,1,60,1,NULL);