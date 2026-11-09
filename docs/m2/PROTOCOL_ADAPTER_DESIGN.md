# Protocol Adapter design — isolate Forever opcodes from core

**Milestone:** IE-M2 Discovery (branch `feat/m2-protocol`)  
**Date:** 2026-11-05 (Europe/Rome)  
**Worktree:** `<ENGINE_ROOT>\wt_m2` @ `545d9a2c93`  
**IT:** [`PROTOCOL_ADAPTER_DESIGN.it.md`](PROTOCOL_ADAPTER_DESIGN.it.md)  
**ADR:** [`../adr/IE-ADR-013-protocol-profile-and-adapter.md`](../adr/IE-ADR-013-protocol-profile-and-adapter.md)  
**Prior sketch:** `docs/independent_engine/architecture/modules/PROTOCOL_ADAPTER_ARCHITECTURE.md` (superseded for IE-M2 scope by this doc; keep as historical)

Stability first. Strangler extraction. One client build at a time. Zero heap on the per-packet path. No event bus on movement/auras until M3.

---

## 1. Goal

Make Forever wire numbers, ExtraInserts, group-shift heuristics, and build handshake flags **data of a Protocol Profile**, consumed through a **Protocol Adapter**, so a Forever client update does not require reviewing the whole core.

Core keeps:

- Semantic handlers (`WorldSession::Handle*`)
- Dense `opcodeTable` keyed by **logical/core** opcode ids
- Session / anti-DoS / crypt material after handshake

Adapter owns:

- `wire id ↔ logical/core id`
- Profile boot load + integrity
- Unknown-opcode policy + telemetry
- (Later blocks) layout capability flags; typed codecs for critical messages

---

## 2. Translation layer

### 2.1 Identity types (API)

Already frozen (headers only):

- `aeternum::protocol::WireOpcode` — raw Forever wire uint32
- `aeternum::protocol::CoreOpcode` — after ExtraInserts + group remap (still not a Trinity type)
- `IOpcodeRegistry::WireToCore` / `CoreToWire`
- `IClientProfile` — profile id, build, ExtraInserts

Discovery adds skeleton (no runtime wiring yet):

- `include/aeternum/api/protocol/adapter.hpp` — `LogicalOpcode` dense index, `IOpcodeAdapter` boot/build traits, unknown policy enum

**Logical vs Core:** for M2-Blocco1..2, `CoreOpcode.raw` remains the existing Trinity `OpcodeClient`/`OpcodeServer` numeric space (handlers already keyed there). A future clean-room logical index is optional and must not break `GetOpcodeArrayIndex` without a dedicated block.

### 2.2 Algorithm stages (recv)

Order must stay bit-identical to today (`ClassicOpcodes::TranslateClientOpcode`):

1. **RemoveClientExtraInserts** (profile insert list snapshot) — hole → invalid
2. **ExtraGroupShift** (connection group + main-group insert index)
3. **TranslateClientOpcode70245** (`.inc` map, else groupâˆ’1 fallback)
4. Validate against `opcodeTable` / unknown policy

Outbound inverse: 70245 server map + special cases → group shift → **AddServerExtraInserts**.

### 2.3 Dense preallocated lookup (no hash map on hot path)

| Structure | Build time | Packet path |
|---|---|---|
| ExtraInserts snapshot | `vector`/`array` sorted once at boot | linear over tiny N (2 client inserts today) — OK; optional bitset per group later |
| `ClassicClientOpcodes.inc` (687 entries) | Today: `unordered_map` in ClassicOpcodes | **M2 target:** boot-build dense per-group arrays or perfect hash; interim: keep map behind Adapter façade with measured budget |
| Handler table | Already dense `std::array` `NUM_CMSG_OPCODES=1984` | Keep |

**Hard rule (P-c):** no `new` / `malloc` / temporary `std::vector` growth on recv/send translate. Profile JSON parse only at boot.

**Implemented (M2-Blocco1):** `AdapterOpcodeTable` = 4 × `DenseOpcodeMap` (`GroupSpan[256]` + `OpcodeEntry[4096]` + relaxed hit counters), two-pass build at boot, O(1) `noexcept` lookup, ≈ 277 KB total (measured 283 800 B). `std::map` / `unordered_map` / `set` family is forbidden in `src/aeternum/protocol_adapter/**` and heap is forbidden in `opcode_table.hpp` — enforced by `aeternum-adapter-nomap-lint` (fails the `tests` build). Details: [M2_BLOCK1.md](M2_BLOCK1.md) §4.

### 2.4 Handler registration

- Unchanged: `OpcodeTable::Initialize` + `DEFINE_HANDLER` at process start.
- Adapter does **not** patch handlers at runtime.
- `IOpcodeRegistry::HandlerOf` (today always `Unhandled` in thin adapter) will bind to `opcodeTable` status in a later block — still boot-only.

---

## 3. Plug-in points (WorldSocket / session)

```text
Client
  → WorldSocket decrypt
  → Adapter::WireToCore (today: ClassicOpcodes::TranslateClientOpcode @ WorldSocket.cpp:412-413)
  → opcodeTable / WorldSession queue
  → Handle*

Core SMSG
  → Adapter::CoreToWire (WorldSocket.cpp:539/567/584 TranslateServerOpcode)
  → encrypt → Client
```

Bridge already seats ExtraInserts removal via `Forever::ProtocolSession` (`ClassicOpcodes.cpp:128-133`, `World.cpp:1286` InitAtStartup). M2 expands that seat to full translate + unknown policy without changing packet bytes when Profile matches production conf.

Auth/handshake flags (Profile-sourced, still WorldSocket):

- `Network.SkipBuildAuthKeyCheck` ← profile `handshake/flags.json` (`WorldSocket.cpp:747-761`)
- `Network.EnterEncryptedModeRegionGroup` ← profile (`:968-969`, `:1054-1055`)

---

## 4. Versioning / multi-profile

| Rule | Decision |
|---|---|
| Concurrent profiles | **No** (ADR-002 / M1 decision) |
| Flip build | New folder under `forever/client_profiles/<id>/`, point `ACTIVE`, restart |
| API surface | `IClientProfile` / `IOpcodeRegistry` stay Trinity-free |
| Conf override | Conf ExtraInserts remain fallback when AutoApply off; Profile wins when applied |

---

## 5. Unknown-opcode policy — golden rule (implemented in M2-Blocco1)

Every client opcode lookup ends in exactly one class (see [M2_BLOCK1.md](M2_BLOCK1.md) §5):

| Class | Decision | Side effects |
|---|---|---|
| `Handled` | Dispatch | per-slot hit counter |
| `KnownUnhandled` | Dispatch (core decides as today) | per-slot hit counter |
| `IgnoredKnown` (Profile `opcodes/unknown_policy.json` → `ignored_known[]`, integrity-covered) | **Drop** | counter only — **ZERO log, ZERO I/O, ZERO heap** |
| `Unknown` (hole, absent group, or key > 0xFFFFFF) | **Drop** | counter + **rate-limited ERROR** |

**Golden rule:** a *known* ignored opcode must never produce log noise; a truly *unknown* opcode must never be silent forever, but must never flood the log either.

Unknown rate limiter (Profile `unknown_policy.json` → `unknown`): `window_ms` 10000, `threshold_per_window` 50, `max_errors_per_window` 1. ≤ threshold in a window → counter only; above → at most `max_errors_per_window` ERRORs per window through a boot-installed sink (function pointer, no heap); the rest is counted as suppressed. Counters are preallocated relaxed atomics. Do **not** disconnect (preserve today's `WorldSocket.cpp:419-420` skip behavior).

`0x3D013D` (dec 3998013) is a **logical (post-translation) CMSG id** in an unnamed hole (wire `0x3E013D`, derived by the generator) → `IgnoredKnown`. Promotion path: golden capture → name it in `Opcodes.h` → remove from `ignored_known` → regenerate.

This supersedes the Discovery default `log_once_metric_drop`.

---

## 6. Telemetry

| Metric / log | Purpose |
|---|---|
| `protocol.unknown_opcode` | Forever drift detector |
| `protocol.remap.invalid_insert_hole` | Client hit an ExtraInsert hole |
| `protocol.profile.load` | Boot success/fail + profile id + build |
| Existing `network.opcode` debug | Keep; do not flood production |

No per-movement event bus (D4).

---

## 7. Perf budget

| Budget | Target |
|---|---|
| Wire→core translate | Match current Remap cost (± noise); Catch2 parity tests remain bit-identical |
| Allocations / packet | **Zero** on translate path |
| Boot profile load | < 100 ms local disk for 70790 package (soft) |
| Golden suite | Must stay green on any Adapter change |

Bench only when a block claims a win (D5 discipline from M1).

---

## 8. Risks (stability first)

| Risk | Mitigation |
|---|---|
| Silent wrong Remap (CREATE→pet-battle class of bug) | Critical gate Catch2 + real-client create-char smoke before profile activate |
| Dense-table rewrite bugs | Strangler: façade first, swap `.inc` map later behind flag |
| Conf vs Profile drift | Consistency check at boot; fail-closed option |
| Touching WorldSocket crypt path | Separate block; Profile only supplies RegionGroup/skip flags first |
| Scope creep into full codecs | Codecs only for CREATE/CHECK / auth-critical in early blocks |

---

## 9. Migration plan — M2 blocks

| Block | Scope | Tests | Rollback |
|---|---|---|---|
| **M2-Blocco1** | Profile JSON artifacts for 70790 (`opcodes/critical.json`, `handshake/flags.json`, `unknown_policy.json`); SHA-256 integrity reuse; docs already done | Catch2 load+hash; gate CREATE/CHECK unchanged | Delete new JSON; keep `extra_inserts.conf` |
| **M2-Blocco2** | Façade: `ClassicOpcodes::Translate*` calls only through Adapter seat (behavior identical); bind `HandlerOf` read-only | Remap goldens parity; `[protocol][goldens]` | Conf `Classic.ProtocolApiAdapter.Enable=0` |
| **M2-Blocco3** | Unknown policy + metric; log-once for `0x3D013D` | Unit + optional soak counter | Disable metric/policy flag |
| **M2-Blocco4** | Replace hot-path `.inc` unordered_map with dense per-group tables generated at boot | Perf smoke + parity | Feature flag revert to map |
| **M2-Blocco5** | Profile-sourced auth flags (SkipBuild / RegionGroup) with conf override | Boot assert profile vs conf; client login smoke on isolata | Conf override back |
| **M2-Blocco6+** | Layout capability flags; first typed codecs (CREATE/CHECK); golden packet replay harness | Corpus replay Catch2 | Codec off → legacy Packet Read |

No big-bang. No production deploy without isolata QA + maintainer OK.

---

## 10. Test strategy

| Layer | What |
|---|---|
| Catch2 Remap | Existing `[classic][extrainsects]`, `[classic][profile][active][gate]`, `[protocol][goldens]` |
| Golden corpus | `forever/client_profiles/1.60.1.70790/goldens/` — extend with live PKT captures (Aeternum only) |
| Replay | Synthetic JSONL first; later bytes-on-wire fixtures through Adapter only |
| Isolata QA | Login → world → logout → relogin on `_testinst_iem1` after each behavior-touching block |
| Regression bar | FATAL 0 boot; CREATE/CHECK gate green; no new handler-table status churn unless intentional |

---

## 11. Rollback

1. Feature flags / conf keys preferred over revert commits for runtime.
2. Keep previous profile folder immutable after publish.
3. Production: backup instance binaries + conf before any M2 deploy (same discipline as M1 Blocco4).
4. Git: `feat/m2-protocol` local commits; push/deploy only on maintainer order.

---

## 12. Out of scope (Discovery / early M2)

- Multi-emulator / non-Forever profiles
- Dynamic module opcode patching
- Event bus on MOVE/auras
- Clean-room rewrite of all Packets
- foreverdb.net scraping (not required for opcode Discovery)

---

## 13. M2-Blocco1 — delivered (local)

Boot-only Protocol Profile loader + integrity-covered `opcodes.json` (generated by `contrib/aeternum/protocol/gen_opcode_table.py`) + `unknown_policy.json` + dense `AdapterOpcodeTable` + golden-rule classification + rate-limited unknown + no-map lint + Catch2 `[m2][blocco1]` A–F. **No** WorldSocket / core change. See [M2_BLOCK1.md](M2_BLOCK1.md).
