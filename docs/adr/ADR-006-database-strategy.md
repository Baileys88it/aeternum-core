# ADR-006: MySQL remains system of record

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-006-database-strategy.md`](it/ADR-006-database-strategy.md)


## Problem
Pressure to “modernize” storage could fork persistence away from working Forever DBs.

## Solution
Keep **MySQL** (`auth`, `characters`, `world`, `hotfixes`) as authoritative gameplay store. Continue `DBUpdater` + `sql/custom` (+rollbacks). No ClickHouse for gameplay. Module SQL limited & reviewed.

## Why
Evidence: production Forever already on MySQL 8.0.46; master prompt forbids unjustified DB engines.

## Alternatives
MariaDB drop-in — possible later, not required.  
Postgres rewrite — rejected.  
ClickHouse analytics — only if separate justified use case.

## Sources
- https://dev.mysql.com/doc/  
- In-tree `DatabaseWorkerPool`, `DBUpdater`  
- M0 `DATABASE_AUDIT.md`

## Complexity / Perf / Maintenance
Low decision cost; perf = status quo until measured; maintenance = keep rollback discipline.

## Tests
Apply/rollback pairs on copies; backup restore drill (M6).

## Risks & rollback
Failed migration → rollback SQL + dump restore.

## Decision (Accepted M2 — 2026-11-05)

MySQL remains system of record. No DB strategy change in M2.
