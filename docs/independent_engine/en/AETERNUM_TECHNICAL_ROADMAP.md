# AETERNUM_TECHNICAL_ROADMAP (IE-M0 … IE-M9)

**Date:** 2026-11-05 (Europe/Rome)  
**Naming:** **IE-Mn** = Independent Engine track. Distinct from completed **operational** M0–M9 under `docs/development/`.  
**Deadlines:** none fabricated — complexity only (L/M/H/VH).

| Milestone | Class | Complexity | Depends on |
|---|---|---|---|
| IE-M0 Discovery | Essential | M | — |
| IE-M1 Kernel & Contracts | Essential | M | Owner decisions |
| IE-M2 Protocol Engine | Essential | H | IE-M1 |
| IE-M3 Module Platform | Essential | H | IE-M1, partial M2 |
| IE-M4 World & Gameplay Boundaries | Essential | VH | IE-M3 |
| IE-M5 Data & Persistence | Essential | VH | IE-M4 started |
| IE-M6 Compatibility & Test Lab | Essential | H | IE-M2 |
| IE-M7 Performance Intelligence | Optional | M | Metric exists |
| IE-M8 Automated Evolution | Optional/Research | H | IE-M6 |
| IE-M9 Progressive Legacy Independence | Essential long-term | VH | prior |

---

## IE-M0 — Architecture Discovery
- **Objectives:** Evidence map; target arch; ADRs proposed; M1 plan; QA scenarios.  
- **Required work:** read-only inspection; docs outside repo.  
- **Dependencies:** none.  
- **Deliverables:** 11 markdown files in ``docs/independent_engine/`\`.  
- **Risks:** thin evidence → false confidence (mitigated by deepen pass).  
- **Complexity:** M.  
- **Testing:** n/a (docs).  
- **Acceptance:** Owner acknowledges; uninspected list honest; decisions listed undecided.  
- **Prerequisites:** clean tree; soak/Trinity untouched.  
- **Essential/Optional/Research:** Essential.

## IE-M1 — Independent Kernel and Contracts
- **Objectives:** Contract sketches; IE-ADR outcomes; README honesty; forbidden-include policy; independence backlog.  
- **Required work:** see `AETERNUM_M1_IMPLEMENTATION_PLAN.md`.  
- **Dependencies:** owner decisions #1–10.  
- **Deliverables:** contracts location; ADR records; README fix; lint plan.  
- **Risks:** paper-only; accidental gameplay churn.  
- **Complexity:** M.  
- **Testing:** Catch2 unchanged green; sketches compile without Trinity entity headers.  
- **Acceptance:** checklist in M1 plan.  
- **Prerequisites:** IE-M0 accepted.  
- **Essential:** contracts+ADRs+honesty. **Optional:** local_ci include lint. **Research:** CMake project rename.

## IE-M2 — Independent Protocol Engine
- **Objectives:** `IClientProfile`/`IOpcodeMap` wired; reduce Packets ad-hoc Forever edits; goldens for CREATE/CHECK name.  
- **Required work:** façade over ClientProfile+ClassicOpcodes; begin codec for critical messages.  
- **Dependencies:** IE-M1.  
- **Deliverables:** protocol adapter TUs; tests; profile docs updated.  
- **Risks:** wire regressions (char create history).  
- **Complexity:** H.  
- **Testing:** Catch2 gates + manual launcher login on Aeternum.  
- **Acceptance:** ExtraInserts/profile change without gameplay TU edits for covered set; login OK.  
- **Essential:** profile+opcode ports. **Optional:** broad codecs. **Research:** auto sniff importer.

## IE-M3 — Module Platform
- **Objectives:** Trinity-free public SDK; HostServices; admin uses IHealth snapshots.  
- **Required work:** split host adapter; rewrite admin reads off World.h where possible.  
- **Dependencies:** IE-M1; protocol profile id stable.  
- **Deliverables:** SDK headers without forbidden types; sample module; tests.  
- **Risks:** feature gaps; admin incomplete snapshots.  
- **Complexity:** H.  
- **Testing:** compile sample module without game includes; ModuleSdkRuntime expanded.  
- **Acceptance:** public include lint clean for sdk/include; admin still default off/loopback.  
- **Essential:** Trinity-free SDK+host services. **Optional:** more events. **Research:** dynamic load (defer).

## IE-M4 — World and Gameplay Boundaries
- **Objectives:** EntityId/MapId IWorldView; first gameplay service.  
- **Required work:** adapter from ObjectAccessor/Map; lifetime rules; one SpellService slice.  
- **Dependencies:** IE-M3.  
- **Deliverables:** world view API; module read demo; soak comparison.  
- **Risks:** dangling handles; perf.  
- **Complexity:** VH.  
- **Testing:** unit lifetime + 72h-class soak.  
- **Acceptance:** module reads entity snapshot without Player.h; behavior parity.  
- **Essential:** read view. **Optional:** command bus. **Research:** ECS-like storage (likely reject).

## IE-M5 — Data and Persistence Independence
- **Objectives:** IDataCatalog + one repository vertical slice.  
- **Required work:** CharacterSummary list/load via port; MySQL adapter; fake repo tests.  
- **Dependencies:** IE-M4 ports pattern.  
- **Deliverables:** persist/data headers + adapters.  
- **Risks:** TX/perf; call-site volume.  
- **Complexity:** VH.  
- **Testing:** fake vs MySQL parity tests.  
- **Acceptance:** slice usable by module/ops without DatabaseEnv.h.  
- **Essential:** one slice. **Optional:** more repos. **Research:** non-MySQL backend.

## IE-M6 — Compatibility and Test Lab
- **Objectives:** opcode lab, profile diff, goldens; resolve `0x3D013D`.  
- **Required work:** tools under tools/*-lab; capture discipline on Aeternum only.  
- **Dependencies:** IE-M2.  
- **Deliverables:** lab scripts + reports.  
- **Risks:** noisy captures.  
- **Complexity:** H.  
- **Testing:** goldens CI via local_ci.  
- **Acceptance:** unknown opcode disposition documented; lab runnable offline.  
- **Essential:** lab+unknown opcode. **Optional:** fuzzing. **Research:** auto ExtraInserts suggest.

## IE-M7 — Performance Intelligence
- **Objectives:** richer hotspot insights from Metric; alert thresholds.  
- **Dependencies:** Metric sink (done).  
- **Complexity:** M. **Class:** Optional.  
- **Acceptance:** actionable reports validated on soak data.  
- **Risks:** metric cardinality explosions (mitigated by MaxSeries).

## IE-M8 — Automated Evolution
- **Objectives:** assist profile upgrades; module compatibility reports.  
- **Class:** Optional/Research. **Complexity:** H.  
- **Dependencies:** IE-M6.  
- **Risks:** false automation trust.

## IE-M9 — Progressive Legacy Independence
- **Objectives:** shrink Trinity surface behind adapters; measure include coupling reduction.  
- **Class:** Essential long-term. **Complexity:** VH.  
- **Acceptance:** measured coupling ↓; Forever parity; cutover still ADR-009 owner gate.  
- **Note:** does not itself change GPL obligations.


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).
