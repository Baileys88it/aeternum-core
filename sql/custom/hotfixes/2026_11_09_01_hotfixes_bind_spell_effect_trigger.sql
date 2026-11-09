-- Forever 1.60.1.70862: Bind (spell 3286) effect index 1 is SPELL_EFFECT_TRIGGER_SPELL pointing at
-- spell 368736, but that spell does not exist in client SpellName/SpellMisc/Spell (ghost reference).
-- Trinity logs ERROR Spell::EffectTriggerSpell and returns. Homebind (effect 0) and Hearthstone item
-- 6948 (NPCHandler::SendBindPoint) already work without the trigger. Zero EffectTriggerSpell to silence
-- the error. Safe to run again.
DELETE FROM `spell_effect` WHERE `ID` = 680892 AND `VerifiedBuild` = 70862;
INSERT INTO `spell_effect` (
  `ID`,`EffectAura`,`DifficultyID`,`EffectIndex`,`Effect`,`EffectAmplitude`,`EffectAttributes`,`EffectAuraPeriod`,
  `EffectBonusCoefficient`,`EffectChainAmplitude`,`EffectChainTargets`,`EffectItemType`,`EffectMechanic`,
  `EffectPointsPerResource`,`EffectPosFacing`,`EffectRealPointsPerLevel`,`EffectTriggerSpell`,`BonusCoefficientFromAP`,
  `PvpMultiplier`,`Coefficient`,`Variance`,`ResourceCoefficient`,`GroupSizeBasePointsCoefficient`,`EffectBasePoints`,
  `ScalingClass`,`TargetNodeGraph`,`EffectMiscValue1`,`EffectMiscValue2`,`EffectRadiusIndex1`,`EffectRadiusIndex2`,
  `EffectSpellClassMask1`,`EffectSpellClassMask2`,`EffectSpellClassMask3`,`EffectSpellClassMask4`,
  `ImplicitTarget1`,`ImplicitTarget2`,`SpellID`,`VerifiedBuild`
) VALUES (
  680892,0,0,1,64,0,0,0,
  0,1,0,0,0,
  0,0,0,0,0,
  1,0,0,0,1,0,
  0,0,0,0,0,0,
  0,0,0,0,
  25,0,3286,70862
);

DELETE FROM `hotfix_data` WHERE `TableHash` = 4030871717 AND `RecordId` = 680892;
INSERT INTO `hotfix_data` (`Id`,`UniqueId`,`TableHash`,`RecordId`,`Status`,`VerifiedBuild`) VALUES
(2110001,1464300001,4030871717,680892,1,70862);
