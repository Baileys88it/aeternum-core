# AETERNUM_REPOSITORY_DISCOVERY_REPORT

**Phase:** M0 deepened discovery  
**Date:** 2026-11-05 (Europe/Rome)  
**Repo:** ``<repo-root>``  
**Branch/HEAD:** `aeternum/main` / `9894b2e1c275eafa56547dc6b6a40e22389fb5b2`  
**Build dir:** ``<aeternum-root>`\build` (not modified)  
**Working tree:** clean throughout investigation

## Method

[redacted-sensitive-line]

## Structure (CONFIRMED)

Root: CMake `project(TrinityCore)`, GPL-2.0 `COPYING`, `src/{common,server,modules,tools,genrev}`, `dep/`, `sql/`, `forever/`, `docs/`, `tests/`, `tools/`, `contrib/`, `(operator files, not included)`.

### Ownership buckets

| Bucket | Paths / signals | Evidence |
|---|---|---|
| TrinityCore / Forever snapshots | Most of `src/server/game`, `dep`, base sql | History before `b3c55465c8`; CMake name |
| Aeternum patches | sql/custom, classic opcode path, wf-70707 snapshots | git log pre-Aeternum |
| Aeternum engineering | Forever/, modules/sdk, admin_monitoring, Metric local, ConfigSchema, CrashDumpBudget, contrib/aeternum, client_profiles, config-validator, ADR docs | `b3c55465c8..HEAD` = **20 commits, 194 files, +10700/−168** |
| Third-party | `dep/*` | PackageList |
| Unfinished | SDK draft, WASM deferred, IE contracts absent | version.h `AETERNUM_SDK_API_DRAFT` |

**Mobius:** named in brief/docs lineage; **no** `mobius/` directory — UNKNOWN as separable codebase.

## Build

- SERVERS → database, proto, shared, bnetserver, game, scripts, **modules**, worldserver.  
- `contrib/aeternum` generates START/STOP/STATUS/CHECK_CONFIG bats + README into RelWithDebInfo.  
- Catch2 tests: 31 `.cpp` under `tests/`.  
- GitHub Actions disabled; `contrib/aeternum/ci/local_ci.ps1` exists.

## Dependencies

Boost (Asio/Beast), OpenSSL, MySQL, protobuf, fmt, rapidjson, zlib, CascLib, recast, SFMT, argon2, gSOAP, jemalloc (plat), …

## Custom Aeternum capabilities (verified)

1. Client profile ExtraInserts auto-apply + drift check + Catch2 gates  
2. Metric local sink ~75MB + daily/human reports  
3. Admin localhost HTTP + Prometheus + audit (default off)  
4. Module SDK static lifecycle draft + planner  
5. Runtime bats, local CI, opcode coverage report, client test report  
6. ConfigSchema + CrashDumpBudget + Aeternum banner  

## Test infrastructure

| Layer | Present? | Notes |
|---|---|---|
| Unit Catch2 Aeternum | Yes | Metric, profile, remap, module planner, admin unit |
| Unit Catch2 Trinity game | Yes | many tests/game |
| Protocol goldens | No | gap |
| Full login CI | No | manual 2026-11-05 success |
| Soak | In progress | 72h started ~10:54 Europe/Rome |

## SQL layout

updates master: auth5 / char3 / world33 / hotfixes1; custom much larger (world 176). Live updates table: UNKNOWN (no credentialed SELECT). Log shows all four DBs “up-to-date” with archived/new counts (see Dependency Analysis).

## Risks (discovery)

Identity still TrinityCore; GPL vs independence language; dual maintenance; doc drift; CRITICAL type coupling; opcode unknowns.

## Uninspected / incomplete (honest)

1. Full clang include graph / IWYU  
2. scripts/ tree dependency map  
3. bnet serviceHash numeric table  
4. `updates` table row diff vs files  
5. Deep audit of operator tooling (not included)  
6. Third-party license texts beyond COPYING  
7. Handler semantic audit (48 files)  
8. Soak metric file contents (not opened)  
9. Admin HTTP penetration test  
10. Mobius upstream delta  
11. Hotfix DB2 coverage completeness  
12. Security review of launcher REST token flows (code listed only)


## Detailed Aeternum file inventory (CONFIRMED paths)

### forever/client_profiles/1.60.1.70790/
- `ACTIVE` pointer at parent dir
- `manifest.json` / `manifest.yaml` — build 70790, ExtraInserts lists, critical opcodes CREATE/CHECK name
- `extra_inserts.conf` — Client `0x440071,0x440072` Server `0x470125`
- `opcodes_table.md`, `packet_notes.md`, `build_auth_keys.md`
- `seams/opcode_seams.md`, `seams/hotfix_seams.md`
- `CASE_STUDIES/70755_to_70790_create_char.md`
- `tests/README.md`

### src/server/game/Forever/
- `ClientProfile.h/.cpp` (~414 LOC cpp)
- `ClassicOpcodeRemap.h` (pure)

### src/modules/sdk/
- Public: `module.hpp`, `registry.hpp`, `version.h` (0.2.0 draft)
- Internal: `host_hooks.hpp`, `static_runtime.hpp` (planner)
- Host: `ModuleHost.cpp` (~221 LOC)

### src/modules/admin_monitoring/
- AdminHttpServer, AdminAuth, AdminAuthRateLimit, AdminBind, AdminPrometheus, AdminAuditLog, AdminMonitoringScripts, conf.dist

### src/common/Metric/ (Aeternum-extended)
- Metric.cpp/h (extended), MetricAggregator, MetricLocalStore, MetricDailySummary, MetricHumanReport, MetricErrorCollector

### contrib/aeternum/
- AttachRuntimeScripts.cmake, bat templates, local_ci.ps1, create_aeternum_dbs.sql, set_realm_name_aeternum.sql

### tools/config-validator/
- validate_extra_inserts.py + tests

### tests (Aeternum-focused)
- MetricLocal, ConfigSchema, CrashDumpBudget
- ActiveClientProfileGate, ClassicExtraInsertsRemap, ProfileDataUpdate
- ModuleSdkRuntime, AdminMonitoring

## Recent commit themes (aeternum/main tip)

9894b2e1 Local CI + release packaging proposal  
2c9252b5 Opcode coverage report  
f591d75d Runtime bats, realm Aeternum, legacy.dll  
113d3aac Client test report LoginREST/SkipBuild  
af870368 M8 Catch2 green evidence  
a8bf025a Branding banner  
67ebac57 / 3665019d / 56d305cc M9/M8/M7 ops+SDK+admin  
c85b09c2 / 83432520 / 50f16350 M6/M5/M4 profiles+admin+SDK  
396a3db4 M3 metric sink  
f941cf26 M2 foundations ADR accept  
4cd2143f / b3c55465 M1/M0 docs  

## Contradiction log

| Claim | Location | Reality |
|---|---|---|
| Admin HTTP / Prometheus not in tree | README.md | admin_monitoring implements both |
| Module SDK planned only | README table partially | Draft headers exist; still unfinished |
| client_profiles does not exist yet | older TARGET_ARCHITECTURE note | Now exists (doc partially stale) |


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).


## Evidence artifacts located this deepen/QA pass

| Artifact | Absolute path | Trust |
|---|---|---|
| Opcode coverage JSON | ``<buildlogs>`` | Exists; **numbers stale** (1031/379) vs recount 1032/380 |
| Opcode analyzer script | ``<buildlogs>`` | Present |
| Catch2 full green log (historical) | ``<archive>`` | 1841 assertions / 110 cases; **not re-run** in M0 |
| Include sample dump | ``docs/independent_engine/`\_include_sample.txt` | Generated M0 |
| Soak Server.log (updates up-to-date lines) | ``<aeternum-root>`\build\bin\RelWithDebInfo\logs\Server.log` | PARTIAL schema evidence |

## Recount methodology (opcodes)
PowerShell/regex over `DEFINE_HANDLER(CMSG_` lines in `Opcodes.cpp`; classify by STATUS_*; among non-UNHANDLED/IGNORED/NEVER, subtract Handle_NULL (2) → active-wired 626. Matches QA Python parse.
