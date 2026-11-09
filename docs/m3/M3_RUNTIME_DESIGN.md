# M3 — Module Runtime: design (Discovery, DESIGN ONLY)

**Phase:** M3 Discovery (design) — decisions closed by maintainer t667u; **Blocco 1 implemented** (§9), **Blocco 2 implemented** (§10) | **Date:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime`, originally from `aeternum/main` = `a52e748262` (M1-Complete); **rebased on `feat/m2-protocol` `34ceddeba8d4`** (Q2, tag `M2-Complete` absent). Sections 1-7 keep the Discovery cites (taken on main / READ-ONLY from `wt_m2`).
**Authorization:** maintainer t663u. **Priority:** STABILITY > CORRECTNESS > COMPATIBILITY > PERFORMANCE > SIMPLICITY.
**Labels:** EXISTING (in the tree today) · M2-BRANCH (only in `feat/m2-protocol`) · PROPOSED (M3 design, not implemented) · OPEN (decision for maintainer).
**Not to be confused with** `docs/development/*/M3_REPORT.md` (old Metrics/Measurement milestone).

---

## 0. Summary of decisions

| # | Decision | Status |
|---|---|---|
| T1 | FSM order: Discovered → Validated (manifest + SHA-256, **no** LoadLibrary) → Loaded (OS image mapped + entry resolved) → OnLoad/OnInit/OnStart → Active (hooks published) | IMPLEMENTED (Blocco 1) |
| T2 | M3 loader is a **new** Module Manager OS-handle path. Trinity `WITH_DYNAMIC_LINKING` / `ScriptReloadMgr` is **not** used for SDK modules | IMPLEMENTED (Blocco 1) |
| T3 | `on-boot-only` only. No hot FreeLibrary. FreeLibrary only at clean shutdown, no prior fault, `in_flight == 0` | IMPLEMENTED (Blocco 1) |
| T4 | Fail closed, aligned with `ModuleHost`: load/entry/ABI/hash failure → Rejected/Error, server stays up; critical abort only where M1 already aborts | IMPLEMENTED (Blocco 1) |
| T5 | Dynamic registration only via `aeternum_module_entry` → `AeternumModuleFT`. `IModule`/`RegisterModule` stays Level 1 static | IMPLEMENTED (Blocco 1) |
| T6 | Every FT crossing wrapped by D11 (`InvokeNoexcept`) **plus** SEH (Windows) / signal wrapper (Linux) | IMPLEMENTED (Blocco 1) |
| T7 | Packet hooks on logical opcodes: inbound in `WorldSession::Update` just before `opHandle->Call`; outbound around `SendPacket` before `TranslateOutbound`; preallocated table, zero heap | PROPOSED |
| — | Hot-reload | **Out of scope for M3** (later gated option, requires DYNAMIC_LOADING_AND_HOT_RELOAD.md §5/§6 checklist) |
| — | Out-of-process sandbox | **Not default, not M3.** Future optional hardening only |

---

## 1. What exists today (inspected on `aeternum/main` a52e748262)

| Fact | Cite | Label |
|---|---|---|
| C ABI: `extern "C"`, `AETERNUM_MODULE_ABI_VERSION 1u`, status codes, opaque handles `AeternumPlayerHandle` / `AeternumSessionHandle` / `AeternumModuleHandle` / `AeternumMapHandle`, `AeternumStringView`, `AeternumOutBuffer` | `include/aeternum/abi/AeternumABI.h:69-102` | EXISTING |
| Module FT: `struct_size`, `on_load`, `on_init`, `on_start`, `on_stop`, `on_shutdown`, `metrics_observe` | `AeternumABI.h:119-127` | EXISTING |
| Host FT: only `log_info` and `db_async_query` | `AeternumABI.h:129-136` | EXISTING |
| Entry point `aeternum_module_entry(host_abi_version, host, out)` prescribed; strict equality `aeternum_abi_compatible` | `AeternumABI.h:144-152` | EXISTING (declaration only) |
| **Zero callers** of `aeternum_module_entry`; **no** `LoadLibrary` / `dlopen` / `GetProcAddress` in `src/modules/sdk` | `git grep` on main (only `ScriptReloadMgr.cpp:256-258`, Trinity HotSwap) | GAP |
| D11 guard: `InvokeNoexcept`, `AETERNUM_C_API_BEGIN/END` (catch `std::exception` and `...`) | `src/modules/sdk/include/aeternum/sdk/abi_guard.hpp:49-75` | EXISTING |
| FSM `Discovered → Validated → Loaded → Active`, any → `Error`; illegal transitions return false | `sdk/internal/module_manager.hpp:45-52`, `Discover` `:115`, `MarkValidated` `:130`, `MarkLoaded` `:141`, `MarkActive` `:152`, `MarkError` `:166` | EXISTING |
| Host boot: `TrinityOpsAdapter.cpp:142` `ModuleSdkWorldScript::OnStartup` → `HostDiscoverAndStart(ResolveManifestBootOptions())`; shutdown `:143` → `HostStopAndShutdown` | `src/modules/sdk/adapters/trinity/TrinityOpsAdapter.cpp:138-143` | EXISTING |
| `HostDiscoverAndStart`: registry sighting → Discover (`ModuleHost.cpp:180-187`); manifest gate only if `!planned.empty()` (`:191`); critical reject → `StopNow(Error)` (`:229-235`); `PlanStaticModules` (`:240`); `OnLoad+OnInit` → `MarkLoaded` (`:261-282`); `OnStart` → `MarkActive` (`:284-306`); critical failure rollback + abort (`:264-277`) | `src/modules/sdk/src/ModuleHost.cpp:166-314` | EXISTING |
| Modules today: static `IModule` + `RegisterModule`, `ModuleRuntime::Static` only | `sdk/module.hpp:25-29,65-75`; `admin_monitoring/AdminMonitoringScripts.cpp:104,193` | EXISTING |
| Manifest gate: id match, `abiVersion == AETERNUM_MODULE_ABI_VERSION`, `hot-reloadable` rejected, SHA-256 of `integrity.file` / `integrity.files[]` (paths under `<manifestDir>/<id>/`) | `sdk/internal/manifest_gate.hpp:184-242`, hot-reload reject `:212-213`; hashing `sha256_file.hpp:133` | EXISTING |
| Missing integrity = still accepted in M1 (transitional) | `manifest_gate.hpp:33` | EXISTING (M3 tightens, §3) |
| `loadPolicy` default `on-boot-only` | `sdk/manifest.hpp:55-72` | EXISTING |
| ABI lint: no STL headers under `include/aeternum/abi/**` | `contrib/aeternum/ci/check_abi_no_stl.ps1` | EXISTING |
| Lock-free MPSC ring for DB callbacks, `DbCallbackRing8192` | `sdk/db_callback_ring.hpp:55-142` | EXISTING (not yet wired to a host FT) |
| `ConfigReloadHook` = plain C function pointer slot (already C-safe) | `include/aeternum/api/ops/services.hpp:33-43` | EXISTING |
| P-c zero-heap rule + `static_assert` on `OnPacketReceive` signature + cap 100 packets/session/tick | `src/server/game/Server/WorldSession.cpp:386-403` | EXISTING |
| `ScriptMgr::OnPacketReceive` copies the packet (`WorldPacket copy(packet)`) when a ServerScript is registered | `src/server/game/Scripting/ScriptMgr.cpp:1494-1500` | EXISTING (debt) |
| D4: no event bus on movement/aura until M3 design | `DYNAMIC_LOADING_AND_HOT_RELOAD.md` §0 | DECISION |

**Gap M3 fills:** a generic OS-level (`.dll` / `.so`) loader inside the Module Manager, a host-side caller of `aeternum_module_entry`, the FT trampolines with fault containment, a packet hook table, and the ABI surface for hook subscription. None of these exist today.

**Two semi-ABIs (documented, not resolved now):** static `IModule` (C++ vtable, Level 1, same binary) and the C `AeternumModuleFT` (dynamic). M3 does **not** build the bridge; an optional static→FT adapter can come later. No C++ vtable crosses the DLL boundary.

---

## 2. OS-level dynamic loading (A)

### 2.1 Discovery
- PROPOSED: in addition to the registry sighting (`ModuleHost.cpp:180-187`), scan `<manifestDir>/<id>/module.manifest.json` (same root as `ManifestBootOptions::manifestDir`, default `modules`, `manifest_gate.hpp:59-63`) for manifests declaring `"runtime": "native"`. File seen → `Discover(id, critical)`. **No mapping** at this step.
- Both kinds (static and native) feed the same `PlanStaticModules` (dependencies, profiles, cycles; `static_runtime.hpp:58`).
- Zero native modules + zero static modules = vanilla boot unchanged (`ModuleHost.cpp:191`).

### 2.2 Validated (no LoadLibrary)
Manifest parse + path allowlist (`IsSafeIntegrityRelativePath`) + SHA-256 of **every** declared file + `abiVersion` + `loadPolicy` (default `on-boot-only`) + plan. **On the native path a missing hash is Rejected** (stricter than M1, where missing integrity is still accepted, `manifest_gate.hpp:33`). The native binary must be one of the `integrity.files[]` entries. A rejected module is **never** passed to LoadLibrary/dlopen.

### 2.3 Loaded (T1)
1. Map the **same canonical path just hashed**:
   - Windows: `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)` (no PATH/CWD search).
   - Linux: `dlopen(path, RTLD_NOW | RTLD_LOCAL)` (unresolved symbols fail at load, no symbol interposition).
2. Resolve **only** `aeternum_module_entry` (`GetProcAddress` / `dlsym`). Missing → Error.
3. Call the entry inside the F1 / D11 wrapper (§5) with `AETERNUM_MODULE_ABI_VERSION` and the host FT.
4. Check `out.info.abi_version` (strict `aeternum_abi_compatible`), `out.ft.struct_size`, `out.info.id == manifest id`, `load_policy` and `critical` consistent with the manifest. Mismatch → Error.
5. `MarkLoaded`. **Hooks are not published yet.**

**Semantic extension of "Loaded" (states are not renamed):** for native modules `Loaded` means "OS image mapped + entry resolved and accepted". For static `IModule` it keeps the M1 meaning "after OnLoad+OnInit" (`ModuleHost.cpp:281-282`).

### 2.4 Start → Active
Existing ModuleHost sequence through the FT: `on_load`, `on_init` (hook subscriptions are allowed only here; the module stays Loaded) → `on_start` → `MarkActive` + hooks published at the current epoch (§4.4).

### 2.5 Load policy / hot-reload
- `on-boot-only` is the only native policy in M3. `runtime-restart` = load at next boot (as in M1). `hot-reloadable` stays rejected (`manifest_gate.hpp:212-213`).
- Hot-reload: **out of scope for M3**; any later option must pass the §5/§6 checklist of DYNAMIC_LOADING_AND_HOT_RELOAD.md and is forbidden for modules that pass STL across the ABI (D3).

### 2.6 Not Trinity HotSwap (T2)
Trinity `WITH_DYNAMIC_LINKING` (`cmake/options.cmake:39`) and `ScriptReloadMgr` (`ScriptReloadMgr.cpp:256-258`) load `ScriptObject` C++ libraries with STL across the boundary — violates D2/D3. They stay off. The M3 loader is a separate path owned by the Module Manager (OS handle per module record).

### 2.7 Failure policy (T4)

| Failure | Result | Boot |
|---|---|---|
| Manifest invalid / hash mismatch / hash missing (native) | Rejected → Error | critical: abort as M1 (`ModuleHost.cpp:229-235`); non-critical: skip |
| LoadLibrary / dlopen fails, entry missing, ABI/struct_size/id mismatch | Error, image released (no code of the module has run except loader/static init) | non-critical skip; critical as M1 lifecycle abort |
| `on_load` / `on_init` / `on_start` fails or faults | Error | same as `ModuleHost.cpp:261-303` |
| Never `TC_LOG_FATAL` for these cases; server stays up unless the module is critical | | |

---

## 3. FSM ↔ C ABI integration (B)

- FSM records stay in `ModuleManager` (`module_manager.hpp:79-192`; STL is allowed there — host side, boot-only). PROPOSED new per-record host data: OS handle, `AeternumModuleFT` copy, `mod_ctx`, image range, fault code, `in_flight` counter.
- **No STL at the boundary.** Everything crossing the DLL: primitives, `AeternumStringView`, `AeternumOutBuffer`, opaque handles, C function pointers + `void* user`. `std::function` / lambdas with capture are **forbidden** (heap + CRT mismatch). The ABI lint (`check_abi_no_stl.ps1`) must also cover the new hook header (PROPOSED).
- **ABI growth (PROPOSED):** hook subscription needs new host functions. Options: append to `AeternumHostFT` guarded by `struct_size`, and bump `AETERNUM_MODULE_ABI_VERSION` to 2. Since compatibility is strict equality (`AeternumABI.h:149-152`) and the gate rejects `abiVersion != AETERNUM_MODULE_ABI_VERSION` (`manifest_gate.hpp:207-208`), a bump invalidates v1 manifests (only `admin.monitoring`, static). → OPEN Q1.
- **Host FT rules (PROPOSED):** `log_info` thread-safe; `db_async_query` async only, completion delivered on the world thread through a `DbCallbackRing8192` drain (`db_callback_ring.hpp:142`). No sync DB from modules.
- **Fault → FSM** (PROPOSED): on hot paths the trampoline cannot allocate or take `g_mutex` (`ModuleHost.cpp:24`). It sets a per-module atomic "disabled" bit and a fault code in a preallocated slot; the world thread converts it to `MarkError(id, reason)` on the next tick.

---

## 4. Event Hooking System (C)

### 4.1 What modules see
Logical opcodes only (after the M2 adapter). A hook before `TranslateInbound` would see wire opcodes: **out of scope**.

PROPOSED POD view (C, no STL):
```
AeternumPacketView { uint32 logical_opcode; uint8 const* data; uint32 size; uint32 direction; }
callback: uint32 (AETERNUM_ABI_CALL *)(void* user, AeternumSessionHandle s, AeternumPacketView const* v)
verdict: PASS | DROP   (DROP only with a declared capability)
```
The view is borrowed: valid only during the call, read-only, no payload rewrite in M3.

### 4.2 Inbound plug point (T7)
- M2-BRANCH chain: `WorldSocket::ReadDataHandler` → `ProtocolAdapterShadow::TranslateInbound(rawOpcode, logicalOpcode)` (`wt_m2 WorldSocket.cpp:420`) → `packet.SetOpcode(opcode)` (`:434`) → `_worldSession->QueuePacket` (`:485`). Socket-level opcodes (PING, AUTH_SESSION, KEEP_ALIVE, …, `:439-458`) are handled before the session and are **not hookable**.
- **Hook point:** `WorldSession::Update`, immediately **before** `opHandle->Call(this, *packet)` (EXISTING sites `WorldSession.cpp:441, 456, 469, 490`, after `AntiDOS.EvaluateOpcode`), inside the cap of 100 packets/session/tick (`:386, 402-403`).
- **Not** through `ScriptMgr::OnPacketReceive` while it does `WorldPacket copy(packet)` (`ScriptMgr.cpp:1494-1500`): that copy violates P-c.
- Threading: `WorldSession::Update` runs on the world thread and, for thread-safe opcodes, on map threads (`MapSessionFilter`). Hooks must be reentrant and must not block (contract, §5.4).

### 4.3 Outbound plug point (T7)
- If enabled: inspect/cancel on the **logical** packet around `SendPacket`, **before** `TranslateOutbound`: `WorldSession::SendPacket` (EXISTING `WorldSession.cpp:226`, next to `sScriptMgr->OnPacketSend` `:307`, before `m_Socket[conIdx]->SendPacket` `:310`).
- After `TranslateOutbound` (`wt_m2 WorldSocket.cpp:525`, block/stub `:526-545`, compression/crypto in `WritePacketToBuffer` `:553+`): **audit only**, not a module hook.
- Outbound in M3 = OPEN Q3 (observe only vs cancel).

### 4.4 Hook table (PROPOSED)
- Two dense tables preallocated at boot, indexed by the core logical opcode index (`GetOpcodeArrayIndex`, `Opcodes.h:1081/2531`; sizes `NUM_CMSG_OPCODES = 1984`, `NUM_SMSG_OPCODES = 1671`).
- Per opcode: fixed array of `kMaxHooksPerOpcode` (proposal 4) slots `{fn, user, moduleIndex}` + atomic `subCount`. Global fixed module cap (proposal 32) with atomic enabled bits.
- Fast path: `subCount == 0` → one load + branch. Otherwise bounded scan ≤ `kMaxHooksPerOpcode`. **Zero heap** also on skip and on fault. No `std::unordered_map` / `std::map` (same rule as the M2 `aeternum-adapter-nomap-lint`, `wt_m2 tests/CMakeLists.txt:76`).
- Subscription only during `on_init` (boot); only opcodes declared in the manifest; table frozen after boot.
- Publishing at `Active` with an **epoch** (Security F3); per-module `in_flight` counter around each call.
- Hooks must not do sync DB, I/O or waits; DB only through the async FT + MPSC ring.
- D4: movement/aura opcodes are not hookable by default → OPEN Q4.

### 4.5 Merge dependency
The plug points in `WorldSocket` / adapter only exist in `feat/m2-protocol`. The hook interface lands in M3; wiring at `WorldSession::Update` / `SendPacket` lands when M2 merges into `aeternum/main` (OPEN Q2).

---

## 5. Security and anti-crash (D) — first-cut policy

Security Engineer findings F1/F3/F5 are integrated where the Tech Lead decisions name them (F5 = load order §2, F1 = SEH/signal wrapper, F3 = epoch/in_flight/unload). Remaining refinement belongs to the Security Engineer.

### 5.1 Trust anchor
SHA-256 + Validated before any LoadLibrary (§2.2). Load the same canonical path just hashed. PROPOSED: keep the file open with deny-write share while hashing and mapping (Windows) to close the TOCTOU window; Linux equivalent to be defined. Static constructors / `DllMain` run inside LoadLibrary: the SDK forbids work there and the loader call itself runs inside the wrapper.

### 5.2 Fault containment (F1 / D11, T6)
- Every crossing (entry, `on_*`, `metrics_observe`, hooks, DB callbacks) goes through a host trampoline:
  - C++: `InvokeNoexcept` / `AETERNUM_C_API_*` (`abi_guard.hpp:49-75`). C++ `catch` alone does **not** catch an access violation.
  - Windows: `__try/__except` in a C-only trampoline (no C++ objects to unwind in that frame); filter handles access violation, illegal instruction, divide by zero, stack overflow, and also the MSVC C++ exception code if it escapes an `extern "C"` function. Only faults whose address lies inside the module image are handled; a fault in core code is not swallowed (normal crash + dump).
  - Linux: per-call thread-local `sigsetjmp` armed only during the module call, `SIGSEGV/SIGBUS/SIGFPE/SIGILL` handler with `sigaltstack`, PC checked against the module image (`dladdr`).
- After a fault: module → Error, hooks disabled, no re-entry, world continues unless `critical` (then controlled stop via `ProcessExit::Error`, as `ModuleHost.cpp:250`).
- **Honest limit:** in-process containment is best effort. A wild write before the fault may already have corrupted core memory. PROPOSED: after any contained fault, the process is flagged "tainted" in `/modules/lifecycle` and a restart at the next window is recommended. An infinite loop cannot be preempted in-process; the existing freeze detector (`MaxCoreStuckTime`) stays the last line.

### 5.3 Memory ownership
- Allocate and free on the same side. Data crossing the ABI is either borrowed (views valid only during the call) or allocated/freed through documented ABI allocators (PROPOSED `host_alloc/host_free` with per-module counters). Module-private heap stays private.
- Leak detection on unload: counters ≠ 0 → WARN (feasible for host allocator only).

### 5.4 Threads
No module-owned thread may call into the world update loop or the network loop. Host FT checks the calling thread where needed and returns `AETERNUM_ERR_DENIED`. No sync DB on world/map threads (P-b).

### 5.5 Unload (T3, F3)
- M3 is on-boot-only: no hot FreeLibrary.
- Clean shutdown (`HostStopAndShutdown`, `ModuleHost.cpp:316-333`): disable bits → unpublish hooks and zero slots (new epoch) → wait `in_flight == 0` → `on_stop` / `on_shutdown` → FreeLibrary/dlclose **only** if no prior fault and `in_flight == 0`.
- After a fault: Error, image intentionally leaked (never FreeLibrary), no re-entry.

### 5.6 Not M3
Out-of-process / WASM sandbox: future optional hardening only (MODULE_SECURITY_MODEL.md §8). maintainer's requirement is in-RAM `.dll/.so`.

---

## 6. Planned verification (not implemented)
- Catch2 `[m3]`: FSM order (no LoadLibrary before Validated), hash missing on native = Rejected, entry missing / ABI mismatch = Error with server up, fault in hook → Error + no re-entry, `subCount == 0` fast path, no heap on hook path, unload only with `in_flight == 0`.
- Lint: `check_abi_no_stl.ps1` on the new hook header; no-map lint on the hook table.
- Test module `.dll/.so` that faults on purpose (isolata only).

## 7. Open questions for maintainer (CLOSED — see §8)
1. ABI v2 (strict equality) vs keep v1 and grow `AeternumHostFT` by `struct_size` only.
2. Merge order: merge `feat/m2-protocol` into `aeternum/main` before M3 implementation, or rebase `feat/m3-runtime` on M2.
3. Outbound hooks in M3: observe only, or cancel too.
4. D4: hooks on movement/aura opcodes allowed in M3 (declared + budgeted) or still forbidden.
5. Native runtime switch: build option and/or config key, default OFF.
6. Contained-fault policy: "tainted" flag + recommended restart vs automatic controlled restart.
7. Static→FT bridge for `IModule`: later milestone or never.

## 8. Decisions (maintainer t667u) — Q1–Q7 closed

| # | Question | Decision (binding) |
|---|---|---|
| Q1 | ABI v2 vs v1 + `struct_size` | **ABI v1.** `AETERNUM_MODULE_ABI_VERSION` stays `1`; `AeternumHostFT` grows only by appending fields guarded by `struct_size` (`AETERNUM_FT_HAS`) |
| Q2 | Merge M2 first vs rebase | **Rebase `feat/m3-runtime` on M2.** Tag `M2-Complete` absent → based on the `feat/m2-protocol` tip `34ceddeba8d4`; no merge into `aeternum/main`, no tag created |
| Q3 | Outbound hooks | **SMSG hooks observe-only** (no cancel) |
| Q4 | Movement/aura hooks (D4) | **No hooks on movement opcodes** |
| Q5 | Native runtime switch | **Build flag, default ON** (Blocco3): CMake `AETERNUM_NATIVE_MODULE_RUNTIME` (+ config `Aeternum.Modules.NativeRuntime`, default 0; empty isolata = 1) |
| Q6 | Contained-fault policy | **Tainted + graceful restart within 60 s** (policy). Blocco 1: Error + no re-entry + process alive + "tainted" flag; the restart itself is a later block |
| Q7 | Static `IModule` → C FT bridge | **Permanently forbidden** |

## 9. Blocco 1 — implemented (OS loader + C-ABI wrapper)

Details, files and QA: [`M3_BLOCK1.md`](M3_BLOCK1.md). Summary:
- §2.1–2.4 / T1, T3, T4: `native_runtime.hpp` + `native/NativeRuntime.cpp` (Discovered → Validated with SHA-256 of every file, **no** map → `LoadLibraryExW(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)` / `dlopen(RTLD_NOW | RTLD_LOCAL)` → entry → `on_load/on_init/on_start` → Active).
- §5.2 / T6: C trampolines `native/NativeGuardWin.c` (`__try/__except`) and `native/NativeGuardPosix.c` (`sigsetjmp` + `sigaltstack`) around every init crossing.
- Q1: `AeternumHostFT` + `log_error`, `runtime_tainted` (appended, `struct_size`).
- Not yet: hook table / `WorldSession` wiring (§4), `HostAlloc` (§5.3), 60 s restart (Q6), DB ring (§3). → hook table, HostAlloc and the 60 s shutdown: Blocco 2 (§10).

## 10. Blocco 2 — implemented (HostAlloc F2 + dense hook table + tainted 60 s shutdown F6)

Details, files and QA: [`M3_BLOCK2.md`](M3_BLOCK2.md). Git: `aeternum/main` fast-forwarded to `34ceddeba8d4`, annotated tag `M2-Complete`, `feat/m3-runtime` (Blocco1 `42c9f7ffea7d`) pushed; Blocco2 local only. Summary vs. the PROPOSED sections:
- §5.3: `host_alloc`/`host_free` appended to `AeternumHostFT` (ABI v1, Q1) with cookie (module slot + generation + size), tail canary, per-block/module/global caps; orphan, stale (double), foreign, spoofed and corrupt frees are a no-op + log + module Error. Leak report on clean stop.
- §4.4: dense `cmsg[1984]` / `smsg[1671]`, ≤ 4 hooks per opcode, atomic `count`, zero heap, 32 module slots with generation; subscription only in `on_init`, staged then published once at the end of `Boot()`, table frozen. **Deviation:** the "only opcodes declared in the manifest" rule is deferred (deny lists only: movement groups CMSG `0x41` / SMSG `0x5E` per Q4, socket-level CMSG).
- §4.2: inbound dispatch at the 4 `opHandle->Call` sites in `WorldSession::Update`, after `OnPacketReceive`. §4.3: outbound observe-only (Q3) in `WorldSocket::SendPacket` on the logical packet before `TranslateOutbound` (not in `WorldSession::SendPacket`; covers every send path). CMSG `DROP` counted and ignored.
- §5.2 / §5.5 / F3: every hook call guarded (`aeternum_guard_call_hook`), `in_flight` + epoch + generation; unpublish → drain → `on_stop`.
- Q6: contained fault ⇒ tainted + one-shot arm ⇒ world thread `RequestShutdown(60, reason)` (existing `World::ShutdownServ`, no `TerminateProcess`).
