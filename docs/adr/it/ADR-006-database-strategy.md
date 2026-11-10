# ADR-006: MySQL remains system of record

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-006-database-strategy.md`](../ADR-006-database-strategy.md)


## Problema
Rischio di cambiare storage senza necessità.

## Soluzione
MySQL resta SoR; DBUpdater + sql/custom; no ClickHouse gameplay.

## Perché
Produzione già su MySQL 8.0.46.

## Alternative
MariaDB possibile; Postgres no; ClickHouse solo analytics.

## Fonti
dev.mysql.com; DatabaseWorkerPool; audit M0.

## Complessità / Perf / Manutenzione
Bassa; status quo; disciplina rollback.

## Test
Apply/rollback; restore drill.

## Rischi & rollback
Rollback SQL + dump.

## Decisione (Accettato M2 — 2026-11-05)

MySQL resta system of record. Nessun cambio DB in M2.
