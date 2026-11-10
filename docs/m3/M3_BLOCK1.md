# IE-M3 Blocco1 - OS-level loader + C-ABI wrapper (native .dll/.so, on-boot-only)

**Milestone:** IE-M3 Blocco1 (authorized by maintainer, t667u)
**Date:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime` (worktree `<ENGINE_ROOT>\wt_m3`), rebased on `feat/m2-protocol` `34ceddeba8d4` (tag `M2-Complete` absent). **Local commit only - no push.**
**Production / isolata:** untouched. Build tree: `<ENGINE_ROOT>\build_m3` (RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`).
**IT:** [`M3_BLOCK1.it.md`](M3_BLOCK1.it.md) - **Design:** [`M3_RUNTIME_DESIGN.md`](M3_RUNTIME_DESIGN.md) (§8 decisions, §9 status)

## 1. Switch (maintainer Q5)

| Switch | Default | Effect |
|---|---|---|
| CMake `AETERNUM_NATIVE_MODULE_RUNTIME` | **OFF** | OFF: `ModuleHost` has no native code path (compiled out); static `IModule` path and vanilla boot unchanged. ON: loader linked into `aeternum-sdk` |
| worldserver.conf `Aeternum.Modules.NativeRuntime` | **0** | Read only when compiled ON; 1 = scan `<Aeternum.Modules.ManifestDir>/<id>/` for `"runtime": "native"` |

Distinct from Trinity `WITH_DYNAMIC_LINKING` / `ScriptReloadMgr` (not used, stays OFF). The loader library `aeternum-native-runtime` is also built for Catch2 when `BUILD_TESTING=ON` (test-scoped link; the worldserver does not get it while the option is OFF).

## 2. Manifest (native keys, optional; absent = static, M1 unchanged)

```json
{ "id": "my.module", "abiVersion": 1, "runtime": "native", "library": "my.module.dll", "critical": false,
  "integrity": { "files": [ { "path": "my.module.dll", "sha256": "<64 hex>" } ] } }
```
The M1 static gate rejects a `"runtime": "native"` manifest for a compiled-in id (`runtime_native_not_static_module`).

## 3. Load order (F5, fail-closed) - `native_runtime.hpp`, `native/NativeRuntime.cpp`

| FSM | What happens | Never |
|---|---|---|
| Discovered | `<root>/<id>/module.manifest.json` with `"runtime": "native"` seen (ids already owned by a static module are skipped) | no mapping |
| Validated | parse; `id` = dir; `abiVersion` = 1; `hot-reloadable` refused; `library` safe + `.dll`/`.so`; **integrity required** (missing hash = Rejected, stricter than M1); ambiguous form = Rejected; every path relative/allowlisted charset, no `..`, no absolute; module dir and every path component checked for symlink / junction / reparse point and `canonical(path) == root/id/path`; EVP SHA-256 of **every** file, constant-time compare; the image must be one of the hashed files. Windows: the image is opened deny-write (`FILE_SHARE_READ`), its final path re-checked, hashed **through that handle**, and the handle is kept until mapping | **no LoadLibrary/dlopen** |
| Loaded | `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR \| LOAD_LIBRARY_SEARCH_SYSTEM32)` / `dlopen(path, RTLD_NOW \| RTLD_LOCAL)` on the **same canonical path** (Linux: dev/inode re-checked against the hashed fd); image range from the PE header / `dl_iterate_phdr`; only `aeternum_module_entry` resolved (missing = unload + Error); entry called inside the F1 guard; checks: `aeternum_abi_compatible` (v1 equality), `ft.struct_size` in `[AETERNUM_MODULE_FT_V1_BASE_SIZE, 4096]`, `info.id` (must live in the image) = manifest id, `load_policy` and `critical` = manifest, every FT pointer inside the image. **No hooks published** (no hook table in Blocco1) | no hash-after-load as the only check |
| Active | `on_load` → `on_init` → `on_start`, each guarded | |
| Error | reason `native_<cause>[:detail]` in `/modules/lifecycle` | never `TC_LOG_FATAL`; only a **critical** native module aborts boot (M1 `StopNow(Error)` + rollback) |

Release rules: clean failure (map/entry/ABI/id/policy) = image released. Contained fault = image **kept mapped**, no re-entry. Shutdown (`HostStopAndShutdown`, natives first): `on_stop`/`on_shutdown` guarded, then FreeLibrary/dlclose only if no fault and `in_flight == 0`.

Per record: OS handle, file lock, FT copy (`min(struct_size, sizeof)`), reported `struct_size`, `mod_ctx`, image range, last fault (code + address), `faulted` (sticky), `in_flight`.

## 4. F1 guard - `native_guard.h`, `native/NativeGuardWin.c`, `native/NativeGuardPosix.c`

Pure C trampolines (no C++ object in the guarded frame) around **every** init crossing: `aeternum_module_entry`, `on_load`, `on_init`, `on_start` (and `on_stop`, `on_shutdown` at shutdown).
- Windows: `__try/__except`; filter accepts AV, in-page, bounds, misalignment, illegal/privileged instruction, int/float div0 and overflow family, stack overflow (`_resetstkoflw` after the block), and the MSVC C++ exception `0xE06D7363` only when `ExceptionInformation[3]` is the module image base (D11 breach by the module). **Only** if the fault address is inside the module image. Never masked: breakpoint, single-step, guard page, heap corruption, fail-fast/GS (`0xC0000409`).
- Linux: thread-local `sigsetjmp` armed only during the call; `SIGSEGV/SIGBUS/SIGFPE/SIGILL` installed with `SA_SIGINFO | SA_ONSTACK` for the call (previous dispositions restored), per-thread `sigaltstack`; PC from `ucontext` checked against the image; otherwise forwarded to the previous disposition.
- A fault in core code (outside the image) is **not** swallowed (normal crash path / dump).
- Contained fault → module Error, `faulted` sticky, no re-entry, process flagged **tainted** (`IsNativeRuntimeTainted()`, Host FT `runtime_tainted()`, ERROR log "graceful restart recommended (policy Q6: 60s; not automatic in Blocco1)").

**Not covered by the guard (F0):** `DllMain` / static constructors run inside `LoadLibrary`, before any crossing: contract = trivial `DllMain` (the Catch2 dummy returns `TRUE` and does nothing). A crash there is effectively fatal - this is why integrity is verified **before** mapping. Wild writes before a fault, heap/GS corruption, module threads, `ExitProcess`, VEH installed by the module and hangs are outside in-process containment (hang = existing `MaxCoreStuckTime`).

## 5. Host FT (maintainer Q1: ABI v1 + `struct_size`)

`AETERNUM_MODULE_ABI_VERSION` stays `1`. `AeternumHostFT` appended: `log_error(AeternumStringView)`, `runtime_tainted(void)`. Helpers in `AeternumABI.h`: `AETERNUM_FT_FIELD_END`, `AETERNUM_FT_HAS(ft, type, field)`, `AETERNUM_HOST_FT_V1_BASE_SIZE`, `AETERNUM_MODULE_FT_V1_BASE_SIZE`, `AETERNUM_FT_MAX_STRUCT_SIZE`. The host passes `struct_size = sizeof(AeternumHostFT)`; modules test `AETERNUM_FT_HAS` before using an appended field. `db_async_query` returns `AETERNUM_ERR_UNSUPPORTED` in Blocco1 (DB ring not wired). No STL, no exceptions, no `std::function` across the ABI; `check_abi_no_stl.ps1` unchanged and PASS. No hook-subscription API yet (hook table = later block; SMSG observe-only Q3, no movement hooks Q4). No `IModule` → FT bridge (Q7).

## 6. QA - Catch2 `[m3][blocco1]` (`tests/modules/NativeRuntimeBlocco1.cpp`)

Dummy images (real `.dll`, static CRT, imports `KERNEL32.dll` only): `tests/modules/native_dummy/aeternum_m3_dummy.c` built as `aeternum_m3_dummy_{ok,fault_init,fault_entry,bad_abi}`. Fixtures under `build_m3/tests/m3b1_fixtures/`, hashes computed by the test.

| Case | Result |
|---|---|
| Valid SHA-256 | Validated with 0 map calls and image not mapped → 1 `LoadLibraryExW` → entry/on_load/on_init/on_start in order → Active; shutdown on_stop/on_shutdown → FreeLibrary (image no longer mapped) |
| Corrupt digest / corrupted image / one flipped hex digit | Error `native_sha256_mismatch:<file>`, **0** map calls, no module code ran, image not mapped |
| F5 rules | no hash, ambiguous integrity, image not hashed, `..` / absolute library, unsafe extra path, hot-reloadable → Rejected with 0 map calls; static gate refuses native manifest |
| Directory symlink escaping the root | `native_module_dir_reparse` (skipped with SUCCEED if the host forbids symlink creation) |
| Null write in `on_init` | `native_fault:on_init:code=0xC0000005:rva=...`, fault address inside the image, `faulted`, tainted, `on_start` never called, later crossings refused (no re-entry), image kept mapped, **test process alive** |
| Null write in `aeternum_module_entry` | `native_fault:entry:...`, never Loaded, no re-entry, process alive |
| Entry reports abi 2 | `native_abi_mismatch:2`, image released, not tainted |
| Critical mismatch | `abortStartup` requested (M1 semantics), no FATAL |
| Zero native modules / static manifest only | nothing discovered, nothing mapped |

Numbers: see the Master Report (`tests.exe "[m3][blocco1]"` and `"[m1]"` on `build_m3`).
Linux: `NativeGuardPosix.c` + the dummy were compiled and exercised with gcc on a Linux box (entry/on_init faults contained twice in one process; core fault still crashes with SIGSEGV). The full Linux CMake build is not run in this block.

## 7. Not in Blocco1

Packet hook table / `WorldSession` + `SendPacket` wiring, `HostAlloc` cap + cookie (F2), thread-id allowlist on host APIs (F4.1, no world/map API is exposed yet), DB MPSC ring, automatic 60 s graceful restart, Linux CI build, production / isolata deploy.

## 8. Rollback

Revert the Blocco1 local commit. With `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` (default) the worldserver binary has no native code path at all.
