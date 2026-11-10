# Module SDK surface draft (M4)

**Status:** Draft — **API not frozen** (`AETERNUM_SDK_API_DRAFT`)  
**Italian:** [`../it/MODULE_SDK_SURFACE.md`](../it/MODULE_SDK_SURFACE.md)  
**Code:** `src/modules/sdk/include/aeternum/sdk/`

## Public vs internal

| Path | Audience | Promise |
|---|---|---|
| `aeternum/sdk/version.h` | Everyone | Version macros; draft until 1.0.0 |
| `aeternum/sdk/module.hpp` | Module authors (future) | Identity / lifecycle / entry draft |
| `aeternum/sdk/internal/host_hooks.hpp` | Engine only | Unstable host seams |

## Relation to existing module system

`src/modules/` already builds optional static modules via `trinity_add_module` + ScriptMgr.
The SDK draft does **not** replace that path; it documents the future façade (ADR-004) without locking headers.

## Freeze criteria (later)

- Admin & Monitoring pilot builds against public headers only
- Semver policy published; `AETERNUM_SDK_API_DRAFT` removed
- At least one deprecation cycle practiced on a minor bump

See also: [MODULE_SDK_GUIDE.md](MODULE_SDK_GUIDE.md)
