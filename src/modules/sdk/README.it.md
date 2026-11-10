# Aeternum Module SDK (statico, v0.2 draft)

Header pubblici in `include/aeternum/sdk/`. Interni in `internal/`. Host in `src/ModuleHost.cpp`. Non congelato.

## Pezzi kernel M1 (feat/m1-kernel)

| Header | Ruolo |
|---|---|
| `include/aeternum/sdk/abi_guard.hpp` | Guard D11 (nessuna eccezione sul confine C) |
| `include/aeternum/sdk/manifest.hpp` | Parser `module.manifest.json` (rapidjson in-tree) |
| `include/aeternum/sdk/db_callback_ring.hpp` | Ring MPSC lock-free (8192) per callback DB |

ABI C: `include/aeternum/abi/AeternumABI.h`. Test Catch2 con tag `[m1].

## M1 blocco 2 - gate manifest al boot

`include/aeternum/sdk/internal/manifest_gate.hpp` (interno) valida `<ManifestDir>/<id>/module.manifest.json` una sola volta in
`HostDiscoverAndStart` (id coerente, `abiVersion`, `loadPolicy` default on-boot-only, SHA-256 reale (D10) di integrity.file se integrity.sha256 e presente). Manifest invalido =>
modulo rifiutato (critico => abort del boot). Manifest assente => caricamento legacy + WARN, salvo `Aeternum.Modules.RequireManifest = 1`.
I moduli dichiarano il file con `trinity_add_module(... MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/module.manifest.json)`.
Dettagli: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK2.md`.

Log: categoria `module.sdk.manifest` (riepilogo INFO, WARN legacy, ERROR rifiutato, DEBUG OK per modulo). Le istanze con conf separata
ricevono i manifest con `Deploy-AeternumModuleManifests.ps1 -ConfFile <istanza>/conf/worldserver.conf` (copiato accanto ai binari).

## M1 blocco 3 - Module Manager FSM

`include/aeternum/sdk/internal/module_manager.hpp` traccia **Discovered → Validated → Loaded → Active → Error**
durante `HostDiscoverAndStart` (integra il gate manifest del blocco 2 e `PlanStaticModules`). Nessun hot-reload.
Introspezione: `ListModuleLifecycle()`. Dettagli: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK3.md`.

## M1 blocco 4 - integrita D10 + lifecycle admin

SHA-256 reale (OpenSSL EVP) del file allowlist `integrity.file` accanto al manifest; mismatch => gate Rejected / FSM Error.
Admin HTTP `GET /modules/lifecycle` espone `ListModuleLifecycle()` (token + `admin.read`).
Dettagli: `docs/independent_engine/architecture/modules/M1_KERNEL_BLOCK4.md`.
