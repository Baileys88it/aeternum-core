# Data & update management (M6)

**Status:** Implemented (M6 v0)  
**Italian:** [`../it/DATA_AND_UPDATE_MANAGEMENT.md`](../it/DATA_AND_UPDATE_MANAGEMENT.md)  
**Roadmap:** MIGRATION_PLAN **M6 = Data & update management**  
**Priority link:** advances **client-update system** (profile package is source of ExtraInserts truth).

## Mapping note

Our delivered sequence pulled Admin monitoring forward as **M5** (MASTER listed it under M8).  
**This M6** follows the roadmap label **Data & update management** and is the next small step after M4’s profile foundation: operators no longer must hand-sync conf ExtraInserts when a profile is active.

## What M6 delivers

1. **`Classic.ProfileExtraInserts.AutoApply`** (`-1` = auto when `Classic.ActiveClientProfile` set): load `forever/client_profiles/<id>/extra_inserts.conf` at world startup; **profile wins** over conf lists.
2. **`Classic.ProfileDataRoot`**: absolute path to the tree containing `forever/client_profiles/` (soak/dev: source root).
3. **Consistency checks** at apply time:
   - `ACTIVE` pointer vs `Classic.ActiveClientProfile` (warn on drift)
   - `manifest.json` `extra_inserts` arrays vs `extra_inserts.conf` (warn on mismatch)
4. **ClassicOpcodes** consume `Forever::Effective*ExtraInserts()` so remap uses the applied profile.
5. **Catch2** `[classic][profile][m6][data]`
6. **Backup/restore drill** checklist: [`../../development/en/BACKUP_RESTORE_DRILL.md`](../../development/en/BACKUP_RESTORE_DRILL.md)

## Explicitly out of M6

- Hot-swap ExtraInserts without restart
- Auto-write conf files on disk
- Hotfix/SQL applicator UI
- Mutating admin endpoints
- New third-party dependencies (uses in-tree rapidjson)

## Flip workflow (unchanged restart bound)

1. Add/update profile package under `forever/client_profiles/<new>/`.
2. Keep Catch2 ACTIVE gate + M6 data tests green.
3. Point `ACTIVE` + `Classic.ActiveClientProfile` (+ auth `build_info` / `gamebuild`) together.
4. Restart worldserver (120s IT+EN on production Trinity when applicable).  
   ExtraInserts conf lines become documentation/fallback when AutoApply is on.
