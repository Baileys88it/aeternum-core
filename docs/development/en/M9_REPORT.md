# M9 Report -- Ops harden + metric store path evidence

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Commit:** `67ebac5702` -- *M9: ops harden + metric store path evidence* (30 files, +1013/-65)  
**IT:** [`../it/M9_REPORT.md`](../it/M9_REPORT.md)

## Mapping

| Source | Label |
|---|---|
| Original roadmap | M9 = Release engineering (packaging/installers -- **partially** covered) |
| Our sequence | Ops: crash dump budget, config schema / `--check-config`, extended health, optional operator drain; plus metric-path evidence fix |

## FIRST: metric store persistence (M8 false alarm)

| Fact | Evidence |
|---|---|
| Conf directory | `Metric.Local.Directory` points at `_run/logs/metrics` (not `_run/metrics`) |
| Pre-soak disk | **907** files / **1830168** bytes (`m9_soak.log` PRE_METRICS) |
| Store JSON T0 | bytes **1834289**, file_count **910**, directory + directory_abs exposed |
| Store JSON T20 | bytes **2143304**, file_count **1062** |
| Disk at T20 | **1062** files / **2143304** bytes -- **exact match** to store `bytes` / `file_count` |
| Resolutions seen live | `10s_*` (913), `1m_*` (148), `latest_report.txt` (1) -- listing at end of soak |
| Sink log | WARN `Metric local sink enabled` with abs path (Server.log during soak) |

`1h_*`, `1d_*`, and `daily_YYYYMMDD.txt` were **not** observed live (soak ~22 min, idle).

## Delivered

| Item | Note |
|---|---|
| Crash dump budget | `Crash.Directory` / `BudgetMB` / `RetentionDays`; Enforce on startup; Catch2 `WriteTestMarker` (safe) |
| Config schema | Required FAIL-fast; unknown WARN on Aeternum-owned prefixes; `worldserver --check-config` |
| Extended `/health` | DB ping world/characters/login, script count, module list (read-only) |
| Operator drain | **Disabled by default**; separate operator token; POST `/admin/drain`; min delay 120s; IT+EN announce; audit; no world mutations |
| Docs EN+IT | CONFIG_SCHEMA, CRASH_DUMP_BUDGET, ADMIN_MONITORING / METRIC_CATALOG updates |

## Build & tests

| Check | Result |
|---|---|
| Catch2 `[admin],[metric],[crash],[config]` | **All tests passed (150 assertions in 23 test cases)** -- `_buildlogs/m9_tests.log` |
| Aggregator smoke (warning only) | 200k Record: **7228 us** (~0.03614 us/sample) -- same log |
| `--check-config` | **OK** (`required_failures=0 unknown_warned=0 issues=0`) -- `_buildlogs/m9_check_config.log`, exit 0 |
| Soaked binaries (final) | `worldserver.exe` LastWriteTime 05:32:19; `bnetserver.exe` 05:32:22 Europe/Rome (RelWithDebInfo) |

## Live soak (Aeternum alt ports)

| Check | Result |
|---|---|
| Countdown 120s IT+EN | YES -- `_buildlogs/m9_countdown_evidence.log` + `stop_prod_m9.log`; `STOP_PROD_DONE 2026-11-05 05:36:07 Europe/Rome` |
| Prod before stop | bnet PID 10764 / world PID 21876 |
| Aeternum | bnet **21048** (2119 / 18083), world **6040** (18085 / 9100) |
| Soak duration | **21.79 min** (start 05:36:54, T20 05:58:41 Europe/Rome) |
| `/health` sample | 200 with `db.world/characters/login=true`, `scripts=5909`, module `admin.monitoring` v0.3.0 started |
| Drain RBAC | POST with readonly token -> **403** `rbac_drain`; POST with operator token -> **200** `{"scheduled":true,"delay_sec":120}` |
| Crash marker | soak wrote `test_marker_soak.marker` under configured crash dir (file count 1); Catch2 budget test separate |
| Crash | none observed during soak |
| Prod after cycle | bnet **20904** / world **23048** (ready in Server.log) |

## Explicitly out of scope / not verified

- Real game-client login -- **not verified**
- 72-hour soak -- **not run**
- Live `1h` / `1d` / `daily_*` metric files -- **not produced** in this soak
- Full release packaging / signed installers (remainder of MASTER M9)
- Wheaty real crash dump under load -- **not intentionally triggered** (test hook / marker only)

## Actions

GitHub Actions left **disabled** (`enabled:false`); push to `aeternum/main` only.

## Follow-up: remote client login test (2026-11-05)

See [CLIENT_TEST_REPORT.md](CLIENT_TEST_REPORT.md) for LoginREST cert + Network.SkipBuildAuthKeyCheck fixes, restore notes, and the unhandled-opcode backlog from maintainer's session.
