# Database Architecture (M1)

**Status:** Proposed  
**Italian:** [`../it/DATABASE_ARCHITECTURE.md`](../it/DATABASE_ARCHITECTURE.md)

## Decision

**MySQL remains authoritative** for gameplay persistence (`auth`, `characters`, `world`, `hotfixes`).  
ClickHouse / other engines: **rejected** for gameplay (M0); analytics only if later justified.

## Current pillars (Implemented)

| Piece | Path / evidence |
|---|---|
| Async pools | `DatabaseWorkerPool` |
| Updates | `DBUpdater` + `sql/updates` |
| Forever ops SQL | `sql/custom` + rollback companions |
| Prod engine | MySQL **8.0.46** (Trinity Engine :3308) — reference env |

## Migration strategy (Proposed)

1. Prefer additive SQL with **paired rollback** (existing Aeternum discipline).  
2. Schema changes require: backup point, apply script, verify query, rollback script tested on copy.  
3. Profile flips (client build) may need coordinated `build_info` / hotfix rows in same release notes.  
4. Aeternum must not invent a second migration runner until ADR revises DBUpdater — extend, don’t duplicate.

## Backup / recovery

- Logical dumps before risky applies (auth/world/characters/hotfixes).  
- Binary/worldserver conf backups separate from SQL.  
- Recovery: restore dump → re-apply known-good fix set; document in M6.

## Module DB access

Admin pilot: **read-mostly** metrics from process + optional SELECT on non-sensitive aggregates; no arbitrary SQL from remote clients.
