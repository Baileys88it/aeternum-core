# Forbidden includes — Independent Engine public surfaces

**Status:** Accepted with IE-M0 owner decisions (2026-11-05)  
**Policy owner:** IE-ADR-004 / IE-ADR-010

## Hard fail (CI local script)

Paths:

- `include/aeternum/api/**`
- `src/modules/sdk/include/**`

Forbidden tokens:

- Types: `Player*`, `WorldSession*`, `Map*`, `Unit*`, `Creature*`, `WorldObject*`
- Headers: `Player.h`, `WorldSession.h`, `Map.h`, `Unit.h`, `Creature.h`, `WorldObject.h`, `World.h` (public SDK only)

Script: `contrib/aeternum/ci/check_forbidden_includes.ps1`

## Known debt (reported, not failed yet)

- `src/modules/sdk/src/ModuleHost.cpp` — host still Trinity-coupled until Module extraction.
- `src/modules/admin_monitoring/**` — may include game headers until Operations ports land.

Do not expand debt. Clear it as those milestones extract.
