# ADR-002: Single active Forever client profile

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-002-client-compatibility.md`](it/ADR-002-client-compatibility.md)


## Problem
ClientExtraInserts and packet edits are global conf/code; wrong values remap unrelated opcodes (70790 create → pet-battle).

## Solution
**One active client profile per deployment.** Profile package holds manifest, ExtraInserts, opcode expectations, packet adapter notes, auth/hotfix expectations, tests. Flip build = activate next profile (restart-OK initially). Sources: `ClassicOpcodes.cpp`, conf keys, `auth.build_info`.

## Why
maintainer decision; makes N→N+1 a localized repeatable change; matches “isolate what changes.”

## Alternatives
Multi-profile runtime — rejected (complexity).  
Conf-only tribal knowledge — rejected (proven failure mode).

## Sources
- In-tree: `ClassicOpcodes.cpp`, NOTES charcreate 70790.  
- Official: none for Forever wire (proprietary); process inspired by versioned protocol adapters generally.  
- Our conclusion: profile + mandatory create-char gate.

## Complexity / Perf / Maintenance
Med–high to implement (M4); remaps already O(inserts); maintenance drops after tooling.

## Tests
Unit remap; conf validation; **real client character-create smoke** mandatory.

## Risks & rollback
Keep previous profile artifacts; conf flip back; SQL `build_info`/`gamebuild` revert with rollback scripts.

## Decision (Accepted M2 — 2026-11-05)

**Path:** `forever/client_profiles/1.60.1.70790/` (and future ids). Single active profile per deployment. Manifest + ExtraInserts + opcode table + auth notes + packet notes + tests. Runtime loader deferred to **M4**; M2 ships data-only + validator + Catch2 remap skeleton.
