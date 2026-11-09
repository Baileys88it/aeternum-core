# IE-M2 Blocco2 — ProtocolAdapter Shadow Mode in WorldSocket

**Milestone:** IE-M2 Blocco2 (authorized by maintainer, t656u)  
**Date:** 2026-11-06 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). Blocco1 pushed (`origin/feat/m2-protocol` = `89bc307a00`); Blocco2 is a **local** commit.  
**IT:** [`M2_BLOCK2.it.md`](M2_BLOCK2.it.md) · **Blocco1:** [`M2_BLOCK1.md`](M2_BLOCK1.md) · **Design:** [`PROTOCOL_ADAPTER_DESIGN.md`](PROTOCOL_ADAPTER_DESIGN.md)

## 1. What changed

Shadow Mode = **observation only**. For every inbound packet `WorldSocket::ReadDataHandler` keeps dispatching on the legacy
`ClassicOpcodes::TranslateClientOpcode` result. In parallel the raw wire id is resolved through the dense
`AdapterOpcodeTable` (O(1), zero heap) and the two results are compared. The adapter never changes dispatch and never drops a packet.

| Artifact | Path |
|---|---|
| Shadow compare (pure, header-only, hot path, linted no-map/no-heap) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/shadow.hpp` |
| Worldserver glue (boot load + native logger sinks) | `src/server/game/Server/Protocol/AdapterShadow.h/.cpp` |
| Per-packet hook | `src/server/game/Server/WorldSocket.cpp` — `WorldSocket::ReadDataHandler()` right after `TranslateClientOpcode` |
| Boot load (once; config reload = no-op) | `src/server/game/World/World.cpp` — after `Forever::LogClientProfileSeams` in `LoadConfigSettings` |
| Game include dirs (private) | `src/server/game/CMakeLists.txt` |
| Lint hot-path list += `shadow.hpp` | `contrib/aeternum/ci/check_adapter_no_map.ps1` |
| Catch2 `[m2][blocco2]` G–J | `tests/protocol_adapter/ProtocolAdapterBlocco2.cpp` |

## 2. Verdicts (`ShadowCompareClientWire`)

| Verdict | Condition | Effect |
|---|---|---|
| `Inactive` | table not loaded (profile missing / integrity fail / `Classic.ActiveClientProfile` empty) | no lookup, no counter, **no log** |
| `Match` | adapter logical == legacy logical | counter |
| `IgnoredKnown` | profile `ignored_known` (`0x3D013D`, wire `0x3E013D`) | counter only, **never a log, never a mismatch** (golden rule) |
| `BothUnknown` | no adapter row and no core handler for the legacy id | unknown counter + **rate-limited** ERROR (`unknown_policy.json`) |
| `Mismatch` | adapter logical != legacy, or adapter unknown while the core has a handler | counter + ERROR line (bounded per window) |

`legacyHasHandler = opcodeTable.IsValid(opcode) && opcodeTable[opcode] != nullptr`. Dispatch id = `ShadowDispatchOpcode(r)` = legacy, always.

## 3. Logging (native `TC_LOG_*`)

- **Mismatch:** `TC_LOG_ERROR("network.opcode", "Aeternum ProtocolAdapter shadow mismatch raw=0x… legacy=0x… adapter=0x…|UNKNOWN (dispatch=legacy)")`
  (`TC_LOG_FATAL` when `NDEBUG` is not defined, i.e. Debug builds; log only, no abort). At most
  `kShadowMismatchLogsPerWindow` (10) lines per `window_ms` (10000); every mismatch is still counted.
- **Unknown rate limiter → native logger:** the table sink is `NativeUnknownSink` →
  `TC_LOG_ERROR("network.opcode", "Aeternum ProtocolAdapter unknown opcode rate limit: wire=0x… count_in_window=N (> threshold 50 per 10000 ms) unknown_total=N")`.
  Policy from `opcodes/unknown_policy.json`: window 10000 ms, threshold 50, max 1 ERROR per window.
- **ignored_known:** zero lines. The legacy WorldSocket lines "sent wrong opcode" / "No defined handler" are skipped **only** when the adapter
  classified the packet as `ignored_known`; the legacy dispatch outcome is unchanged (packet skipped, session alive). All other legacy log lines are unchanged.
- **Boot:** `TC_LOG_WARN("server.loading", "Aeternum ProtocolAdapter shadow: ACTIVE profile … sha256 files=2 …")` or an explicit inactive / load-failed line.

## 4. Profile location at runtime

`Forever::ResolveProfileDataRoot()` (`Classic.ProfileDataRoot`, then cwd walk) + `Forever::ProfileDirectoryRel(Classic.ActiveClientProfile)`.
The directory must contain the Blocco1 files (`manifest.json` with `integrity.files[]`, `opcodes/opcodes.json`, `opcodes/unknown_policy.json`).
If the root points at a tree without them the loader fails closed and the shadow stays `Inactive` (legacy behavior, no per-packet log).

## 5. QA (Catch2, build_m2 RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`)

| Tag | Cases | Assertions | Result |
|---|---|---|---|
| `[m2][blocco2]` | 4 (G, H, I, J) | 230058 | PASS |
| `[m2][blocco1]` | 6 (A–F) | 50200 | PASS |
| `[m2]` | 10 | 280258 | PASS |
| `[m1]` | 39 | 258 | PASS |
| full suite | 179 | 282474 | PASS |

- **G** mismatch → exactly one ERROR line with raw/legacy/adapter, dispatch = legacy; adapter-unknown + core handler = mismatch; both-unknown ≠ mismatch; mismatch storm bounded per window.
- **H** ignored_known through shadow + native-style sink, threshold 0 → 0 lines, never a mismatch (even with a diverging legacy id).
- **I** unknown burst on the native-style sink with the real profile policy (10000/50/1) → silent ≤ 50, then exactly 1 ERROR per window.
- **J** unloaded table → `Inactive`, no crash, no log, no counters; worldserver glue without boot load returns false.

Lint `aeternum-adapter-nomap-lint`: PASS (3 adapter files, no associative containers, hot path heap-free).

Isolated-instance and real-client results are recorded in the Master Report, not here.

## 6. Rollback

Revert the Blocco2 commit. Runtime kill switch without rebuild: point `Classic.ProfileDataRoot` at a tree without the Blocco1 files
(shadow `Inactive`, legacy behavior identical to M1-Complete).
