# Module Architecture (M1)

**Status:** Proposed  
**Pilot module:** Administration & Monitoring  
**Italian:** [`../it/MODULE_ARCHITECTURE.md`](../it/MODULE_ARCHITECTURE.md)

## Problem

Extensibility today = compile-in C++ under `src/server/scripts` + `ScriptMgr` hooks. No versioned SDK, no manifest, no isolated failure domain for optional features.

## Solution (phased)

### Phase A — Static modules (M5 first deliverable)

- Modules are still linked into worldserver (like current scripts).  
- Each module ships a **manifest** + registers through a thin **SDK façade** over `ScriptMgr`.  
- Existing scripts remain valid; migration is opt-in.

### Phase B — Native dynamic modules (deferred)

Allowed only when: stable ABI, documented lifecycle, no hot-unload requirement, version checks at load. **C++ unload while callbacks live is unsafe** — not an early goal.

### Phase C — Sandboxed WASM (deferred / investigate only)

Adopt only with demonstrated isolation benefit vs static modules; Wasmtime evaluated in M0 register (`v50.0.1`). Requires host API allow-list + resource limits. **Not** for Admin pilot.

## Manifest schema (Proposed)

```yaml
id: admin.monitoring
version: 0.1.0
sdk: ">=1.0.0 <2.0.0"
engine_api: ">=1.0.0"
runtime: static          # static | native | wasm
critical: false
dependencies: []
optional_dependencies: []
permissions:
  - metrics.read
  - process.info.read
  - admin.http.localhost
init_order: 100
database_migrations: []
config_schema: admin_monitoring.json
```

## Lifecycle

```text
DISCOVER → VALIDATE → RESOLVE_DEPS → INITIALIZE → REGISTER_SERVICES
    → ACTIVATE → RUNNING → DEACTIVATE → SHUTDOWN
```

| Stage failure | Behavior |
|---|---|
| VALIDATE / DEPS | Abort module; if `critical:false`, server continues |
| INITIALIZE / ACTIVATE | Same; log + metric `module_start_failed` |
| RUNNING errors | Module-local; must not terminate process unless critical |
| SHUTDOWN failure | Log; continue shutting down others |

## SDK stability (Proposed)

- Semver on SDK headers.  
- Deprecations: warn ≥1 minor; remove only on major.  
- Modules must not include private engine headers (allow-list).  
- Compile-time `#if AETERNUM_SDK_VERSION` checks + runtime manifest check.

## Relation to ScriptMgr

**Sources today:** `src/server/game/Scripting/ScriptMgr.h/.cpp`.  
SDK wraps registration macros/hooks; does not fork a second script system (avoid duplication — master prompt §3.16).
