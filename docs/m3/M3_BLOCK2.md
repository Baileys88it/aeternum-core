# IE-M3 Blocco2 - HostAlloc cookies (F2) + dense packet hook table + tainted 60 s graceful shutdown (F6)

**Milestone:** IE-M3 Blocco2 (authorized by maintainer, t668u)
**Date:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime` (worktree `<ENGINE_ROOT>\wt_m3`) on top of `42c9f7ffea7d` (Blocco1, pushed). `aeternum/main` = `feat/m2-protocol` = `34ceddeba8d4`, annotated tag `M2-Complete`. **Blocco2 = local commit only - no push.**
**Production / isolata:** untouched (no deploy, no restart). Build tree: `<ENGINE_ROOT>\build_m3` (RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`, `AETERNUM_NATIVE_MODULE_RUNTIME` default **OFF**).
**IT:** [`M3_BLOCK2.it.md`](M3_BLOCK2.it.md) - **Blocco1:** [`M3_BLOCK1.md`](M3_BLOCK1.md) - **Design:** [`M3_RUNTIME_DESIGN.md`](M3_RUNTIME_DESIGN.md) (§10)

## 1. ABI (maintainer Q1: ABI v1, `struct_size` growth only)

`AETERNUM_MODULE_ABI_VERSION` stays `1`. `AeternumHostFT` appended (after `runtime_tainted`):

| Field | Signature | Notes |
|---|---|---|
| `module_self` | `AeternumModuleHandle (void)` | handle of the module whose crossing is running; `NULL` outside a crossing |
| `host_alloc` | `void* (AeternumModuleHandle, uint64_t size)` | `NULL` = refused (0 bytes, cap, no crossing, spoofed handle) |
| `host_free` | `AeternumStatus (AeternumModuleHandle, void*)` | `NULL` = OK; any rejection = no-op + log + module Error |
| `subscribe_packet` | `AeternumStatus (AeternumModuleHandle, uint32_t dir, uint32_t opcode, AeternumPacketHookFn)` | only inside `on_init` |

New C types: `AETERNUM_PACKET_DIR_CMSG/SMSG`, `AETERNUM_HOOK_PASS/DROP`, `AeternumPacketView { struct_size, direction, opcode, size, data, session }` (borrowed, valid only during the call), `AeternumPacketHookFn = uint32_t (*)(void* mod_ctx, AeternumPacketView const*)`. Modules test `AETERNUM_FT_HAS(host, AeternumHostFT, subscribe_packet)` first. `check_abi_no_stl.ps1` unchanged. No `IModule` → FT bridge (Q7).

Module handle = `(generation << 8) | (slot + 1)`: 32 slots (`kMaxNativeModules`), 24-bit generation bumped on every claim, so a handle from an earlier boot/module never resolves. The handle passed must equal the running crossing's slot (`CrossingScope`, thread-local); a mismatch is a **spoof** -> module quarantined.

## 2. HostAlloc / HostFree (F2) - `internal/native_host.hpp`, `native/NativeHost.cpp`

Block = 32-byte header `{ magic "AEM3", slot, generation, reserved, size, check }` + user bytes (16-aligned) + 8-byte tail canary. `check` and the canary are keyed on address + slot + generation + size.

| Rule | Value |
|---|---|
| per block | 16 MiB |
| per module (live) | 64 MiB |
| global (live) | 256 MiB, 16384 live blocks |
| registry | fixed open-addressing table (32768, backward-shift delete) + 256-entry recent-free ring; **looked up before any dereference** |

`host_free` outcomes (`HostFreeResult`): `Freed`, `Null` (OK), and rejected = no-op + `ERROR` log + module **Error** (`native_host_free_<cause>`):
`no_crossing`, `bad_handle` (spoof), `orphan` (pointer never returned by `host_alloc`, e.g. stack / module heap / interior pointer), `stale` (already freed = double free), `foreign` (block of another module/generation), `cookie_corrupt` (header or tail overwritten; block **leaked**, never handed back to the CRT).
Clean stop: live blocks reclaimed with a WARN (leak report). Faulted module: blocks **abandoned** (counted `leakedBlocks`, never freed - memory may be inconsistent).

## 3. Dense packet hook table - `internal/packet_hooks.hpp`, `native/NativeHooks.cpp`

- `inline constinit PacketHookTable g_packetHooks`: `cmsg[1984]`, `smsg[1671]` (`NUM_CMSG_OPCODES` / `NUM_SMSG_OPCODES`), index = copy of `GetOpcodeArrayIndex` (`Opcodes.h`; Catch2 checks all 2^20 group/id pairs both directions). Note: `GetOpcodeArrayIndex` computes `idInGroup` as `uint32`, so its "miss" `-1` becomes `0xFFFFFFFF` (a positive index; Trinity relies on `IsValid()` `< ssize(table)`). `CmsgIndex/SmsgIndex` use a signed id (real `-1`) and every table access is also bounded by 1984/1671. Per opcode: `std::atomic<uint32_t> count`, `staged`, `HookEntry entries[4]` `{fn, moduleSlot, generation, epoch}`. **Zero heap**, no map (`aeternum-adapter-nomap-lint` now also scans both files as hot path: no map, no `new`/`malloc`/`std::vector`/`std::string`).
- **Subscribe only during `on_init`** (subscribe window = the slot whose `on_init` runs). Hooks are **staged**, then published at the end of `Boot()` (release-store of `count`) and the table is **frozen**: any later call (`on_start`, hook, world thread) = `AETERNUM_ERR_DENIED`. Staged hooks of a module that fails before Active are retracted. Network threads are already running during `OnStartup`, hence stage-then-publish.
- Return codes: `DENIED` (no crossing / frozen / not in `on_init` / movement / socket-level), `INVALID_ARGUMENT` (bad direction, null fn, fn outside the module image), `NOT_FOUND` (opcode without dense slot), `OVERFLOW` (5th hook on one opcode).
- **No movement hooks (Q4):** whole CMSG group `0x41` and SMSG group `0x5E` denied (`CMSG_MOVE_GUILD_BANK_ITEM 0x3E004F` is not movement). Socket-level CMSG denied: `AUTH_SESSION`, `AUTH_CONTINUED_SESSION`, `PING`, `KEEP_ALIVE`, `LOG_DISCONNECT`, `ENABLE_NAGLE`, `CONNECT_TO_FAILED`, `ENTER_ENCRYPTED_MODE_ACK`.
- **Inbound:** `WorldSession::Update`, all 4 handler sites: after `sScriptMgr->OnPacketReceive`, before `opHandle->Call` (`DispatchInbound`). No new `ScriptMgr` hook.
- **Outbound (Q3, observe-only):** `WorldSocket::SendPacket` on the logical packet before `TranslateOutbound` (`DispatchOutbound`, `session = NULL`: `_worldSession` is not read without its lock). A `DROP` verdict is counted (`dropIgnored`) and ignored in Blocco2.
- Fast path: `count == 0` -> one acquire load + branch. Game code has **no link dependency** on the runtime: dispatch goes through an atomic function pointer set at publish; with `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` nothing ever publishes. Single-image assumption = `WITH_DYNAMIC_LINKING=OFF` (unchanged).
- **Every crossing guarded:** per call `CrossingScope` (`in_flight++` before the state check, F3), epoch + slot `Active` + generation re-checked, `aeternum_guard_call_hook` (SEH `__try/__except` / POSIX `sigsetjmp`; POSIX handlers installed persistently once hooks exist, per-thread `sigaltstack`). The fault path is heap-free: fixed `snprintf` reason `native_fault:hook:<dir>:0x<opcode>:code=..:rva=..`, slot quarantined (no further crossings), log deferred to the world thread.

## 4. Tainted graceful shutdown (F6 / maintainer Q6)

Any contained fault (init crossing or hook) or HostFree violation that errors a module: process **tainted** + one-shot atomic **arm**. The world thread consumes it: `WorldScript::OnUpdate` (adapter, compiled only with the native runtime ON) -> `PumpGracefulShutdown(Services().process)` -> `IProcessControl::RequestShutdown(60, reason)` = existing `World::ShutdownServ` path, exit `SHUTDOWN_EXIT_CODE`, players see the normal countdown. No `TerminateProcess`/`exit`, no new public API. Test seam: `PumpGracefulShutdown(IProcessControl*)` with a fake, `ResetGracefulShutdownForTest()`.
Shutdown order: unpublish hooks (new epoch) -> slots `Stopping` -> wait `in_flight == 0` (5 s budget) -> `on_stop`/`on_shutdown` -> FreeLibrary only if clean.

## 5. QA - Catch2 `[m3][blocco2]` (`tests/modules/NativeRuntimeBlocco2.cpp`)

New dummy variants in `tests/modules/native_dummy/aeternum_m3_dummy.c`: `aeternum_m3_dummy_hooks`, `_orphan`, `_double_free` (opcode values in `aeternum_m3_dummy_opcodes.h`, `static_assert`ed against `Opcodes.h`).

| Case | Checks |
|---|---|
| ABI | FT growth by `struct_size` only, ABI 1, view = 32 bytes, NULL-safe entry points |
| Geometry | 1984/1671 slots, index = `GetOpcodeArrayIndex` for every group/id, movement/socket policy |
| HostAlloc unit | no crossing, pairing, caps, **orphan** (stack, module heap, interior pointer), stale/double free, foreign, spoof, tail/header corruption (leaked), reclaim/abandon, generation |
| Orphan dummy | module frees a pointer it never got from `host_alloc` -> no-op, Error `native_host_free_orphan`, tainted + armed |
| Double free dummy | second free -> `stale`, Error, tainted |
| Vanilla | no hooks: 0 published, fast path, dispatch no-op |
| Hooks clean | 8 published; 5th emote = `OVERFLOW`; movement/ping = `DENIED`; `on_start` subscribe = `DENIED`; **forced subscription after `on_init` fails** (re-invoked `on_init`, host-side call); dispatch in/out; DROP ignored; clean shutdown unpublishes + frees kept block |
| Hook fault | null write in a hook -> contained, quarantined, no re-entry, tainted, pump -> one `RequestShutdown(60, ...)`, process alive |
| Boot fault | `on_init` fault -> armed, pump issues the 60 s shutdown once |

Numbers: Master Report (`tests.exe "[m3][blocco2]"`, `"[m3]"`, `"[m1]"`, full suite on `build_m3`; logs `<ENGINE_ROOT>\_buildlogs\m3b2_*`). Linux: guard + runtime + `hooks` dummy compiled with clang (zig) on a Linux box and exercised (8 published, 4 emote hooks run, SIGSEGV in hook contained, 60 s shutdown requested once).

## 6. Not in Blocco2

Manifest-declared opcode list (subscription currently unrestricted inside `on_init` apart from the deny lists), CMSG `DROP` / SMSG cancel, thread-id allowlist (F4.1), DB MPSC ring, Linux CMake CI, deploy to isolata / production.

## 7. Rollback

Revert the Blocco2 local commit. With `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` (default) the dispatch calls reduce to a `count == 0` load per packet and nothing can publish.
