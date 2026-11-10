# M1 Implementation Notes (kernel block 1)

**Branch:** `feat/m1-kernel` (local; no remote push for this block)  
**Date:** 2026-11-05 (Europe/Rome)  
**Scope:** library + Catch2 only — **not** wired into `World.cpp` / `WorldSession` runtime paths.

## Landed

| Artifact | Path | Notes |
|---|---|---|
| C ABI header | `include/aeternum/abi/AeternumABI.h` | Pure C11/C++20; MSVC `AETERNUM_EXTERN_C_*`, export/call macros; opaque handles; FT tables; `aeternum_module_entry` |
| D11 guard | `src/modules/sdk/include/aeternum/sdk/abi_guard.hpp` | `InvokeNoexcept` + `AETERNUM_C_API_*` macros; never rethrows |
| Manifest parser | `src/modules/sdk/include/aeternum/sdk/manifest.hpp` | `module.manifest.json`; **rapidjson** (in-tree); default `loadPolicy=on-boot-only`; sha256 format (+ **real file hash in Blocco4**) |
| DB callback ring | `src/modules/sdk/include/aeternum/sdk/db_callback_ring.hpp` | **MPSC** lock-free, 8192 slots (`DbCallbackRing8192`); `try_push` / `try_pop` non-blocking |
| C smoke | `tests/modules/aeternum_abi_c_smoke.c` | Compiles as C11, linked into `tests` |
| Catch2 | `tests/modules/AeternumAbi.cpp`, `ManifestParser.cpp`, `DbCallbackRing.cpp` | Includes BENCHMARK + MPSC stress |

## Ring: MPSC vs SPSC

**MPSC chosen.** `DatabaseWorkerPool` runs `_async_threads` Asio workers (`DatabaseWorkerPool.cpp` / `SetConnectionInfo`). Completions destined for the World Thread can therefore be produced by multiple DB workers; World Thread is the sole consumer and must never wait (D6).

## Build / test

```text
cmake --build <build_dir> --config RelWithDebInfo --target tests
<path_to>/tests.exe "[m1]"
```

Forbidden-includes lint also scans `include/aeternum/abi`.

## Explicitly out of this block

- Runtime integration (World drain, OnPacketReceive filters)
- Real SHA-256 hashing / allowlist enforce
- Dead-code removal of `ScriptReloadMgr` tick (scheduled M1 later)
- Push of `feat/m1-kernel`

## Registry

| Date | Note |
|---|---|
| 2026-11-05 | M1-Blocco1 DEV land |
| 2026-11-05 | Runtime wiring items moved to M1_KERNEL_BLOCK2.md (P-a, P-c, D9 boot gate) |
