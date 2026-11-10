# AETERNUM_RISK_REGISTER

**Date:** 2026-11-05 (Europe/Rome)  
**Scale:** Likelihood L/M/H; Impact L/M/H/Critical

| ID | Category | Risk | Likelihood | Impact | Mitigation | Residual | Owner decision? |
|---|---|---|---|---|---|---|---|
| R1 | Legal/product | GPL-2.0 vs “independent engine” expectations | H if marketing overclaims | Critical | Clarify goal (GPL arch independence vs clean-room); never claim license freedom via adapter alone | High until clarified | **Yes #1** |
| R2 | Compatibility | Refactors break Forever login/combat | M | Critical | Dual maintenance ADR-009; soak; Protocol-first; no big-bang | Med | Cutover #7 |
| R3 | Ops | Trinity Engine ↔ Aeternum drift | H | H | Cherry-pick/SQL pair discipline; no agent writes to Trinity | Med | Process |
| R4 | API | Draft SDK treated as stable | M | H | Draft banners; IE-ADR-006; freeze only when Trinity-free | Med | **Yes #4** |
| R5 | Protocol | Unknown `0x3D013D` + 380 UNHANDLED | M | M | Sniff Aeternum-only; triage Forever UX needs | Med | **Yes #9** |
| R6 | Docs | README denies Admin/Prometheus | H | M | Honesty commit | Low after fix | **Yes #10** |
| R7 | Ops | Soak/14d criteria incomplete | — (in progress) | H | Finish soak monitors; no early cutover | H until done | Reaffirm #7 |
| R8 | Security | Admin token leak / non-loopback bind | L if defaults held | H | Keep Enable=0 Bind=<redacted-ip>; env tokens; audit | L | No |
| R9 | Architecture | Premature Player/World façades | M if rushed | H | Delay IE-M4; require tests+soak | Med | Order #5 |
| R10 | Architecture | Paper modularity w/o include lint | H | M | IE-ADR-010 + local_ci optional in M1 | Med | Optional |
| R11 | Perf | Abstraction latency on map tick | M | M | Metric budgets; avoid chatty handle lookups | Med | No |
| R12 | QA | No automated full login | H | M | Checklist; future lab | Med | Optional |
| R13 | Product | Confuse operational M0–M9 with IE-M0–M9 | H | M | IE- naming everywhere | Low after docs | **Yes #2** |
| R14 | Technical | admin/host include World/DatabaseEnv | H | M | Snapshot ports IE-M3 | Med | No |
| R15 | Unknown | Mobius heritage unclear | — | L | Mark UNKNOWN; avoid claims | L | No |
| R16 | Protocol | ExtraInserts misconfig breaks create-char | M | H | Profile auto-apply + Catch2 gates + validator | Med | No |
| R17 | Persistence | Statement enum sprawl blocks ports | H | H | Vertical slice only; don’t boil ocean | H | No |
| R18 | Build | CMake still project(TrinityCore) | H | L/M brand | Rename only with owner OK | Med | Optional |
| R19 | Ops | Dual ports confuse launcher | M | H | Isolation profile; STATUS.bat | Med | No |
| R20 | QA | Thin M0 docs accepted as complete | M | H | Deepen pass + QA_REVIEW | Med | Review |

## Phase J — 10 architectural validation scenarios

| # | Scenario | Current feasibility | Required architectural changes | Missing interfaces | Required tests | Main risks |
|---|---|---|---|---|---|---|
| 1 | New client profile without gameplay logic edits | **PARTIAL YES** — ExtraInserts/profile path works; packet struct changes may still touch Packets/* | Packet adapter layer; forbid gameplay `#ifdef` build | `IClientProfile`, `IPacketCodec` | Profile goldens; create-char gate | Hidden struct drift |
| 2 | Packet serialization tested independently | **PARTIAL** — remap pure tests only | Extract codecs from WorldSession/Packets | `IPacketCodec` | Golden vectors per critical CMSG/SMSG | False greens |
| 3 | Module developed without Trinity headers | **NO** | HostServices + world/ops ports; host stays adapter | Trinity-free SDK + IWorldView/IHealth | Compile test w/o game includes | Feature starvation |
| 4 | World exposes entities without legacy class leak | **NO** | Handle/snapshot API | `EntityId`, `IWorldView` | Lifetime/generation tests | Dangling handles |
| 5 | Gameplay services tested without full server | **NO** (narrow units only) | Service ports + fakes | `ISpellService` et al. | Component tests | Behavior drift |
| 6 | Change DB implementation without gameplay rewrite | **NO** | Repositories | `I*Repository` | Fake vs MySQL parity | TX/perf bugs |
| 7 | Content packages validated independently | **PARTIAL** — ExtraInserts validator + drift; no full content package | Manifest+validator product | Data package API | CI validator | False trust |
| 8 | Replace one subsystem without destabilizing server | **NO** generally | Adapters + flags + soak | Per-domain ports | Soak + regression checklist | Instability |
| 9 | Detect incompatible modules on release | **PARTIAL** — sdkRange string + profile planner; not hard semver enforce | Negotiate on HostDiscover | Version policy | Negative load tests | Silent break |
| 10 | Legacy + new impl coexist during migration | **INFERRED possible** via adapters/flags | Dual routing in adapters | Kernel feature flags | A/B soak | Dual cost |

### Unnecessary complexity warnings
Avoid ECS rewrite, microservices, early `domains/` physical moves, C ABI before dynamic modules, mass UNHANDLED opcode work.

## Migration / rollback
- Code: git revert on aeternum/main  
- Prod: stay on Trinity Engine  
- DB: aeternum_* copies for experiments  
- Conf: `_buildlogs/clienttest_working_*` style backups  

## Reviewer disagreements
- Admin-first vs Protocol-first → **Protocol-first** (leverage); admin harden in M3  
- Freeze SDK 1.0 now → **No** until Trinity-free headers  
- Physical domains in M1 → **No**


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).


## Added risks from QA (R21–R23)

| ID | Category | Risk | Likelihood | Impact | Mitigation | Owner decision? |
|---|---|---|---|---|---|---|
| R21 | Compliance | `dep/*` third-party license inventory never done | H | M–H | Inventory pass (read-only) before any redistrib claim | Optional schedule |
| R22 | Compatibility/Ops | Launcher TLS/cert / LoginREST mismatch (2026-11-05 lesson) | M | Critical | Cert parity checklist in client-test + STATUS; conf backup before cert copy | No (ops runbook) |
| R23 | Protocol QA | No protocol golden/fuzz corpus → false green unit tests | H | H | IE-M2/M6 mandate goldens for CREATE/CHECK name at minimum | Tied to #9 / M2 |

## Phase J — measurable pass/fail criteria (QA P6 fix)

| # | Scenario | PASS when | FAIL when | Owner of test | Fixtures required |
|---|---|---|---|---|---|
| 1 | New profile w/o gameplay edits | Diff of `src/server/game/{Spells,Combat,Entities,AI}` empty for profile-only change; CREATE/CHECK Catch2 goldens green; ExtraInserts only under `forever/client_profiles/<id>/` | Any gameplay TU change required for ExtraInserts-only bump | Protocol eng + QA | Profile pack + Catch2 `[classic][profile]*` |
| 2 | Packet ser/deser independent | Codec unit test runs **without** linking `worldserver` game session; round-trips golden bytes | Test requires WorldSession/Player | Protocol eng | Golden `.bin` vectors |
| 3 | Module w/o Trinity headers | `cl /c` or CMake target for sample module includes **only** `aeternum/sdk/**` + contracts; link fails if Player.h pulled | Any `#include "Player.h"` etc. in module public/src | Module eng + CI script | sample_module + forbidden-include check |
| 4 | World entities w/o legacy leak | Public header for IWorldView compiles in TU that cannot see Player/Unit/Map headers (IWYU/forbidden list) | Entity API returns or takes Player* | World eng | contract compile test |
| 5 | Gameplay w/o full server | At least one gameplay service test uses fake ports; no process listen on 8085 | Test boots worldserver | Gameplay eng | fake ISpellService |
| 6 | Swap DB impl | CharacterSummary slice works with FakeCharacterRepository; same tests with MySQL adapter | Gameplay TUs include DatabaseEnv for that slice | Persist eng | fake + MySQL fixtures |
[redacted-sensitive-line]
| 8 | Replace subsystem safely | Feature flag runs adapter A vs B for one port through soak window with metric parity band | Flag flip crashes or desyncs players | Arch + QA | flag + soak checklist |
| 9 | Detect incompatible modules | HostDiscover refuses module with sdkRange outside host; exit/log fatal for critical | Incompatible module still Starts | Module eng | negative sdkRange fixture |
| 10 | Legacy+new coexist | Two impl behind flag; identical golden/soak for covered path | Cannot enable both builds without fork tree | Arch | dual adapter stubs |

**Independence PASS (overall):** scenarios **1, 3, and 9** PASS + active-wired opcode gates green (when Catch2 next authorized) + README honesty fixed. Scenarios 4–6/8 remaining NO ⇒ **do not claim independent engine**.
