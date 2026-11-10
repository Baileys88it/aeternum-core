# Metric catalog (M3 operations)

**Italian:** [../it/METRIC_CATALOG.md](../it/METRIC_CATALOG.md)

## Enable (safe defaults)

```ini
Metric.Enable = 0
Metric.Sink = local
Metric.Local.Directory = Logs/metrics
Metric.Local.BudgetMB = 75
Metric.Local.RetentionDays = 14
Metric.Local.FlushSeconds = 10
Metric.Local.Compress = 0
Metric.Local.MaxSeries = 256
Metric.Threshold.world_tick_ms = 100
```

Set `Metric.Enable = 1` on an isolated Aeternum-only process to collect. Do **not** share live production ports or databases with another running emulator instance.

## Series recorded in M3

| Name | Unit | Notes |
|---|---|---|
| world_tick_ms | ms (double, sub-ms) | Full World::Update wall time (steady_clock duration as double milliseconds) |
| world_tick_overrun_total | count | +1 when tick > budget |
| world_update_time_total / world_update_time | ms | Existing timers (type label kept; map_id stripped) |
| update_time_diff | ms | Tick diff argument |
| map_updater_queue_wait_ms | ms | MapUpdater::wait |
| map_update_time_diff | ms | Per-map update (map_id **stripped** for cardinality) |
| sessions_active / online_players | gauge | Overall status interval |
| pkt_in_packets / pkt_out_packets | count/flush | Per flush window |
| db_queue_login/character/world/hotfixes | gauge | Queue depth |

## Outputs

- `Logs/metrics/10s_*.mlines`, `1m_*`, `1h_*`, `1d_*` (+ `.gz` if compress)
- `Logs/metrics/latest_report.txt` — human report v0 (top-N + catalog hints)
- **`Logs/metrics/daily_YYYYMMDD.txt`** — one file per UTC day: `name|labels|count|avg|p95|max` for all series, plus warn/error/fatal totals, counts by category, and top messages. Included in the ~75 MB budget / retention.
- Logger category `metric`

p95 is a reservoir approximation (32 samples per series through roll-ups); if the reservoir is empty it falls back to max.

## Bytes/day estimate

Rough model: ~3–7 MB/day at 1m resolution before roll-up; mixed 10s/1m/1h/1d + one daily summary stays under a **75 MB** hard budget with 14-day retention when cardinality stays near `MaxSeries=256`.

## M8 dense scrape (bounded)

Optional runtime instrumentation (conf, no cmake `WITH_DETAILED_METRICS` required):

```ini
Metric.DenseScrape = 1
Metric.Dense.SamplePercent = 10
Metric.Local.MaxSeries = 256
Metric.Local.BudgetMB = 75
Metric.Local.RetentionDays = 14
```

| Name | Labels | Notes |
|---|---|---|
| `opcode_handler_ms` | `opcode=<handler>` | Sampled client-opcode handler wall time; aggregator MaxSeries + admin TopN |
| `db_query_ms` | `db=<schema>` | MySQL Execute/Query wall time per connection schema |
| `script_map_update_ms` | `map_id=<id>` | `ScriptMgr::OnMapUpdate` cost (top-N via MaxSeries) |

Admin scrape:

- `GET /metrics` — gauges + latest window
- `GET /metrics?dense=1` or `/metrics/dense` — same, Top-N by |sum| (`Admin.MetricsTopN`, default 20)
- `GET /metrics/store` — `{bytes,budget_bytes,retention_days,max_series,dense,dense_samples,dense_cpu_ns}`

Overhead evidence: `aeternum_metric_dense_samples_total`, `aeternum_metric_dense_cpu_ns_total`, store bytes vs `BudgetMB`. Measure on isolated soak before enabling on shared prod.

## M9 notes � local metric store path
- Files are written under `Metric.Local.Directory` (soak default: `<ENGINE_ROOT>/_run/logs/metrics`), **not** `_run/metrics`.
- Resolutions: `10s_*.mlines`, `1m_*.mlines`, `1h_*.mlines`, `1d_*.mlines`, plus `daily_YYYYMMDD.txt` and `latest_report.txt`.
- `/metrics/store` JSON includes `directory`, `directory_abs`, `file_count`, `bytes`, budget/retention, dense gauges.
- M8 false alarm: verification looked at `_run/metrics` while the sink correctly used `_run/logs/metrics`.
