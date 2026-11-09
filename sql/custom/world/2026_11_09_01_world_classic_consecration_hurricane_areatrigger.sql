-- Classic 1.60: Consecration and Hurricane of the official client data create an area trigger (effect 1, create properties below)
-- and put a periodic dummy aura on the caster (effect 2), like the ground spells of 2026_10_04_03_world_classic_ground_spells.sql.
-- None of these create properties were on the server ("Invalid areatrigger create properties id" 41311 in Server.log): no ground
-- effect was shown (Consecration SpellVisual 5600 / Hurricane 4346 play their ground kit on the area trigger). Visual only, no
-- action set: the damage stays with the scripts (classic_spell_pal_consecration, classic_spell_dru_hurricane).
-- Areas: spheres with the effect radius of the client data (SpellEffect EffectRadiusIndex 14 = SpellRadius 8 yd, build 1.60.1.70862),
-- lasting the spell's duration (Consecration 8 s, Hurricane 10 s channel). Safe to run again.
DELETE FROM `areatrigger_template` WHERE `Id` IN (41311,41313,41314,41315,41316,41199,41198,41170) AND `IsCustom` = 1;
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `ActionSetId`, `ActionSetFlags`, `VerifiedBuild`) VALUES
(41311, 1, 0, 0, 0, 0),
(41313, 1, 0, 0, 0, 0),
(41314, 1, 0, 0, 0, 0),
(41315, 1, 0, 0, 0, 0),
(41316, 1, 0, 0, 0, 0),
(41199, 1, 0, 0, 0, 0),
(41198, 1, 0, 0, 0, 0),
(41170, 1, 0, 0, 0, 0);

DELETE FROM `areatrigger_create_properties` WHERE `Id` IN (41311,41313,41314,41315,41316,41199,41198,41170) AND `IsCustom` = 0;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`,
`ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `PositionalSoundKitId`,
`TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`,
`ShapeData5`, `ShapeData6`, `ShapeData7`, `Roll`, `Pitch`, `Yaw`, `TargetRoll`, `TargetPitch`, `TargetYaw`, `ScriptName`,
`VerifiedBuild`) VALUES
(41311, 0, 41311, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Consecration R1 (26573)
(41313, 0, 41313, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Consecration R2 (20116)
(41314, 0, 41314, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Consecration R3 (20922)
(41315, 0, 41315, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Consecration R4 (20923)
(41316, 0, 41316, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Consecration R5 (20924)
(41199, 0, 41199, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Hurricane R1 (16914)
(41198, 0, 41198, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0), -- Hurricane R2 (17401)
(41170, 0, 41170, 1, 0, 0, 0, 0, 0, -1, -1, 0, NULL, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0); -- Hurricane R3 (17402)

-- Hurricane: each tick casts the rank's damage spell of the client data at the area trigger (classic_spell_dru_hurricane).
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'classic_spell_dru_hurricane';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(16914, 'classic_spell_dru_hurricane'),
(17401, 'classic_spell_dru_hurricane'),
(17402, 'classic_spell_dru_hurricane');
