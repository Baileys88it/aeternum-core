# AETERNUM_EXTRACTION_PLAN

**Date:** 2026-11-05 (Europe/Rome)  
**Rule:** leverage over ease; invert dependencies before moving directories.

## Priority matrix (all 13 candidates)

| Rank | Candidate | Current implementation | Current dependencies | Complexity | Risk | Expected benefit | Required interfaces | Required tests | Trinity coupling | Milestone |
|---:|---|---|---|---|---|---|---|---|---|---|
| 1 | Client profiles | `forever/client_profiles/**`, `Forever/ClientProfile.*` | Config, FS, rapidjson, Log | M | L | Forever update isolation | `IClientProfile` | Catch2 profile gates + drift | MEDIUM | IE-M2 |
| 2 | Opcode remap registry | `ClassicOpcodeRemap.h`, `ClassicOpcodes.*`, `ClassicClientOpcodes.inc` | Protocol table, Forever inserts | M | M | Wire correctness across builds | `IOpcodeMap` | Remap unit + sniff goldens | HIGH | IE-M2 |
| 3 | Packet serialization boundary | `Server/Packets/**` (144), WorldSession handlers | Player/ObjectGuid/packet ops | H | H | Independent codec tests | `IPacketCodec` | Golden encode/decode | HIGH | IE-M2/M6 |
| 4 | Configuration management | ConfigSchema, conf.dist, CHECK_CONFIG.bat, config-validator | Conf files | L | L | Safer ops / boot | `EngineConfigSnapshot` | ConfigSchema tests | LOW | IE-M1 |
| 5 | Monitoring metrics sink | `common/Metric/*Local*` + hooks | Metric macros, FS | M | L | Ops independence | `IMetricProbe` | MetricLocal.cpp | MEDIUM | IE-M1/M7 |
| 6 | Admin / ops HTTP | `admin_monitoring/**` | World, DatabaseEnv, Metric, Beast, SDK | M | M | Trinity-free ops | `IHealth`, `IAuditLog` | AdminMonitoring tests + manual | MEDIUM | IE-M3 |
| 7 | Module SDK | `modules/sdk/**` draft 0.2 | Host ScriptMgr/World | M | M | Extension without forks | HostServices + IModule 1.0 path | ModuleSdkRuntime | MEDIUM | IE-M3 |
| 8 | Module lifecycle host | `ModuleHost.cpp`, ModuleLoader gen | ScriptMgr, World, profile | M | M | Deterministic start/stop | registry + planner (exists) | planner negatives | MEDIUM | IE-M3 |
| 9 | Engine Kernel lifecycle | `worldserver/Main.cpp`, STOP bat | World, sockets, Metric | M | M | Clear process ownership | `IEngineHost` | start/stop integration | HIGH | IE-M1 |
| 10 | Persistence abstractions | `database/**` + stmt enums | MySQL, all game | H | H | Fake repos / future storage | `ICharacterRepository`… | contract tests | CRITICAL | IE-M5 |
| 11 | Data loading | DB2Stores, ObjectMgr Load×119 | Files + DB | H | H | Content packages | `IDataCatalog` | load validators | HIGH | IE-M5 |
| 12 | World interfaces | MapManager, Map, grids | Entities, World | VH | VH | Handle-based access | `IWorldView` | lifetime tests + soak | CRITICAL | IE-M4 |
| 13 | Player/entity + gameplay services | Entities + Spells/Combat/Handlers | Everything | VH | VH | Test gameplay without full boot | EntityId commands + ISpellService | component + soak | CRITICAL | IE-M4/M9 |

VH=very high; H=high; M=medium; L=low.

## Sequencing rationale

Protocol (#1–3) maximizes Forever leverage with existing seams. Kernel/config/metrics (#4,5,9) enable enforcement and observability during extraction. Module/ops (#6–8) unlock extension without entity leaks. World/gameplay/data/persistence (#10–13) last because coupling is CRITICAL.

## Non-goals

Mass `src/server/game` renames; dynamic modules; MySQL replacement; implementing 380 UNHANDLED opcodes wholesale.


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).


## Detailed notes per top-5 leverage items

### 1–2 Protocol profile + remap
- **Seed code:** `Forever/ClientProfile.cpp`, `ClassicOpcodeRemap.h`, `ClassicOpcodes.cpp` Effective* inserts, `forever/client_profiles/1.60.1.70790`.  
- **First interfaces:** IClientProfile, IOpcodeMap.  
- **Exit criteria:** Scenario 1 PASS metrics (gameplay diff empty on ExtraInserts-only bump).  
- **Tests:** existing Catch2 tags + goldens.  

### 3 Packet codecs
- Start with CREATE_CHARACTER / CHECK_CHARACTER_NAME_AVAILABILITY only (manifest critical list).  
- Do not attempt 144 packet files in one milestone.

### 5–6 Metric + Admin
- Metric already testable; Admin needs IHealth snapshot to drop World.h/DatabaseEnv.h.  
- Docs-only README fix is **immediate owner decision #10**, not an extraction prerequisite but a trust prerequisite.

### 7–8 Module SDK + host
- Public headers nearly clean; **host** is the coupling. Move ModuleHost to adapter packaging conceptually (may stay path `modules/sdk/src` initially).

## Dependency inversion checklist (before any move)
- [ ] Contract header exists  
- [ ] Adapter implements it  
- [ ] At least one call site uses contract  
- [ ] Forbidden-include lint green on public path  
- [ ] Test proves scenario impacted  
- [ ] Rollback path documented  
