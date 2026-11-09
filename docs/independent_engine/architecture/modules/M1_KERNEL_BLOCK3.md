# M1 Implementation Notes (kernel block 3 - Module Manager FSM)

**Branch:** `feat/m1-kernel` (local commits; no push for this block)
**Date:** 2026-11-05 (Europe/Rome)
**Scope:** Module Manager core FSM + Catch2; minimal host wiring. **No World.cpp changes. No hot-reload.**

## Landed

| Artifact | Path | Notes |
|---|---|---|
| Module Manager FSM | `src/modules/sdk/include/aeternum/sdk/internal/module_manager.hpp` | Header-only; Trinity-free; states Discovered → Validated → Loaded → Active → Error |
| Host wiring | `src/modules/sdk/src/ModuleHost.cpp` | Tracks FSM across existing `HostDiscoverAndStart` / gate / planner / lifecycle |
| Introspection | `registry.hpp` `ListModuleLifecycle()` | Snapshot id/critical/state/reason for the current boot cycle |
| Catch2 | `tests/modules/ModuleManager.cpp` | Tags `[m1][manager][fsm]` |

## FSM ↔ existing boot path

| Step (already in Blocco2) | FSM transition |
|---|---|
| Registered `IModule` seen in `HostDiscoverAndStart` | → **Discovered** (`Discover`) |
| `EvaluateModuleManifestAt` + `ApplyManifestGate` reject | → **Error** (`ApplyGateResultsToManager`) |
| `PlanStaticModules` enables module | Discovered → **Validated** (`ApplyPlanToManager`) |
| Planner disables module (profile/deps/…) | Discovered → **Error** (disableReason) |
| `OnLoad` + `OnInit` OK | Validated → **Loaded** |
| `OnStart` OK | Loaded → **Active** |
| Load/Init/Start fail | → **Error** (critical may abort process as before) |
| `HostStopAndShutdown` | manager `Clear()` (static unload = process end; no Level-3 FreeLibrary) |

Illegal transitions return `false` and leave state unchanged. **Error is terminal in M1** (no hot-reload / no recover-to-Active). `Active → Error` is supported for future runtime quarantine recording.

## Explicitly out of this block

- Invasive `World.cpp` changes (none required; host already boots via WorldScript adapter)
- Hot-reload / `WITH_DYNAMIC_LINKING` paths
- Real SHA-256 verify (**done in Blocco4 / D10**)
- Push of `feat/m1-kernel`
- Deploy to `_public_iem1`

## Build / test

```text
cmake --build <ENGINE_ROOT>\build_iem1 --config RelWithDebInfo --target tests
<ENGINE_ROOT>\build_iem1\bin\RelWithDebInfo\tests.exe "[m1]"
```

## Registry

| Date | Note |
|---|---|
| 2026-11-05 | M1-Blocco3 DEV land (Module Manager FSM + host wiring + Catch2) |