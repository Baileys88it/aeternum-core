# Classic 1.60.1.70707 (WoW Classic Forever, local)

Local branch `wf-70707` (from `wf-combined-20261028`) for the Aeternum server. Not pushed.

Client: Battle.net product `wowf` (was `wow_classic_beta`), folder `_forever_` (was `_classic_beta_`),
`Wow.exe` (was `WowB.exe`), `.flavor.info` = `wowf`. Builds released after 70532 (wago.tools, product wowf):
70630, 70634, 70658, 70685 (2026-10-28) and 70707 (2026-10-30, build config 7feb61f3...).

Changes:

- `SpellDuration.db2` changed layout in 70707: `0xA931BD2B` (3 fields) -> `0x4FDF0951` (4 fields,
  `DurationPerLevel` back between `Duration` and `MaxDuration`; WoWDBDefs). Updated `DB2Structure.h`,
  `DB2Metadata.h`, `DB2LoadInfo.h` and the `HOTFIX_SEL_SPELL_DURATION` query. The server does not use the new
  field. All other DB2 loaded by the server keep their layout hash (staging extraction compared with the 70532 files).
  Consequence: this worldserver needs the 70707 `dbc` folder, and the 70532 worldserver needs the 70532 one.
- `Realm.AlsoAcceptBuilds` (bnetserver.conf) ported from upstream advocaite/TrinityCore df12238185 (C++ and
  conf.dist only, without its sql/custom file). Default empty. Only list builds with the same DB2 layouts:
  70630/70634/70658/70685 share the 70532 layouts, but NOT the 70707 SpellDuration layout, so leave it empty
  unless tested.
- SQL in `sql/custom` (NOT sql/custom, so the worldserver updater does not apply it on its own):
  - `2026_11_01_00_auth_classic_70707.sql`: build_info rows 70630, 70634, 70658, 70685, 70707 and realms 70-73 -> 70707.
  - `2026_11_01_01_auth_classic_70707_rollback.sql`: realms back to 70532.
  - `2026_11_01_02_hotfixes_spell_duration_70707.sql`: `hotfixes.spell_duration.DurationPerLevel` column.
- No auth key is public for 70707: no `build_auth_key` row, keep `Network.SkipBuildAuthKeyCheck = 1`.
- Protocol: `ClassicOpcodes::IsBuild70291OrLater` / `IsBuild70425OrLater` compare `gamebuild >=`, so a 70707
  realm keeps the 70425/70450 opcodes and update fields. Not verified against the client (code encrypted on disk);
  public reports say third-party 70707 servers kept working with the same protocol.
- Extractors: default CASC product is now `wowf` (`-p wow_classic_beta` for the old client).
  The extractor tooling (not included) extracts into the staging folder `<SERVER_ROOT>\data_70707`.