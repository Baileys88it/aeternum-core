# ADR-001: Engine / Forever / Framework / Modules boundaries

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-001-engine-boundaries.md`](it/ADR-001-engine-boundaries.md)


## Problem
The Forever Trinity tree mixes core runtime, client-version deltas, and content scripts without explicit ownership boundaries, raising long-term coupling risk.

## Solution
Define logical boundaries mapped onto the **existing** tree (see `docs/architecture/en/TARGET_ARCHITECTURE.md`): **engine** (World, sockets, MapUpdater, DB pools), **forever** (single active client profile + adapters), **framework** (ScriptMgr + future SDK), **modules** (static first). No mass directory rename in M2–M3.

## Why
Preserves working systems (master prompt); enables localized client updates; matches the maintainer’s maintainability goal.

## Alternatives
1. Big-bang refolder to `engine/` — rejected (high risk, no player value).  
2. Keep status quo forever — rejected (70790 incident shows cost).

## Official / upstream sources
- TrinityCore layout (`src/server/game`, `src/server/database`) — upstream implementation.  
- CppCoreGuidelines (boundaries & ownership): https://github.com/isocpp/CppCoreGuidelines  
- Our conclusion: incremental façades > physical moves.

## Complexity / Perf / Maintenance
Med complexity; negligible perf; lowers maintenance if enforced in review.

## Tests
Architecture review checklist; forbid new build `#ifdef` outside forever/profile paths (Planned lint).

## Risks & rollback
Docs-only now — rollback = revert ADR. Future code façades behind flags.

## Decision (Accepted M2 — 2026-11-05)

Boundaries confirmed. Client profile **data** lives at repo-root `forever/client_profiles/<id>/`. C++ adapters later in `src/server/game/Forever/`. No scattered Forever version checks in core.
