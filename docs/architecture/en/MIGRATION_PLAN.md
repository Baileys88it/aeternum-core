# Migration Plan (M1) — Dual maintenance & cutover

**Status:** Accepted (M2)  
**Italian:** [`../it/MIGRATION_PLAN.md`](../it/MIGRATION_PLAN.md)

## Dual maintenance

| Tree | Role |
|---|---|
| **Trinity Engine** `<SERVER_ROOT>` | Production Forever server — **never** restarted/written by Aeternum agents without maintainer |
| **Aeternum** `<ENGINE_ROOT>\source` | Evolution / design / future runtime |

maintainer runs both until **Aeternum stable**.

## Porting Trinity → Aeternum

1. Prefer cherry-pick / replay of Aeternum commits onto `aeternum/main`.  
2. SQL fixes: copy `sql/custom` pair (apply+rollback) + NOTES.  
3. Conf-only Forever tweaks: land in **client profile** artifacts when M4 exists.  
4. Never push Aeternum changes to `advocaite`.  
5. Promote Aeternum → Trinity only with explicit maintainer approval + backup.

## Milestone sequence (scope reminders)

| M | Focus |
|---|---|
| M2 | Foundations: local quality, tests harness, conf validation helpers — **no** big features |
| M3 | Measurement |
| M4 | Client profile architecture implementation |
| M5 | Module SDK + Admin pilot static module |
| M6 | Data/update management |
| M7 | Advanced gameplay modules (as approved) |
| M8 | Harden admin/observability |
| M9 | Release engineering |

## Cutover criteria — “Aeternum stable” (Proposed checklist)

1. Same launcher contracts green (1119/8085/70790 or successor profile).  
2. Character create + login + who/party/arena smoke automated or checklist-signed.  
3. Client profile flip procedure exercised once successfully.  
4. Admin module localhost metrics trusted by maintainer.  
5. Parallel soak: **72h clean + 14 days players** (Accepted M2) with no worse critical bugs than Trinity.  
6. Backup/restore drill documented.  
7. Explicit maintainer go-ahead to make Aeternum primary.

Until then Trinity remains source of player truth.


## Soak period (Accepted M2 — 2026-11-05)

Before cutover eligibility:

1. **72 hours** continuous clean run (no crashes / no critical regressions vs baseline).  
2. Then **14 days** real use with players without crashes/regressions.  
3. Cutover **only** with maintainer’s **explicit** approval.
