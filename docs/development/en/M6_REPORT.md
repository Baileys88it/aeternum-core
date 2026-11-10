# M6 Report — Data & update management (profile ExtraInserts auto-apply)

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**IT:** [`../it/M6_REPORT.md`](../it/M6_REPORT.md)

## Mapping

| Source | Label |
|---|---|
| MIGRATION_PLAN | **M6 = Data & update management** |
| Our sequence | M4 = client profile foundation; M5 = Admin monitoring (pulled forward from MASTER M8) |
| This milestone | Profile-driven ExtraInserts auto-apply + consistency checks + backup/restore drill — next step for **client-update system** |

## Delivered

| Item | Location |
|---|---|
| Auto-apply ExtraInserts | `Forever::ApplyActiveProfileDataAtStartup` + `Effective*ExtraInserts` |
| Conf | `Classic.ProfileDataRoot`, `Classic.ProfileExtraInserts.AutoApply` (-1 auto) |
| Drift checks | ACTIVE vs conf; manifest.json vs extra_inserts.conf (rapidjson, already in tree) |
| ClassicOpcodes | uses effective inserts |
| Catch2 | `[classic][profile]` 25 assertions / 7 cases PASS (includes `[m6][data]`) |
| Docs EN+IT | `DATA_AND_UPDATE_MANAGEMENT.md`, `BACKUP_RESTORE_DRILL.md` |
| Backup drill | dump `auth.realmlist` → restore scratch `aeternum_m6_drill` COUNT=1 → dropped |

## Live soak

| Check | Result |
|---|---|
| Countdown 120s IT+EN | YES — `_buildlogs/m6_countdown_evidence.log` + `stop_prod_m6.log`; Server.log `Halting process...` |
| Aeternum | world 9968 / bnet 25012; ports 18085 / 2119 / 18083; ≥20 min |
| Metrics | world_tick_ms avg ~0.075 ms; no crash |
| Prod restart | bnet **20192** (1119), world **14980** (8085) |

## Out of scope

Hot-swap without restart; mutating admin; new deps; SQL hotfix applicator.
