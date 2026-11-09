# IE-M3 Blocco3 - Wiring Finale (WorldSocket observe + CMake default ON)

**Branch:** `feat/m3-runtime`  
**Scope:** local commit on top of Blocco2 `106389396ee3`. **No public deploy in this commit.** Isolata `_testinst_iem1` only after build PASS.  
**Production / `_public_iem1`:** untouched this phase (no backup, no restart, no merge, no `M3-Complete` tag).

## 1. CMake default

| Switch | Value | Notes |
|--------|-------|-------|
| CMake `AETERNUM_NATIVE_MODULE_RUNTIME` | **ON** (was OFF) | worldserver links `aeternum-native-runtime` |
| `WITH_DYNAMIC_LINKING` | **OFF** | unchanged; Trinity HotSwap never used for SDK modules |
| worldserver.conf `Aeternum.Modules.NativeRuntime` | **0** default; isolata set to **1** empty | compiled ON + config 1 + zero native modules = empty runtime |

## 2. WorldSocket wiring (observe-only)

- **Inbound:** `WorldSocket::ReadDataHandler` after M2 `TranslateInbound` + `SetOpcode`, in the game-packet default branch (session authed, opcode has a handler), **before** `QueuePacket`. Socket-level CMSG (PING/AUTH/...) never reach this site; subscribe still denies movement `0x41` / `0x5E` and socket-level.
- **Outbound:** `WorldSocket::SendPacket` before `TranslateOutbound` (unchanged site). `WritePacketToBuffer` does **not** fire hooks (would double-fire).
- **No double-fire:** `WorldSession::Update` no longer calls inbound hooks (Blocco2 helper removed).
- Fast path with empty table / unpublished dispatch: one acquire load of `count == 0` per packet; zero heap; no crash.

## 3. Config / schema

- Known key catalog: `Aeternum.Modules.NativeRuntime` added next to ManifestDir / RequireManifest.
- Isolata: `Aeternum.Modules.NativeRuntime = 1` with no native `"runtime":"native"` modules under ManifestDir (empty scan).

## 4. QA - Catch2 `[m3][blocco3]`

- Empty dense table: `DispatchInbound` / `DispatchOutbound` are no-ops (hook counters stay 0).
- Regression: `[m3][blocco1]`, `[m3][blocco2]`, `[m1]`, `[m2]`, full suite as feasible.
- Lint: `check_abi_no_stl.ps1`, `check_adapter_no_map.ps1` (WorldSocket.cpp on hot-path list).

## 5. Isolata boot evidence (this phase)

- Ports `3119` / `28083` / `28085`, admin `127.0.0.1:9200`.
- Banner SHA = this Blocco3 commit; FATAL=0; native runtime ON/empty log line.
- Public `2119` / `18083` / `18085` / admin `9300` must remain the prior banner and PIDs.

## 6. Deferred (not this commit)

- Push of this Blocco3 commit (await authorization).
- Public Dark Launch, merge to `aeternum/main`, tag `M3-Complete`.
- Opcode allowlist from manifest; DROP verdict; SMSG cancel.
