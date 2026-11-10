-- Aeternum arena (2026-10-28): Mak'gora Arena (map 2995, "Hyjal Crater" in client 70532) as a server-side arena.
-- Apply to: hotfixes.
--
-- The client's Map.db2 has InstanceType = 0 for 2995: without an override the core does not treat it as an arena
-- (battleground_scripts rejects it, no BattlegroundMap is created). The row below is the client row, identical
-- field by field; only InstanceType changes 0 -> 4 (MAP_ARENA). VerifiedBuild must be > 0 (the hotfix loader
-- ignores rows with VerifiedBuild = 0).
--
-- BattlemasterListXMap 1169 -> 2995 already exists in the client DB2: do not duplicate it here.
-- PVPDifficulty has no rows for 2995: without one, CreateNewBattleground fails (no bracket).
DELETE FROM `map` WHERE `ID` = 2995;
INSERT INTO `map` (`ID`,`Directory`,`MapName`,`MapDescription0`,`MapDescription1`,`PvpShortDescription`,`PvpLongDescription`,
  `CorpseX`,`CorpseY`,`MapType`,`InstanceType`,`ExpansionID`,`AreaTableID`,`LoadingScreenID`,`TimeOfDayOverride`,`ParentMapID`,
  `CosmeticParentMapID`,`TimeOffset`,`MinimapIconScale`,`CorpseMapID`,`MaxPlayers`,`WindSettingsID`,`ZmpFileDataID`,`WdtFileDataID`,
  `OceanLiquidTypeID`,`NavigationMaxDistance`,`PreloadFileDataID`,`Flags1`,`Flags2`,`Flags3`,`VerifiedBuild`) VALUES
(2995,'2995','Hyjal Crater','','','','',0,0,1,4,0,0,0,900,-1,-1,0,1,-1,0,136,0,7245741,2,0,0,8396381,1040,2281568,70532);

DELETE FROM `pvp_difficulty` WHERE `ID` = 1757;
INSERT INTO `pvp_difficulty` (`ID`,`RangeIndex`,`MinLevel`,`MaxLevel`,`MapID`,`VerifiedBuild`) VALUES
(1757,0,10,60,2995,70532);

-- OPTIONAL: also send the override to the client. Without this row the server treats 2995
-- as an arena but the client still sees a normal map (arena UI / scoreboard may not appear).
-- TableHash 0xBD84CD62 = Map.db2.
-- DELETE FROM `hotfix_data` WHERE `Id` = 2000102;
-- INSERT INTO `hotfix_data` (`Id`,`UniqueId`,`TableHash`,`RecordId`,`Status`,`VerifiedBuild`) VALUES
-- (2000102,2000102,0xBD84CD62,2995,1,70532);
