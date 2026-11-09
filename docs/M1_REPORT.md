# Aeternum M1 Report — Architecture & ADRs

**Date:** 2026-11-04 (Europe/Rome)  
**Status:** Design complete — **STOP for approval** (M2 not started)  
**IT:** [`M1_REPORT.it.md`](M1_REPORT.it.md)  
**Parent of docs commit:** will be recorded at push time on `aeternum/main`

## Summary

M1 delivers the target architecture mapped onto the real TrinityCore Forever tree, a single-active-client-profile compatibility design (with 70790 case study and 12-step update workflow), static-first module/SDK design with **Administration & Monitoring** as pilot, DB/threading/observability strategies, launcher freeze contract, dual-maintenance/cutover plan, and **ADR-001…009** (status **Proposed**) in EN+IT.

No source/CMake/DB/dependency changes.

## Key decisions captured

| # | Decision |
|---|---|
| 1 | One Forever client build / one active profile; switch = replace profile |
| 2 | Trinity Engine stays production until Aeternum stable |
| 3 | Priority #1 = client-update architecture + Module SDK |
| 4 | First SDK module = Admin & Monitoring (localhost, RBAC, no arbitrary remote cmds) |
| 5 | MySQL remains SoR; MapUpdater ownership preserved; measure before optimize |
| 6 | Prefer in-tree Metric before Tracy/OTel/Prometheus in prod |
| 7 | WASM / custom allocators / native dynamic = deferred with proof criteria |
| 8 | modern ≠ better; EN+IT docs mandatory |

## Decisions requiring maintainer approval (accept ADRs)

Accept or amend ADR-001…009 (especially 002, 004, 008, 009) and the cutover checklist in `MIGRATION_PLAN.md`.

## M2 proposal (scope only — NOT started)

**Objective:** Engineering foundations that enable M4/M5 without changing gameplay behavior.

Candidate scope (to approve before work):

1. Conf validation helper for ExtraInserts (no BOM; reject inline comment tokens) — docs/tooling or tests.  
2. Expand Catch2 tests skeleton for opcode remap fixtures (may be empty fixtures first).  
3. Document local build/quality checklist; optional clang-tidy/cppcheck **notes** (install only if maintainer approves tooling).  
4. Skeleton folder `forever/client_profiles/70790/` **documentation-only** copy of current inserts/expectations (still no runtime loader).  
5. Update README milestone table M1=done / M2=next.

**Out of M2:** runtime profile loader, SDK code, Admin HTTP server, Tracy/OTel integration, Trinity touches, CMake refactors.

## Open questions

1. Exact on-disk root for profiles: `forever/client_profiles` vs later `forever/`?  
2. Admin HTTP library choice (defer to M5 addendum)?  
3. Soak duration before Aeternum primary?  
4. Should Metric use existing Influx endpoint or localhost push for pilot?

## Approval gates for M2

1. Accept M1 architecture set + ADR-001…009 (or list amendments).  
2. Confirm M2 scope above (add/remove items).  
3. Reconfirm: no Trinity restarts; Actions stay disabled; launcher contracts frozen.
