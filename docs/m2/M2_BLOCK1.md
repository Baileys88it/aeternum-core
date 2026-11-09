# IE-M2 Blocco1 — Protocol Profile loader + dense AdapterOpcodeTable

**Milestone:** IE-M2 Blocco1 (authorized by maintainer t654u + scope t655u)  
**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`), base tag `M1-Complete` (`a52e748262`)  
**IT:** [`M2_BLOCK1.it.md`](M2_BLOCK1.it.md) · **Design:** [`PROTOCOL_ADAPTER_DESIGN.md`](PROTOCOL_ADAPTER_DESIGN.md) · **ADR:** [IE-ADR-013](../adr/IE-ADR-013-protocol-profile-and-adapter.md)

Status: local commits only. No push, no deploy. **Zero behavior change**: the adapter is boot-only and is **not** wired into `WorldSocket` / `WorldSession` / `Opcodes` (no core file touched).

---

## 1. What was delivered

| Artifact | Path |
|---|---|
| Opcode table generator (documented, deterministic) | `contrib/aeternum/protocol/gen_opcode_table.py` |
| Generated opcode table (integrity-covered) | `forever/client_profiles/1.60.1.70790/opcodes/opcodes.json` |
| Unknown / ignored_known policy (integrity-covered) | `forever/client_profiles/1.60.1.70790/opcodes/unknown_policy.json` |
| Manifest `load_policy` + `integrity.files[]` | `forever/client_profiles/1.60.1.70790/manifest.json` |
| Byte-exact hashing on Windows checkouts | `forever/client_profiles/1.60.1.70790/opcodes/.gitattributes` (`*.json -text`) |
| Dense table + classification + rate limiter (hot path, header-only) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/opcode_table.hpp` |
| Boot-only Profile loader | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/profile_loader.hpp` |
| Structural lint (no map/set, no heap on hot path) | `contrib/aeternum/ci/check_adapter_no_map.ps1` → CMake target `aeternum-adapter-nomap-lint` (dependency of `tests`) |
| Catch2 `[m2][blocco1]` A–F | `tests/protocol_adapter/ProtocolAdapterBlocco1.cpp` |

## 2. Opcode table generation

`python contrib/aeternum/protocol/gen_opcode_table.py --repo .` (regenerate) · `--check` (CI: fails if `opcodes.json` or the manifest hashes are stale) · `--out <file>` (dry run, manifest untouched).

Inputs (read-only): `Opcodes.h` (enums), `Opcodes.cpp` (handler status), `ClassicClientOpcodes.inc` (687 overrides), `ClassicOpcodes.cpp` (server overrides, Blocked/Stub sets), profile `extra_inserts.conf`, `manifest.json` (`critical_client_opcodes`), `opcodes/unknown_policy.json` (`ignored_known`).

It ports `ClassicOpcodes::TranslateClientOpcode` / `TranslateServerOpcode` exactly for one build: `--shift 1` (Classic.OpcodeGroupShift for build ≥ 70291), `--client-insert-index 0x13B`, `--main-insert-index 0x98` (= production conf). Client side scans every wire candidate in the Classic groups and keeps those that land on a named CMSG (or on an `ignored_known` logical id); server side forward-translates every SMSG.

Row format: `[wire, logical, status, flags, name]` (one row per line, LF, deterministic). Flags: `1` handled · `2` critical · `4` blocked · `8` stub · `0x10` override · `0x20` alias · `0x40` ignored_known.

Counts for 1.60.1.70790:

| Key | Value |
|---|---|
| cmsg rows (wire→logical) | 1186 (189 aliases; 1185 named + 1 ignored_known) |
| cmsg handled (non-alias) | 610 |
| cmsg enum / unreachable | 1028 / 32 |
| smsg rows | 1410 (3 blocked, 4 wire collisions) |
| `.inc` overrides | 687 |
| ignored_known | 1 (`0x3D013D` ← wire `0x3E013D`) |

Alias rule (a logical id reached by several wires): preferred wire = shifted numbering > explicit override > highest wire. Only the preferred row feeds `cmsgLogicalToWire`; all rows feed `cmsgWireToLogical` (recv must accept both). Example: `CMSG_AUTH_SESSION` logical `0x440001` → preferred wire `0x460001`, alias `0x450001`.

## 3. Loader (boot-only)

`LoadProtocolProfile(profileDir, AdapterOpcodeTable&, ProfileLoadReport&)`:

1. `manifest.json` (≤ 64 KB): `profile_id`, `client_build`, `load_policy == "on-boot-only"`.
2. Integrity — same rules as M1 module manifests: `integrity.files[]` **XOR** legacy `{file, sha256}` (both → `integrity_ambiguous_form`), ≤ 64 entries, duplicate path rejected, `IsSafeIntegrityRelativePath`, hashing with M1 `Sha256FileHex` (streamed, OpenSSL).
3. `opcodes/opcodes.json` and `opcodes/unknown_policy.json` must be covered (`integrity_uncovered:<path>`).
4. Parse policy + rows; every declared count is re-checked; `ignored_known` section must match flagged rows.
5. Build the 4 dense maps. Any duplicate key / capacity overflow / malformed row fails the load.

Fail-closed: on any error the table is `Reset()` (never half-filled), `Loaded()` is false and `report.reason` is explicit:

| Reason | When |
|---|---|
| `integrity_sha256_mismatch:<path>` | file bytes ≠ manifest hash (tampered file or tampered hash) |
| `integrity_file_missing:<path>` | covered file absent |
| `integrity_file_unsafe:<path>` | absolute / `..` / non-portable path |
| `integrity_uncovered:<path>` | opcodes.json or unknown_policy.json not listed |
| `integrity_ambiguous_form` / `integrity_*` | M1 integrity form rules |
| `load_policy_unsupported` | anything other than `on-boot-only` |
| `opcodes_count_mismatch:<key>` / `opcodes_row_invalid:<sec>:<i>` / `opcodes_duplicate_key:<map>:0x…` / `opcodes_table_capacity:<map>` | data inconsistency |
| `unknown_policy_invalid:<field>` | policy out of bounds |

## 4. Dense table (structural constraint)

No `std::map` / `std::unordered_map` / `std::set` family anywhere in `src/aeternum/protocol_adapter/**` (lint). No heap in `opcode_table.hpp` (lint). Lookup is `noexcept`, branch-light, O(1):

```
key  = 0xGGIIII (24 bit)            key > 0xFFFFFF -> not found (out of range)
span = groups[GG]                   GroupSpan {base, minIndex, count}, 256 entries
off  = IIII - span.minIndex         unsigned: wraps when IIII < minIndex
off >= span.count -> not found      single unsigned compare covers both bounds
slot = slots[span.base + off]       OpcodeEntry {key, value, flags, status, cls} = 12 B
slot.flags & VALID -> hit           holes inside a span stay invalid
```

Why it handles large ids like `0x3D013D`: the table is indexed by *group* (top byte) and only the populated `[minIndex..maxIndex]` window of each group gets slots, so the 24-bit key space (16.7 M ids) collapses into a few thousand contiguous slots, with no hashing and no probing. Capacity is fixed at compile time (`kMaxSlotsPerMap = 4096`); the build is two-pass (NoteKey → Layout → Insert) and fails the load if a future profile does not fit.

Four maps per profile: `cmsgWireToLogical` (recv), `cmsgLogicalToWire`, `smsgLogicalToWire` (send), `smsgWireToLogical` (replay/sniff).

Measured footprint (70790, `sizeof` printed by test A):

| Item | Value |
|---|---|
| `OpcodeEntry` | 12 B |
| `DenseOpcodeMap` | 70 924 B (spans 3 KB + slots 48 KB + hit counters 16 KB + build scratch 2.3 KB) |
| `AdapterOpcodeTable` (4 maps + counters) | 283 800 B (≈ 277 KB), allocated once at boot |
| Slots used (cmsg wire / cmsg logical / smsg logical / smsg wire) | 2011 / 1937 / 1649 / 1652 of 4096 each |

## 5. Golden rule — ignored_known vs unknown

Each lookup ends in exactly one class:

| Class | Source | Decision | Side effects |
|---|---|---|---|
| `Handled` | named, active handler (or socket-level / STATUS_NEVER) | Dispatch | per-slot hit counter |
| `KnownUnhandled` | named, STATUS_UNHANDLED / IGNORED (smsg: blocked) | Dispatch (core decides as today) | per-slot hit counter |
| `IgnoredKnown` | `unknown_policy.json` `ignored_known[]` (integrity-covered) | **Drop** | per-slot hit + `IgnoredKnownTotal` — **ZERO log, ZERO I/O, ZERO heap** |
| `Unknown` | no slot (hole, absent group, or key > 0xFFFFFF) | **Drop** | `UnknownTotal` (+ `OutOfRangeTotal`) + **rate-limited ERROR** |

Unknown rate limiter (`unknown_policy.json` → `unknown`): `window_ms` 10000, `threshold_per_window` 50, `max_errors_per_window` 1.

- ≤ threshold unknowns in the window → counter only, **no log**.
- Above threshold → at most `max_errors_per_window` calls to the ERROR sink per window; the rest increment `ErrorsSuppressed`.
- The sink is a plain function pointer `(ctx, opcode, direction, countInWindow, unknownTotal)` installed at boot (no `std::function`, no heap); the host decides how to log. Time is injected (`nowMs`) so tests are deterministic.
- All counters are preallocated relaxed atomics.

`0x3D013D` (dec 3998013) is a **post-translation (logical) CMSG id** in the unnamed hole `0x3D013C..0x3D013F` (between `CMSG_SET_EMPOWER_MIN_HOLD_STAGE_PERCENT` 0x3D013B and `CMSG_CANCEL_GROWTH_AURA` 0x3D0140). The generator derives its wire `0x3E013D` (70245 step: group − 1). It is classified `IgnoredKnown`: dropped silently and counted. Research: `<DOCS_ROOT>\IE_M2\RESEARCH_0x3D013D.md`. To promote it to a real handler later: name it in `Opcodes.h`, remove it from `ignored_known`, regenerate.

## 6. Tests (`[m2][blocco1]`)

| Case | Asserts |
|---|---|
| A | load OK; both data files verified; table entries == declared counts; policy values; prints sizeof |
| B | tampered `opcodes.json` → `integrity_sha256_mismatch:opcodes/opcodes.json`; tampered manifest hash → same; missing file → `integrity_file_missing:opcodes/opcodes.json`; table left empty |
| C | wire `0x440070`→`0x430070`, `0x440073`→`0x430071`; reverse; AUTH_SESSION preferred `0x460001` + alias `0x450001`; SMSG_AUTH_CHALLENGE `0x4C0000`→`0x4E0000`; `0x3E013C` → `0x3D013B` (`.inc` override, not a hole); holes `0x440071/72`, `0x3E013E` → Unknown/Drop; `0x01000000`, `0xFFFFFFFF` → not found |
| D | `0x3D013D` (logical) and `0x3E013D` (wire) → IgnoredKnown/Drop over 10 000 packets each; counters = 20 000; sink called **0** times even with threshold 0 |
| E | 50 unknowns → 0 ERROR; +950 → exactly 2 (max 2/window), 948 suppressed; new window re-arms; no sink → no crash |
| F | out-of-range ids (wire + logical) → Unknown/Drop, `OutOfRangeTotal`, rate-limited; empty (not loaded) table → Unknown, no crash |

Run (separate build dir, never `build_iem1`): `<ENGINE_ROOT>\build_m2\bin\RelWithDebInfo\tests.exe "[m2]"` and `"[m1]"`.

Results: `[m2]` **6/6 test cases, 50 200 assertions passed**; `[m1]` **39/39 (258 assertions)**; full suite 175/175 (52 416 assertions). Build dir `<ENGINE_ROOT>\build_m2` (RelWithDebInfo, VS 2022).

Lint: `aeternum-adapter-nomap-lint` PASS on the tree; verified to FAIL (exit 1) when `#include <unordered_map>` / `std::map` is injected in the adapter tree or `new` in `opcode_table.hpp`; comments ignored.

## 7. Not in Blocco1 (next blocks)

- Wiring into `WorldSocket` recv/send (shadow mode with parity bit first) — Blocco2.
- Real log sink (`TC_LOG_ERROR`) and metrics export — with the wiring.
- Replacing the `.inc` `unordered_map` in core with the dense table — Blocco4.
- Auth flags from Profile — Blocco5.
