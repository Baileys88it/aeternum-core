# IE-ADR-013: Protocol Profile package + Protocol Adapter seat

**Status:** Proposed (IE-M2 Discovery) — awaiting maintainer acceptance  
**Date:** 2026-11-05 (Europe/Rome)  
**Milestone:** IE-M2  
**IT:** [`it/IE-ADR-013-protocol-profile-and-adapter.md`](it/IE-ADR-013-protocol-profile-and-adapter.md)  
**Related:** ADR-002 (single active profile); IE-ADR-005 (Protocol Independence); M1 integrity SHA-256

## Problem

Forever wire ids, ExtraInserts, group-shift heuristics, auth handshake knobs, and layout `IsBuild*` forks are scattered across `ClassicOpcodes.*`, `Opcodes.*`, `WorldSocket.cpp`, conf keys, and gameplay TUs. A client bump risks silent remaps (70755→70790 CREATE incident class) and core-wide review.

## Decision

1. **Protocol Profile** (on-disk, versioned) is the source of truth for one Forever build under `forever/client_profiles/<id>/`, with machine-readable JSON + SHA-256 integrity (reuse M1 `integrity.files[]` model), boot-only load, single ACTIVE profile.
2. **Protocol Adapter** is the only seat that translates `WireOpcode ↔ CoreOpcode` on the WorldSocket path; core handlers stay on logical/core ids.
3. Hot path: dense preallocated structures; **zero heap**; no hash map requirement on steady-state translate (map allowed only behind transitional flag).
4. Unknown wires: log-once + metric + drop (no disconnect).
5. Migration by small M2 blocks with golden remap/replay tests; strangler, not big-bang.

## Consequences

- New Forever build = new profile folder + gates green + restart (not a core rewrite).
- `ClassicOpcodes.cpp` becomes an Adapter implementation detail, then shrinks as data moves to Profile.
- Conf keys remain override/fallback during transition.
- Production deploy of M2 blocks still requires isolata QA + explicit maintainer OK.

## Alternatives rejected

- Multi-profile runtime — complexity (ADR-002).
- Keep conf-only tribal ExtraInserts — proven failure mode.
- Big-bang opcode table rewrite — stability risk.

## Test bar

Catch2 CREATE/CHECK gate; Remap↔Adapter parity goldens; isolata real-client smoke before any behavior-changing block reaches `_public_iem1`.
