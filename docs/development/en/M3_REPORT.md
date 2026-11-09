# M3 Report — Measurement

**Date:** 2026-11-05 (Europe/Rome)  
**IT:** [../it/M3_REPORT.md](../it/M3_REPORT.md)

## Scope delivered

1. In-tree Metric extended with **local sink** (`Metric.Sink=local`, default when enabled): aggregates 10s → 1m → 1h → 1d, rotation, optional gzip (deflate gzip wrapper; bundled Windows zlib has no `gz*`), retention, **~75 MB** budget, max series cardinality.
2. Base instrumentation: `world_tick_ms` / overrun, `map_updater_queue_wait_ms`, `sessions_active`, `pkt_in/out_packets`, DB queue depths (login/character/world/hotfixes).
3. Human report v0 → log + `Logs/metrics/latest_report.txt` (top-N + catalog hints).
4. **Daily summary** → one `Logs/metrics/daily_YYYYMMDD.txt` per ~24h with avg/p95/max for all series plus warn/error/fatal counts by category and top messages (hooked from `Log::OutMessageImpl` for WARN+). Counts toward the disk budget.
5. Catch2 tests: aggregation, budget, roundtrip, report, overhead smoke, daily rollup + p95 reservoir — **8 cases / 41 assertions, all passed**.
6. Default remains **safe**: `Metric.Enable=0`.

## Overhead evidence (RelWithDebInfo, Catch2)

| Measurement | Result |
|---|---|
| Aggregator 200 000 `Record` | **6837 µs** total (**~0.034 µs/sample**) |
| CPU at ~20 series × 20 Hz | **~0.014 ms/s** wall (≪ 0.01% of one core) |
| Bytes/day (20 series, keep all 10s+1m+1h+1d) | **~12.7 MB/day** theoretical upper |
| Bytes retained (mixed: ~1h of 10s + 1d of 1m + 14d of 1h/daily) | **~2.7 MB** model — well under **75 MB** |
| Daily file (synthetic test) | **490 bytes** for 2 series + error summary |

No new external dependencies. No admin HTTP / Prometheus / per-opcode / Tracy.

## Not run

Live `worldserver` against an isolated DB/ports was **not** started: a production Forever stack is already bound on this host; spinning a second worldserver without a prepared DB copy and distinct ports would risk conflict. Metrics behavior is covered by Catch2.

## Not in M3

Admin HTTP, Prometheus, per-opcode top-N, Tracy.

## Live verification (2026-11-05 Europe/Rome)

Isolated MySQL copies on port 3308: `auth` (~1.5 MB), `characters` (~3.8 MB), `hotfixes` (~425.6 MB), `world` (~355.5 MB tablespace after import; same 755 tables as source). Production schemas left unchanged.

Aeternum `bnetserver` + `worldserver` (RelWithDebInfo) ran with `Metric.Enable=1`, `Metric.Sink=local`, bind `127.0.0.1`, alternate ports **2119 / 18083 / 18085**. Startup clean; realm ready in ~10 s; no crash over **≥20 min** soak.

| Metric | Value |
|---|---|
| Soak | ~20 min (01:30–01:50 Europe/Rome) |
| Metric files end | 121×10s + 20×1m + `latest_report.txt` ≈ **285 KB** total (≪ 75 MB) |
| Extrapolated disk | ~285 KB / 20 min → ~**20 MB/day** order before retention eviction |
| Human report | Produced; catalog thresholds quiet on idle |
| Daily `daily_YYYYMMDD.txt` | Not in 20 min window (needs ~24 h); covered by Catch2 `[metric][local][daily]` |
| DBErrors (Aeternum load) | **6650** lines (mostly TABLE/CREATURE/SPELL locale/template noise; vs prior prod reference **7562**) |
| Client login | **Not possible headlessly** — no Forever client automation in this run |
| CPU (idle soak) | ~1.0 core-equivalent on world process (MapUpdater threads); metrics I/O negligible vs game loop |

After soak: Aeternum stopped; production restarted via scheduled tasks; `Logger.network.opcode` set **1→3** (TRACE→INFO) with backup, UTF-8 no BOM.
