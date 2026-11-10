# ADR-003: Static modules first via ScriptMgr

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-003-module-architecture.md`](it/ADR-003-module-architecture.md)


## Problem
Need extensibility without unsafe dynamic unloading or a second script system.

## Solution
Static modules linked like today’s `src/server/scripts`, registered through ScriptMgr + future SDK façade; manifest + lifecycle. Native dynamic & WASM deferred with explicit criteria (`MODULE_ARCHITECTURE.md`).

## Why
Avoids duplicating TrinityCore script functionality; safest path (master prompt).

## Alternatives
Immediate .dll plugins — rejected early (ABI/unload risk).  
WASM-first — rejected (modern≠better; no proven need for pilot).

## Sources
- `ScriptMgr.h/.cpp` upstream pattern.  
- CppCoreGuidelines (RAII, ownership).  
- Wasmtime docs (for deferred eval): https://docs.wasmtime.dev/

## Complexity / Perf / Maintenance
Low–med; zero runtime overhead vs current scripts; clearer ownership.

## Tests
Module lifecycle unit tests; failed optional module must not kill server.

## Risks & rollback
Feature-flag SDK; scripts keep direct ScriptMgr path until migrated.

## Decision (Accepted M2 — 2026-11-05)

Static modules via ScriptMgr remain the first step. No change in M2 code.
