# Classic 1.60.1.70532 (local, preliminary)

Local branch `forever-70532` for the Aeternum server. Same scheme as 70450 (commit e3abffa1e7):

- `sql/custom/auth/2026_10_24_00_auth_classic_beta_70532.sql`, taken unchanged from the public upstream PR
  advocaite/TrinityCore#121 (same file name, so the updater does not apply it twice once #121 is merged):
  `build_info` row 70532 (1.60.1) and realms 70-73 moved from 70450 to 70532.
- No auth key is public for Win-x64-WoWB 70532 (none in TrinityCore, the advocaite fork, PR #121 or the
  "WoW Forever Bots" source 0.1.22, which also ships no key): keep `Network.SkipBuildAuthKeyCheck = 1`.
- No C++ change: `ClassicOpcodes::IsBuild70291OrLater` / `IsBuild70425OrLater` compare `gamebuild >=`, so a realm on
  70532 uses the 70425/70450 protocol and update field layouts.
- DB2 check (static read of the extracted files): all 370 DB2 loaded by the server are WDC5 and their layout hash
  equals `DB2Metadata.h`; 0 missing, 0 mismatches.
- Protocol compatibility is not verified (the client code is encrypted on disk). Login problems on Windows with some
  launchers are tracked in advocaite/TrinityCore#116.
- bnetserver reads `build_info` only at startup: restart it after the update.