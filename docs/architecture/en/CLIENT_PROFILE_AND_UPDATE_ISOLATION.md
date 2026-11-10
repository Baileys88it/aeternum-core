# Client profile & update isolation (M4 foundation)

**Status:** Foundation (M4) + ExtraInserts auto-apply (M6)  
**Italian:** [`../it/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md`](../it/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md)  
**Policy:** Exactly one active Forever profile per deployment (ADR-002).

## What M4 formalizes

1. **Versioned profile package** at `forever/client_profiles/<product>.<build>/` with pointer file `forever/client_profiles/ACTIVE`.
2. **C++ foundation** under `src/server/game/Forever/`:
   - `ClassicOpcodeRemap.h` — pure ExtraInserts remap (test + future production share)
   - `ClientProfile.h/.cpp` — active id from conf, ExtraInserts file load, seam logging
3. **Documented seams** (opcode + hotfix) under each profile’s `seams/` folder.
4. **Must-not-break gate:** Catch2 `[classic][profile][active][gate]` fails if ACTIVE profile inserts break CREATE/CHECK.
5. Conf key `Classic.ActiveClientProfile` (documented in `worldserver.conf.dist`).

## Not in M4

- Hot-swap without restart
- Multi-profile runtime
- Hot-swap ExtraInserts without restart (M6 delivers startup auto-apply only)
- Frozen Module SDK (see MODULE_SDK_SURFACE.md — draft only)

## Flip workflow (restart-bound)

1. Add `forever/client_profiles/<new>/` with manifest, ExtraInserts, seams, tests.
2. Extend Catch2 expectations / keep ACTIVE gate green for the current ACTIVE id.
3. Real-client character-create smoke (mandatory).
4. Point `ACTIVE` + `Classic.ActiveClientProfile` + conf ExtraInserts + auth `build_info` / `gamebuild` together.
5. Restart worldserver (120s IT+EN on production Trinity when applicable).
