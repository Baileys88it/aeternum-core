# Aeternum Module SDK (static, v0.2 draft)

Public headers under `include/aeternum/sdk/`. Internal planner under `include/aeternum/sdk/internal/`.

Host: `src/ModuleHost.cpp` (ScriptMgr WorldScript). Not frozen (`AETERNUM_SDK_API_DRAFT`).

## M1 kernel pieces (feat/m1-kernel)

| Header | Role |
|---|---|
| `include/aeternum/sdk/abi_guard.hpp` | D11 noexcept wrappers for C ABI |
| `include/aeternum/sdk/manifest.hpp` | `module.manifest.json` via in-tree **rapidjson** |
| `include/aeternum/sdk/db_callback_ring.hpp` | MPSC lock-free ring (8192) for DB callbacks |

C ABI header lives at `include/aeternum/abi/AeternumABI.h`. Tests: `tests/modules/*` tags `[m1].

## M1 block 2 - boot manifest gate

`include/aeternum/sdk/internal/manifest_gate.hpp` (internal) validates `<ManifestDir>/<id>/module.manifest.json` once in
`HostDiscoverAndStart` (id match, `abiVersion`, `loadPolicy` default on-boot-only, D10 SHA-256 of integrity.file when integrity.sha256 is set). Invalid manifest =>
module rejected (critical => boot abort). Missing manifest => legacy load + WARN unless `Aeternum.Modules.RequireManifest = 1`.
Modules declare their file with `trinity_add_module(... MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/module.manifest.json)`.
Details: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK2.md`.

Logging: category `module.sdk.manifest` (INFO summary, WARN legacy, ERROR rejected, DEBUG per-module OK). Instances with their own
conf folder get manifests with `Deploy-AeternumModuleManifests.ps1 -ConfFile <instance>/conf/worldserver.conf` (shipped next to the binaries).

## M1 block 3 - Module Manager FSM

`include/aeternum/sdk/internal/module_manager.hpp` tracks **Discovered → Validated → Loaded → Active → Error**
during `HostDiscoverAndStart` (integrates with the Blocco2 manifest gate + `PlanStaticModules`). No hot-reload.
Introspection: `ListModuleLifecycle()`. Details: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK3.md`.

## M1 block 4 - D10 integrity + admin lifecycle

Real SHA-256 (OpenSSL EVP) of the allowlisted `integrity.file` next to the manifest; mismatch => gate Rejected / FSM Error.
Admin HTTP `GET /modules/lifecycle` exposes `ListModuleLifecycle()` (token + `admin.read`).
Details: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK4.md`.
