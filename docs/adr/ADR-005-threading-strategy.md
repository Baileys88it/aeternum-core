# ADR-005: Preserve MapUpdater ownership; measure first

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-005-threading-strategy.md`](it/ADR-005-threading-strategy.md)


## Problem
Temptation to “modernize” threading without baselines risks races and regressions.

## Solution
Keep `MapUpdater` model (`MapUpdater.h`: queue + workers; one map tick ownership). Add measurement in M3 before any scheduler redesign. No thread-per-module for Admin pilot.

## Why
Preserve what works; optimize what matters (measured).

## Alternatives
Actor framework rewrite — rejected now.  
Taskflow/etc. — investigate only with proof.

## Sources
- In-tree `MapUpdater`, `World::Update`.  
- CppCoreGuidelines concurrency sections.  
- Our conclusion: instrument first.

## Complexity / Perf / Maintenance
Low for M1–M3; perf unknown until measured; maintenance stable.

## Tests
Shutdown deactivate; future stress + sanitizers.

## Risks & rollback
Any experimental scheduler behind flag; default = current.

## Decision (Accepted M2 — 2026-11-05)

Preserve MapUpdater ownership; measure first (see ADR-007). No threading redesign in M2.
