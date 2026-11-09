# Drill backup / restore (checklist M6)

**EN:** [`../en/BACKUP_RESTORE_DRILL.md`](../en/BACKUP_RESTORE_DRILL.md)  
**Policy:** MySQL resta system of record (ADR-006). Solo su copie `aeternum_*`.

## Drill minimo

1. Snapshot auth/characters/world/hotfixes (o copie aeternum_*).
2. Salvare in `<ENGINE_ROOT>\_run\dbdumps\` con timestamp (no commit).
3. Restore su schema scratch / verificare conteggi.
4. Annotare esito nel report milestone.
5. Coppie apply+rollback `sql/custom` se c’è SQL (M6 è code-only).

## Accettazione

Dump+restore ok documentato in `M6_REPORT.md`; Trinity produzione non è target del restore.
