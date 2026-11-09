# AETERNUM_M1_IMPLEMENTATION_PLAN

**Status:** AUTHORIZED by owner 2026-11-05 — IE-M1 in progress (Protocol-first, small aeternum/api 1.0)
**Date:** 2026-11-05 (Europe/Rome)  
**Milestone:** IE-M1 Kernel & Contracts  
**Constraints:** read/write only after approval; never touch `<SERVER_ROOT>`; coordinate with soak CPU.

## Goals
1. Independent Engine contracts baseline  
2. IE-ADR accept/reject/amend recorded  
3. README honesty (Admin/Prometheus)  
4. Forbidden-include policy (+ optional lint)  
5. Zero intentional gameplay behavior change

## Non-goals
Gameplay refactors; opcode mass fixes; dynamic modules; cutover; CMake rename unless owner asks.

## Work items (concrete)

### WP0 — Decision gate (blocking)
- **Tasks:** collect answers to exec-summary decisions #1–10  
- **Files:** none in repo until answers exist; record in ``docs/independent_engine/`\DECISIONS_LOG.md` (create when answering)  
- **Tests:** n/a  
- **Accept:** all 10 answered or explicitly deferred with date  

### WP1 — README / roadmap honesty
- **Files (proposed):** `README.md`, `README.it.md`, optionally `docs/architecture/en/00_INDEX.md`  
- **Change:** Admin/Prometheus → Implemented (module, default off, loopback); distinguish Operational M0–M9 vs IE-M0–M9; link to ``docs/independent_engine/`\`  
- **Tests:** none  
- **Accept:** claims match tree  
- **Rollback:** git revert  

### WP2 — Contract sketches
- **If owner allows in-repo:** add additive `engine/contracts/aeternum/{kernel,protocol,world,persist,ops,sdk}/*.hpp` sketches from API design doc (no call sites required)  
- **Else:** keep in Aeternum_Docs only + short pointer markdown under `docs/architecture/en/IE_CONTRACTS_POINTER.md`  
- **Tests:** optional Catch2 compile TU that includes contracts and fails if `#include "Player.h"` appears in contracts tree  
- **Accept:** sketches exist; forbidden types absent  
- **Rollback:** delete folder / revert  

### WP3 — IE-ADR recording
- **Files (proposed):** `docs/adr/IE-ADR-001-….md` (Accepted/Rejected/Amended) **or** keep under Aeternum_Docs until owner wants repo ADRs  
- **Accept:** each IE-ADR-001..012 has outcome  
- **Rollback:** revert  

### WP4 — Forbidden include policy
- **Files:** `docs/architecture/en/FORBIDDEN_INCLUDES.md` + optional `contrib/aeternum/ci/check_forbidden_includes.ps1` hooked from `local_ci.ps1`  
- **Policy paths:** `engine/contracts/**`, `src/modules/sdk/include/**`  
- **Forbidden:** Player.h Unit.h Creature.h Map.h WorldSession.h WorldObject.h World.h (public sdk only — host excluded until M3)  
- **Accept:** script documents debt list for ModuleHost.cpp / admin_monitoring without failing them yet  
- **Rollback:** remove script hook  

### WP5 — Independence backlog snapshot
- **Files:** `docs/development/en/IE_COUPLING_BACKLOG.md` (or Docs folder) listing include offenders from M0 sampling  
- **Accept:** backlog exists with evidence paths  

### WP6 — Verification
- Run `contrib/aeternum/ci/local_ci.ps1` when owner schedules (avoid soak contention)  
- `CHECK_CONFIG.bat` on build output  
- Confirm Trinity ports unchanged; git status only intentional files  

## Acceptance criteria (M1 done)
- [ ] Decisions logged  
- [ ] IE-ADR outcomes recorded  
- [ ] README matches Admin/Prometheus reality  
- [ ] Contracts in approved location without forbidden types  
- [ ] Forbidden-include policy written; lint optional & non-flapping  
- [ ] Backlog snapshot exists  
- [ ] Catch2 not regressing when run  
- [ ] No Trinity production changes; soak not killed by M1 work  

## Rollback
Revert M1 commits; delete additive folders; no DB migrations in M1.

## Estimated complexity
Medium — docs/headers/policy. Should not require multi-day combat debugging if scope held.

## Exit → IE-M2
Use Forever ClientProfile + ClassicOpcodeRemap as seed for Protocol Engine ports.


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).


## Expanded migration / rollback / soak safety (QA P7)

### Before any M1 commit
1. Confirm Aeternum soak still desired running; if build needed, owner **go/no-go** (CPU contention).  
[redacted-sensitive-line]
3. `git status` clean or only intentional WIP.  
4. Trinity production: verify no file writes under `<SERVER_ROOT>` planned.

### During M1
- Touch only allowed paths (README, docs/adr IE-*, engine/contracts optional, contrib ci script, docs architecture).  
- No changes to Handlers/Spells/Entities/Maps/Opcodes tables.  
- No cert changes.  
- No DB migrations.

### Verification checklist
- [ ] `CHECK_CONFIG.bat` OK on Aeternum build output  
- [ ] `git diff --stat` limited to allowlist  
- [ ] Forbidden-include script dry-run shows expected debt (host/admin) without new violations in contracts/sdk include  
- [ ] If Catch2 authorized: compare assertion count ≥ prior 1841/110 or justify deltas  
- [ ] Launcher non-regression: **manual** login smoke on Aeternum **only if** owner schedules (not automatic in M1)  
- [ ] Trinity ports still serving (observe without killing)

### Rollback
1. `git revert` M1 commit(s) or `git reset --hard` to pre-M1 tag/SHA (only if not pushed / per owner).  
2. Restore conf from `m1_pre_*` snapshot if any conf touched (should be none).  
3. Delete additive `engine/contracts` if that was the failure mode.  
4. Re-run CHECK_CONFIG + STATUS.  
5. Confirm soak PIDs unchanged or restart **Aeternum only** with owner OK.

### Explicit non-regression criterion
Zero intended player-visible behavior change on Trinity production; Aeternum behavior change limited to docs/contracts/lint.


## Status 2026-11-05

ProtocolSessionBridge shipped; see IE_M1_STEP3_SESSION_BRIDGE.md. Flag Classic.ProtocolApiAdapter.Enable default ON.
