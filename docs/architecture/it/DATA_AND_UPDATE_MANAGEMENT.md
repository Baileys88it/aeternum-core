# Gestione dati e aggiornamenti (M6)

**Stato:** Implementato (M6 v0)  
**EN:** [`../en/DATA_AND_UPDATE_MANAGEMENT.md`](../en/DATA_AND_UPDATE_MANAGEMENT.md)  
**Roadmap:** MIGRATION_PLAN **M6 = Data & update management**  
**Priorità:** sistema aggiornamento client — il profilo è la fonte di verità degli ExtraInserts.

## Nota di mapping

Nella sequenza consegnata, il monitoring Admin è stato anticipato come **M5** (in MASTER era sotto M8).  
**Questo M6** segue l’etichetta roadmap e completa la fondazione M4: con profilo attivo non serve più allineare a mano gli ExtraInserts in conf.

## Consegnato

1. Auto-apply ExtraInserts dal pacchetto profilo (`Classic.ProfileExtraInserts.AutoApply`).
2. `Classic.ProfileDataRoot` per trovare `forever/client_profiles/`.
3. Controlli di coerenza ACTIVE ↔ conf e manifest ↔ `extra_inserts.conf`.
4. ClassicOpcodes usa gli ExtraInserts effettivi del profilo.
5. Catch2 `[classic][profile][m6][data]`
6. Checklist backup/restore: [`../../development/it/BACKUP_RESTORE_DRILL.md`](../../development/it/BACKUP_RESTORE_DRILL.md)

## Fuori da M6

Hot-swap senza restart, scrittura automatica del conf, UI hotfix SQL, endpoint admin in scrittura, nuove dipendenze.
