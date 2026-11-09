# Backup / restore drill (M6 checklist)

**Italian:** [`../it/BACKUP_RESTORE_DRILL.md`](../it/BACKUP_RESTORE_DRILL.md)  
**Policy:** MySQL remains system of record (ADR-006). Drill on **copies** (`aeternum_*`), never destroy Trinity production dumps without maintainer.

## Minimum drill (operator)

1. **Snapshot** `auth` / `characters` / `world` / `hotfixes` (or the `aeternum_*` copies) with `mysqldump` or existing `_run/dbdumps` tooling.
2. Store dump under `<ENGINE_ROOT>\_run\dbdumps\` with timestamp; do **not** commit dumps.
3. **Restore** onto a scratch schema / confirm row counts vs pre-dump NOTES.
4. Record result in the milestone report (date, schemas, OK/FAIL).
5. Keep `sql/custom` apply+rollback pairs for any SQL landed this milestone (none required for M6 code-only).

## Acceptance

- One successful dump+restore on copies documented in `M6_REPORT.md`.
- Production Trinity untouched by the drill restore target.
