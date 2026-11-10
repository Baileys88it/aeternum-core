# M8 Report -- Harden admin / observability on Module SDK

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Commit:** `3665019d46` -- *M8: harden admin/observability on Module SDK* (23 files, +734/-257)  
**IT:** [`../it/M8_REPORT.md`](../it/M8_REPORT.md)

## Mapping

| Source | Label |
|---|---|
| Original roadmap | M8 = Administration & observability |
| Our sequence | Harden M5/M7 admin + local metrics (top-N, dense scrape, token/audit) |

## Delivered

| Item | Note |
|---|---|
| Top-N cost series | `TakeTopNBySum` + `Metric.Local.MaxSeries` (conf 256) |
| Dense scrape | `Metric.DenseScrape` + `Metric.Dense.SamplePercent` + `RecordDense` (opcode / DB / script paths) |
| Budget / retention | Local store budget **75 MB** (`budget_bytes=78643200`), retention days **14** (from live `/metrics/store`) |
| Token hardening | Constant-time compare, `Admin.TokenMinLength`, conf-reload rotation, 401 rate limit |
| Audit | who / what / when / result / duration; rotate |
| Endpoints | Still **read-only** GET admin; `/metrics/dense`, `/metrics/store` |
| Docs EN+IT | ADMIN_MONITORING_MODULE, METRIC_CATALOG, OBSERVABILITY updates |

## Build & tests

| Check | Result |
|---|---|
| Intermediate Catch2 log | `_buildlogs/m8_tests.log`: **17 passed / 1 failed** (91 assertions; failure = short-token RBAC vs new min-length 16) -- **pre-fix** run |
| Post-fix Catch2 (full suite) | **All tests passed (1841 assertions in 110 test cases)** -- `_buildlogs/m8_catch2_full.log` (2026-11-05 ~09:36 Europe/Rome, RelWithDebInfo after banner rebuild; covers admin/metric/config + full tree) |
| Later verification | M9 Catch2 filter `[admin],[metric],[crash],[config]` **150 assertions / 23 cases PASS** (`m9_tests.log`) includes fixed admin/metric coverage |

## Live soak (Aeternum alt ports)

| Check | Result |
|---|---|
| Countdown 120s IT+EN | YES -- `_buildlogs/m8_countdown_evidence.log` + `stop_prod_m8.log`; `STOP_PROD_DONE 2026-11-05 04:55:40 Europe/Rome` |
| Prod before stop | bnet PID 15408 / world PID 16116 |
| Aeternum | bnet **11676** (2119 / 18083), world **16084** (18085 / 9100) |
| Soak duration | **21.67 min** (`m8_soak.log`: start 04:56:57, T20 05:18:37 Europe/Rome) |
| Health | 200 `{"status":"ok"}` |
| Dense / store T0 | bytes **1518897**, dense_samples **40**, dense_cpu_ns **358200**, budget_bytes **78643200**, max_series **256** |
| Store T20 | bytes **1828208**, dense_samples **50**, dense_cpu_ns **440700** |
| Overhead (from soak log) | bytes_delta **309311**; **bytes_per_day_est=20554128** (~20.6 MB/day); **dense_cpu_ms_per_day_est=29.285**; budget_use_pct **2.3247** |
| Auth fail sample | three wrong-token GETs -> HTTP **401** (rate-limit 429 not hit at 3 attempts) |
| Crash | none observed |
| Prod after cycle | bnet **10764** / world **21876** |

## Metric store path -- M8 false alarm

During M8 end-of-soak verification the directory `_run/metrics` was reported missing. **Root cause (confirmed in M9):** files were already being written under configured `Metric.Local.Directory` = `_run/logs/metrics` (not `_run/metrics`). M8 soak still showed growing `GetLocalStoreBytes()` / Prometheus store gauges; on-disk persistence was later proven with matching `file_count` in M9.

## Explicitly out of scope / not verified

- Real game-client login -- **not verified**
- 72-hour soak -- **not run**
- Live production of `1h_*` / `1d_*` / `daily_YYYYMMDD.txt` metric files -- **not produced** in ~22 min idle soak (only `10s_*` / `1m_*` / `latest_report.txt` expected at that duration; M9 disk listing confirms this pattern)
- Mutating admin / remote bind

## Actions

GitHub Actions left **disabled**; push to `aeternum/main` only.