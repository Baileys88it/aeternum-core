# ADR-004: Versioned Module SDK (semver)

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-004-module-sdk.md`](it/ADR-004-module-sdk.md)


## Problem
Scripts include broad game headers; no API/ABI version contract for future modules.

## Solution
Independently versioned **Aeternum Module SDK** (semver): stable headers, deprecation policy, manifest `sdk` range, compile-time + runtime checks. First consumer: Admin & Monitoring.

## Why
Priority #1 with client profiles; enables dual maintenance growth without tangle.

## Alternatives
“Just use ScriptMgr forever” — insufficient for permissions/lifecycle.  
Full plugin ABI year-1 — too early.

## Sources
- SemVer: https://semver.org/  
- ScriptMgr as initial backend.  
- Our conclusion: façade over ScriptMgr, don’t replace it in M5.

## Complexity / Perf / Maintenance
Med; negligible perf; requires discipline.

## Tests
SDK version mismatch rejected; sample module builds against SDK-only headers.

## Risks & rollback
SDK major bumps rare; modules pin ranges; freeze SDK during emergencies.

## Decision (Accepted M2 — 2026-11-05)

Versioned Module SDK remains Priority #1 with client-update. Implementation not in M2.
