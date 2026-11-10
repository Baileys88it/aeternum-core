# ADR-008: Admin & Monitoring as first SDK module

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-008-admin-monitoring-pilot.md`](it/ADR-008-admin-monitoring-pilot.md)


## Problem
Need a concrete first module to validate SDK without endangering gameplay or launcher.

## Solution
Pilot **Administration & Monitoring**: localhost HTTP (default `127.0.0.1`), auth+RBAC+audit, read-only metrics/info, **no** arbitrary remote commands / raw SQL. Spec: `ADMIN_MONITORING_MODULE.md`.

## Why
maintainer chose this as first SDK pilot; security-bounded; exercises manifest/lifecycle/permissions.

## Alternatives
Playerbots first — higher gameplay risk.  
World events first — less SDK coverage of permissions.  
No pilot until SDK perfect — delays learning.

## Sources
- Security engineering practice (localhost bind, RBAC).  
- Existing Metric + `.online` command as data sources to wrap.  
- Launcher contract docs.

## Complexity / Perf / Maintenance
Med; low perf if scrape interval sane; ops-facing maintenance.

## Tests
Auth denial; bind localhost; audit entries; metrics presence; launcher unaffected.

## Risks & rollback
Module `critical:false`; unload/disable conf; no schema hard dependency in v0.

## Decision (Accepted M2 — 2026-11-05)

Admin & Monitoring pilot HTTP stack: **Boost.Beast** (Boost already a TrinityCore dependency — **no new libraries**). Bind **localhost only**; token auth; RBAC; audit log. **Implementation NOT in M2** (target with Module SDK / M5+).
