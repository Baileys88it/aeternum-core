# M1 Implementation Notes (kernel block 2 - wiring and base cleanup)

**Branch:** `feat/m1-kernel` (local commits; no push for this block)  
**Date:** 2026-11-05 (Europe/Rome)  
**Scope:** P-a, P-c (documentation + compile-time guards), D9 boot wiring of `module.manifest.json`.

## 1. P-a - ScriptReloadMgr 500 ms tick removed from static builds

| Item | Before | After |
|---|---|---|
| Timer setup | `World.cpp:2040` `m_timers[WUPDATE_CHECK_FILECHANGES].SetInterval(500)` | wrapped in `#ifdef TRINITY_API_USE_DYNAMIC_LINKING` |
| Tick | `World.cpp:2319-2325` `sScriptReloadMgr->Update()` every 500 ms | wrapped in `#ifdef TRINITY_API_USE_DYNAMIC_LINKING` |

With `WITH_DYNAMIC_LINKING=OFF` (Aeternum default, `build_iem1`) the macro is not defined, so the timer and the empty
virtual call are compiled out: zero calls, zero timer housekeeping on the world thread. The guard uses the same macro as
`ScriptReloadMgr.cpp` (stub branch `#ifndef TRINITY_API_USE_DYNAMIC_LINKING`), so the inherited TrinityCore HotSwap
branch keeps working if somebody builds with dynamic linking (approach prescribed by `DYNAMIC_LOADING_AND_HOT_RELOAD.md` 10.1).
`sScriptMgr->SyncScripts()` is unchanged.

## 2. P-c - zero-heap rule on `OnPacketReceive`

`WorldSession::Update` (`WorldSession.cpp`, next to `MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE = 100`) now carries:

- a prescriptive block comment: no heap allocation, no container growth, no packet copy, no DB access on this path,
  including future module packet filters; only boot-time preallocated state;
- `static_assert` locking `ScriptMgr::OnPacketReceive` to `(WorldSession*, WorldPacket const&)` (no by-value copy at the call site);
- `static_assert` keeping the per-session cap per tick `<= 100`.

No runtime tracer was added (none exists in tree). **Known inherited debt:** `ScriptMgr::OnPacketReceive` copies the packet
(`WorldPacket copy(packet)`) when at least one `ServerScript` is registered - one heap allocation per packet. It is out of
this block's scope (TrinityCore ScriptMgr API) and is tracked for the module-filter work.

## 3. D9 - manifest validation wired at boot

| Piece | Path |
|---|---|
| Boot gate (pure, Trinity-free, lint FAIL surface) | `src/modules/sdk/include/aeternum/sdk/internal/manifest_gate.hpp` |
| Host wiring | `src/modules/sdk/src/ModuleHost.cpp` `HostDiscoverAndStart(ManifestBootOptions)` |
| Config read (allowed adapter seat) | `src/modules/sdk/adapters/trinity/TrinityOpsAdapter.cpp` `OnStartup` |
| Manifest deployment | `trinity_add_module(... MANIFEST <file>)` + `worldserver` POST_BUILD copy |
| First manifest | `src/modules/admin_monitoring/module.manifest.json` (`admin.monitoring`, abi 1, on-boot-only, critical false) |

Flow (runs once in `WorldScript::OnStartup`, before planning; never on tick/packet paths):

1. Adapter reads `Aeternum.Modules.ManifestDir` (default: `<directory of worldserver.conf>/modules`) and
   `Aeternum.Modules.RequireManifest` (default `0`). Both keys optional and read quietly.
2. For every registered module: `<dir>/<id>/module.manifest.json` is read (max 64 KiB; id must be a safe dotted id)
   and parsed with `manifest.hpp` (rapidjson).
3. Checks: JSON valid; `id` equals the compiled descriptor id; `abiVersion == AETERNUM_MODULE_ABI_VERSION`;
   `loadPolicy` default `on-boot-only`, `runtime-restart` accepted (same as boot in M1), `hot-reloadable` rejected
   (Level 3 not implemented); `integrity.sha256` if present must be 64 hex chars (format check; **real hash landed in Blocco4 / D10** — see M1_KERNEL_BLOCK4.md).
4. **Fail-closed:** a present-but-invalid manifest always rejects the module. Rejected + `critical` => boot aborted
   (`ProcessExit::Error`); rejected + non-critical => module disabled (dependents disabled by the planner), server continues.
5. Missing manifest: `RequireManifest=0` => legacy static load with WARN (M1 transitional); `RequireManifest=1` => rejected.
6. No registered modules (vanilla) => gate skipped, boot unchanged.

Build output: Windows `<bin>/<config>/modules/<id>/module.manifest.json`; UNIX `<bin>/../etc/modules/<id>/`; both also `install()`ed.
CMake refuses to configure if a declared manifest has no valid `id`.

## 4. Cleanup (M1-Blocco2 cleanup)

| Item | Change |
|---|---|
| Level-aware appender | `src/common/Logging/Appender.cpp`: ERROR/FATAL lines always carry the `ERROR `/`FATAL ` tag, also with appender flags `0` (default `Appender.Server`, `Appender.Console`, `Appender.Bnet`). INFO/DEBUG/WARN lines are unchanged. The per-message prefix is now reset for every appender, so a `flags=0` appender never reuses the prefix built by another appender of the same logger. Text counts such as `Select-String '^ERROR |^FATAL '` on `Server.log` are real (no false zeros). |
| Manifest log category | `module.sdk.manifest`: one INFO summary per boot (`Module manifest gate: N accepted, N legacy, N rejected (dir ..., requireManifest=...)`), WARN per module without manifest, ERROR per rejected module, per-module OK at DEBUG. `worldserver.conf.dist` ships `Logger.module.sdk.manifest=3,Console Server`: the gate is visible without enabling the whole `module.sdk` category on the console. `loadPolicy` is printed by name. |
| Config schema | `Aeternum.Modules.ManifestDir` / `Aeternum.Modules.RequireManifest` added to the known-key catalog (no `unknown key` WARN at boot). |
| Manifest deploy | `contrib/aeternum/scripts/Deploy-AeternumModuleManifests.ps1`, shipped next to the binaries by POST_BUILD. Copies every `<bin>/modules/<id>/module.manifest.json` into the directory the instance reads: `Aeternum.Modules.ManifestDir` from `-ConfFile`, otherwise `<dir of -ConfFile>/modules`. Never deletes; `-DryRun` prints the plan. |

Instance with its own conf folder (example `_testinst_iem1`, conf in `..\conf`):

```
powershell -NoProfile -ExecutionPolicy Bypass -File <bin>\Deploy-AeternumModuleManifests.ps1 -ConfFile <instance>\conf\worldserver.conf -SourceModulesDir <build>\bin\RelWithDebInfo\modules
```

Target: `<instance>\conf\modules\<id>\module.manifest.json` (not the build output `bin\RelWithDebInfo\modules`).

## Tests

`tests/modules/ManifestGate.cpp` (tags `[m1][manifest][gate]`): id safety, missing policy, valid/default policy,
fail-closed cases (bad JSON, id mismatch, ABI mismatch, hot-reloadable, bad loadPolicy, sha256 format), planner
integration (non-critical disabled, critical abort, dependents), vanilla no-op, in-tree `admin_monitoring` manifest.
`tests/common/Appender.cpp` (tags `[log][appender][m1]`): ERROR/FATAL tagged with flags 0, upstream format with explicit
LOGLEVEL flag, no stale prefix across appenders, level filter unchanged. `tests/common/ConfigSchema.cpp`: manifest keys known.

## Deploy note

Deployments that copy only `worldserver.exe` will not have `modules/<id>/module.manifest.json`; with the default
`RequireManifest=0` modules still load (WARN). Copy the `modules/` folder next to the config before setting `RequireManifest=1`.

## Registry

| Date | Note |
|---|---|
| 2026-11-05 | M1-Blocco2 DEV land (P-a, P-c, D9 boot wiring) |
| 2026-11-05 | M1-Blocco2 cleanup: level-aware appender, `module.sdk.manifest` log category, manifest deploy script |
