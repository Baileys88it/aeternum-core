# M7 Report -- Static Module SDK v0.2 (draft) + admin.monitoring consumer

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Commit:** `56d305cc36` -- *M7: static Module SDK v0.2 draft + admin.monitoring consumer* (21 files, +790/-81)  
**IT:** [`../it/M7_REPORT.md`](../it/M7_REPORT.md)

## Mapping

| Source | Label |
|---|---|
| Original roadmap | M7 = Advanced systems (AI/bots/events) -- **not** what this cycle delivered |
| Our sequence (standing) | **M7 = formalize static Module SDK** + migrate Admin onto `IModule` |
| Next | M8 harden admin/observability on the SDK |

## Delivered

| Item | Location / note |
|---|---|
| Static SDK host | `src/modules/sdk/` -- DISCOVER, LOAD, INIT, START, STOP, SHUTDOWN |
| Public vs internal | `aeternum/sdk/module.hpp`, `registry.hpp`, `version.h`; planner in `internal/` |
| Policy | Critical modules abort startup; non-critical disable+log; profile filter; dep topo + cycle detect |
| Admin consumer | `admin.monitoring` registered as non-critical `IModule` (SDK draft 0.2.0) |
| Docs EN+IT | `MODULE_SDK_GUIDE.md`, SDK README, architecture index links |
| API freeze | **Not** frozen (`AETERNUM_SDK_API_DRAFT`); no dynamic loading; no new deps |

## Build & tests

| Check | Result (from logs) |
|---|---|
| Catch2 `[sdk]` | **All tests passed (24 assertions in 7 test cases)** -- `_buildlogs/m7_sdk_tests.log` |
| RelWithDebInfo build | Completed (see `_buildlogs/build_m7.log`); exact wall-clock **not separately recorded** beyond log mtime |

## Live soak (Aeternum alt ports)

| Check | Result |
|---|---|
| Countdown 120s IT+EN | YES -- `_buildlogs/m7_countdown_evidence.log` + `stop_prod_m7.log`; `STOP_PROD_DONE 2026-11-05 04:05:32 Europe/Rome`; prod Server.log `Halting process...` |
| Prod before stop | bnet PID 20192 / world PID 14980 |
| Aeternum | bnet **19652** (ports 2119 / 18083), world **24704** (18085 / admin 9100) |
| Soak duration | **20.85 min** (`m7_soak.log`: start 04:11:47, stop 04:32:37 Europe/Rome) |
| Health | HTTP 200 `{"status":"ok"}` |
| Lifecycle evidence | `m7_soak_verify.log`: Module SDK DISCOVER (1 registered) -> admin.monitoring LOAD/INIT/START ok -> START complete (1 modules running); ExtraInserts auto-applied for profile `1.60.1.70790` |
| Crash | none observed in soak log |
| Prod after cycle | bnet PID **15408** / world PID **16116** (ports 1119 / 8085) -- from countdown evidence footer |

## Explicitly out of scope / not verified

- Dynamic / WASM module runtime
- Freezing the SDK API
- Real game-client login against Aeternum soak instance -- **not verified**
- 72-hour soak -- **not run**
- Gameplay / AI / events modules (MASTER M7) -- proposal only

## Actions

GitHub Actions left **disabled** (`enabled:false`); push to `aeternum/main` only.