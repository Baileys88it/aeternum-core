-- 2026_11_03_42_hotfixes_forever_bear_form_threat.sql filtered for this realm: records live 70888 pushes replaced, and our own hotfix records, are left out.
DELETE FROM `spell_effect` WHERE `ID` IN (700189);
INSERT INTO `spell_effect` (`ID`,`EffectAura`,`DifficultyID`,`EffectIndex`,`Effect`,`EffectAmplitude`,`EffectAttributes`,`EffectAuraPeriod`,`EffectBonusCoefficient`,`EffectChainAmplitude`,`EffectChainTargets`,`EffectItemType`,`EffectMechanic`,`EffectPointsPerResource`,`EffectPosFacing`,`EffectRealPointsPerLevel`,`EffectTriggerSpell`,`BonusCoefficientFromAP`,`PvpMultiplier`,`Coefficient`,`Variance`,`ResourceCoefficient`,`GroupSizeBasePointsCoefficient`,`EffectBasePoints`,`ScalingClass`,`TargetNodeGraph`,`EffectMiscValue1`,`EffectMiscValue2`,`EffectRadiusIndex1`,`EffectRadiusIndex2`,`EffectSpellClassMask1`,`EffectSpellClassMask2`,`EffectSpellClassMask3`,`EffectSpellClassMask4`,`ImplicitTarget1`,`ImplicitTarget2`,`SpellID`,`VerifiedBuild`) VALUES
(700189,10,0,0,6,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1,115,0,0,127,0,0,0,0,0,0,0,1,0,21178,70707);
DELETE FROM `hotfix_data` WHERE `Id`=2001700 OR (`TableHash`=4030871717 AND `RecordId`=700189);
INSERT INTO `hotfix_data` (`Id`,`UniqueId`,`TableHash`,`RecordId`,`Status`,`VerifiedBuild`) VALUES
(2001700,1397721700,4030871717,700189,1,70707);
