# Database Audit (M0)

**Engine:** MySQL Community **8.0.46** (Trinity Engine port **3308**; do not confuse with XAMPP 3306 / other stacks).  
**Databases in use:** `auth`, `characters`, `world`, `hotfixes`.

## Roles

| DB | Role |
|---|---|
| auth | accounts, `build_info`, `realmlist`, bnet-related |
| characters | persistent characters, inventory, … |
| world | spawns, quests, vendors, smart_scripts, … |
| hotfixes | Forever DB2-like overrides (e.g. ItemSparse) |

## Update process

1. Trinity **DBUpdater** applies `sql/updates` when `Updates.EnableDatabases` allows (worldserver conf sample: `15`).  
2. maintainer’s operational Forever SQL lives in **`sql/custom`** with dated files, README/NOTES, and **rollback** companions — preserve this discipline.  
3. Hotfix data often applied alongside world fixes (vendor Forever example 2026-11-04).

## Auth build evidence (SELECT)

`build_info` top builds include **70790**, 70755, 70753, 70707, …  
`realmlist` id **70** name Trinity **gamebuild 70790**.

## Risks

- Pending SQL on live (dberr_all5, vendor) applied to DB but worldserver only reloads on restart (maintainer controls restarts).  
- Residual DBErrors after cleanup still large; some categories intentionally kept (SmartAI refs, areatriggers needed, server-only spells, ambiguous vendors).  
- Never commit credentials; conf DatabaseInfo redacted in audits.
