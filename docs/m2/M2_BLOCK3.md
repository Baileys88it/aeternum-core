# IE-M2 Blocco3 — ProtocolAdapter Dispatch Switch (Legacy Detach)

**Milestone:** IE-M2 Blocco3 (authorized by maintainer, t659u)  
**Date:** 2026-11-06 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). **Local commit only — no push.**  
**Production:** `_public_iem1` remains on **Blocco2 Dark Launch** (Shadow Mode, legacy dispatch). This block is local + isolata QA only.  
**IT:** [`M2_BLOCK3.it.md`](M2_BLOCK3.it.md) · **Blocco2:** [`M2_BLOCK2.md`](M2_BLOCK2.md)

## 1. What changed

Primary inbound dispatch in `WorldSocket::ReadDataHandler` uses **only** the ProtocolAdapter dense-table O(1) result. Shadow Mode compare is removed. `ClassicOpcodes::TranslateClientOpcode` is gone from the inbound critical path (also from the oversized-HOTFIX header gate, which now peeks the adapter table without counters).

| Artifact | Path |
|---|---|
| Inbound translate (pure, header-only, hot path, linted no-map/no-heap) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/shadow.hpp` |
| Worldserver glue (boot load + native logger sink + TranslateInbound) | `src/server/game/Server/Protocol/AdapterShadow.h/.cpp` |
| Per-packet dispatch | `src/server/game/Server/WorldSocket.cpp` — `ReadDataHandler` + HOTFIX size gate in `ReadHeaderHandler` |
| Boot load (once; config reload = no-op) | `src/server/game/World/World.cpp` — after `Forever::LogClientProfileSeams` |
| Catch2 `[m2][blocco3]` K–N | `tests/protocol_adapter/ProtocolAdapterBlocco3.cpp` (replaces Blocco2 shadow-compare tests) |

SMSG / `TranslateServerOpcode` / `IsServerOpcodeBlocked` are **unchanged** (out of scope).

## 2. Inbound decisions (`TranslateInboundClientWire`)

| Action | Condition | Effect |
|---|---|---|
| `Drop` | table not loaded | fail closed: no lookup, no counter, **no log**, **no ClassicOpcodes fallback** |
| `Drop` | `IgnoredKnown` (`0x3D013D` / wire `0x3E013D`) | counter only, **never a log**, stop the chain (no queue) |
| `Drop` | `Unknown` / OOR | counter + **rate-limited** ERROR (`unknown_policy.json`); stop the chain |
| `Dispatch` | `Handled` / `KnownUnhandled` | use adapter logical id for the existing core handler table |

## 3. Logging

- **Unknown rate limiter → native logger:** `TC_LOG_ERROR("network.opcode", "Aeternum ProtocolAdapter unknown opcode rate limit: …")`. Policy: window 10000 ms / threshold 50 / max 1 ERROR per window.
- **ignored_known:** zero lines (golden rule), even if threshold is 0.
- **Boot success:** `TC_LOG_WARN("server.loading", "Aeternum ProtocolAdapter: ACTIVE profile … (dispatch=adapter ProtocolAdapter)")`.
- **Boot failure:** `TC_LOG_FATAL` (empty profile / missing root / load failed) — fail closed; every inbound packet drops until fixed.

## 4. Production note

Do **not** deploy Blocco3 to `_public_iem1` in this phase. Public stays on Blocco2 Dark Launch (`a81a15c1a181`, ports 2119/18083/18085) collecting live shadow parity. Isolata QA must use **isolated ports** (typically 3119/28083/28085) so public is never displaced.

## 5. Rollback

Revert the Blocco3 local commit. Runtime without rebuild is not available for Blocco3 (no legacy fallback): a missing profile FATAL-logs at boot and drops all inbound packets.

## 6. QA

Catch2 and isolata results are recorded in the Master Report.