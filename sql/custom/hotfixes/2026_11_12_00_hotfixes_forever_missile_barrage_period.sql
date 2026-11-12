-- Forever 1.60.1.70888: Missile Barrage 400589 effect 1 is a DUMMY -500 in the client data; restore the period modifier
-- (ADD_FLAT_MODIFIER, SpellModOp Period 19, Arcane Missiles 0x800) so missiles fire every 0.5 s. Sent to the client as a hotfix.
DELETE FROM `spell_effect` WHERE `ID` = 1045767 AND `VerifiedBuild` = 70862;
INSERT INTO `spell_effect` (`ID`,`EffectAura`,`DifficultyID`,`EffectIndex`,`Effect`,`EffectAmplitude`,`EffectAttributes`,`EffectAuraPeriod`,
  `EffectBonusCoefficient`,`EffectChainAmplitude`,`EffectChainTargets`,`EffectItemType`,`EffectMechanic`,`EffectPointsPerResource`,`EffectPosFacing`,
  `EffectRealPointsPerLevel`,`EffectTriggerSpell`,`BonusCoefficientFromAP`,`PvpMultiplier`,`Coefficient`,`Variance`,`ResourceCoefficient`,
  `GroupSizeBasePointsCoefficient`,`EffectBasePoints`,`ScalingClass`,`TargetNodeGraph`,`EffectMiscValue1`,`EffectMiscValue2`,`EffectRadiusIndex1`,
  `EffectRadiusIndex2`,`EffectSpellClassMask1`,`EffectSpellClassMask2`,`EffectSpellClassMask3`,`EffectSpellClassMask4`,`ImplicitTarget1`,`ImplicitTarget2`,
  `SpellID`,`VerifiedBuild`) VALUES
(1045767,107,0,1,6,0,0,0, 0,1,0,0,0,0,0, 0,0,0,1,0,0,0, 1,-500,0,0,19,0,0, 0,2048,0,0,0,1,0, 400589,70862);
DELETE FROM `hotfix_data` WHERE `TableHash` = 4030871717 AND `RecordId` = 1045767;
INSERT INTO `hotfix_data` (`Id`,`UniqueId`,`TableHash`,`RecordId`,`Status`,`VerifiedBuild`) VALUES (2110002,1464300002,4030871717,1045767,1,70862);
