# ADR-009: Dual maintenance until Aeternum stable

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-009-dual-maintenance-trinity.md`](it/ADR-009-dual-maintenance-trinity.md)


## Problem
Evolving the engine while players rely on Trinity Engine risks outages if cut over early.

## Solution
Keep **Trinity Engine** (`<SERVER_ROOT>`) as production until explicit **Aeternum stable** criteria met (`MIGRATION_PLAN.md`). Aeternum agents must not write/restart Trinity. Port fixes Trinity→Aeternum via cherry-pick/SQL pairs; reverse promote only with maintainer approval.

## Why
maintainer decision; STABILITY first.

## Alternatives
Immediate cutover — rejected.  
Stop Trinity development — rejected (players).

## Sources
- Operational reality M0/M1.  
- Aeternum rollback culture.

## Complexity / Perf / Maintenance
Process overhead (dual); zero runtime perf impact; requires clear porting rules.

## Tests
Cutover checklist dry-run; backup drill.

## Risks & rollback
Drift between trees — mitigate with frequent ports; cutover rollback = stay on Trinity.

## Decision (Accepted M2 — 2026-11-05)

Dual maintenance confirmed. Soak before cutover: **72 hours** continuous clean run, then **14 days** real player use without crashes/regressions. Cutover **only** with maintainer’s explicit approval. Aeternum agents must not write/restart Trinity Engine.
