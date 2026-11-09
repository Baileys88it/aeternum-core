# Observability & Performance Strategy

**Status:** Accepted (M2 decisions) â€” measurement before optimization; **low disk footprint**  
**Italian:** [`../it/OBSERVABILITY_AND_PERFORMANCE.md`](../it/OBSERVABILITY_AND_PERFORMANCE.md)  
**ADR:** [`../../adr/ADR-007-observability.md`](../../adr/ADR-007-observability.md)

## Goal (maintainer)

The core must be able to **register any server metric** needed to understand precisely how the server is doing, in a form that is **easy to read** and makes clear **where to improve** â€” while occupying **very little disk space**.

## Current state

| Signal | Today |
|---|---|
| Logs | `Logger.*` |
| Metric | In-tree `src/common/Metric` (Influx line) â€” typically disabled |
| Tracy / OTel / Prometheus | Not integrated |
| Baseline M0 | Process WS/PM; world init ~7s; ticks **UNMEASURED** |

## Design principles

1. **Capability first, noise never** â€” the engine can instrument everything below; defaults keep cardinality and I/O tiny.
2. **Aggregates, not events** â€” counters / gauges / histograms rolled up on fixed intervals (`10s` â†’ `1m` â†’ `1h` â†’ `1d`). No per-event metric logging by default.
3. **Readable output** â€” periodic human summary (top-N hotspots + â€œwhere to improveâ€ hints) **and** (later) Prometheus text via admin localhost.
4. **Hard disk budget** â€” default **~75 MB** total for metric stores (configurable 50â€“100 MB); size-based rotation + retention.
5. **modern â‰  better** â€” start with existing TrinityCore Metric â†’ local file/log; no external services until justified.
6. **Measure overhead** â€” compile-time and/or conf toggles; sampling; explicit CPU/RAM overhead budget before enabling dense sets in production.

## Implementation milestone (proposed)

| Phase | Milestone | Work |
|---|---|---|
| Docs + catalog | **M2** | Strategy, naming catalog, size model |
| Enable + baseline file sink | **M3 (landed)** | Local sink + base instrumentation + human report v0 â€” see METRIC_CATALOG.md |
| Dense catalog + admin scrape | **M5â€“M8** | Per-opcode top-N, DB pools, script/AI cost; Prometheus text on admin module localhost; rotation/retention enforcement |

Cutover still requires the maintainer's explicit approval (ADR-009).

## Instrumentation capability catalog

Each metric below is **in scope** for the core. Names are proposals (`aeternum_*` / reuse Metric categories where they already exist).

| Name | Unit | Meaning | Healthy (starting guess) | If bad â€” where to look |
|---|---|---|---|---|
| `world_tick_ms` | ms | World `Update` duration | p99 < tick budget (e.g. 50â€“100 ms @ 1Hz goals TBD) | World loop; script hooks; map queue backlog |
| `world_tick_overrun_total` | count | Ticks exceeding budget | ~0 sustained | Same; reduce work / defer non-critical |
| `map_update_ms` | ms | Per-map update (hist, labels=`map_id` **top-N only**) | p99 within budget | Heavy creatures/scripts on that map |
| `map_updater_queue_wait_ms` | ms | Time tasks waited on MapUpdater | low steady | Thread count vs map load; lock contention |
| `sessions_active` | gauge | Connected game sessions | expected pop | Network/auth issues if cliff |
| `session_latency_ms` | ms | Sampled RTT/latency | playable band | Client net; world stall |
| `opcode_count` | count/interval | Handled opcodes (**top-N** by count) | stable mix | Unexpected spike â†’ abuse or remap bug |
| `opcode_handler_ms` | ms | Handler time (**top-N**) | no single opcode dominates tick | That handler / packet layout |
| `pkt_in_bytes` / `pkt_out_bytes` | bytes/interval | Wire volume | proportional to pop | Flood / broadcast storms |
| `pkt_in_packets` / `pkt_out_packets` | count/interval | Packet rate | proportional to pop | Same |
| `db_query_ms` | ms | Query latency sample per pool | p99 small | Slow SQL; missing index; lock |
| `db_queue_depth` | gauge | Async queue depth per pool | near 0â€“low | Pool threads; query storms |
| `map_count` / `grid_active` / `creature_active` | gauge | World size signals | stable for content | Leaks; failed unload |
| `script_cost_ms` | ms | ScriptMgr / hooks cost | small fraction of tick | Hot scripts |
| `ai_cost_ms` / `smartai_cost_ms` | ms | AI / SmartAI cost | bounded | Pathing; update frequency |
| `spell_cost_ms` / `aura_cost_ms` | ms | Spell/aura subsystems | bounded | Aura explosion; periodic ticks |
| `memory_rss_bytes` / `memory_ws_bytes` | bytes | Process memory | flat after warm-up | Leaks; cache growth |
| `threads_alive` | gauge | Thread count | stable | Runaway pools |
| `crash_total` / `fatal_log_total` | count | Crashes / fatals observed by wrappers | 0 | Crash dumps; last metrics flush |

**Cardinality rules:** no per-player / per-GUID labels; opcodes and maps use **top-N** (e.g. 20) per interval; everything else low-cardinality (`pool=Login|Character|World`, `result=ok|error`).

## Low-overhead controls

- Conf / compile toggles: `Metric.Enable`, detail tiers (`core`, `net`, `db`, `scripts`, `opcodes_topn`).
- Sampling: e.g. 1/N sessions for latency; opcode timing sampled.
- Histograms with few buckets (fixed).
- Detailed tracing (Tracy / verbose logs): **on-demand only**, time-boxed (e.g. max 15â€“60 min), never default in prod.

## Storage & disk budget (very little space)

### Aggregation

| Interval | Role |
|---|---|
| 10s | Hot buffer (in memory) |
| 1m | Flushed aggregates |
| 1h / 1d | Roll-ups for trends |

No per-event metric log lines by default.

### Format

Compact line or binary blob per interval; optional compression (e.g. zstd) on rotated files. Human summary is a **small** separate text/markdown report (KB, not GB).

### Rotation & retention

- Size-based rotation + max retention days.
- **Hard disk budget default: 75 MB** total under the metrics directory (configurable 50â€“100 MB). When exceeded: drop oldest roll-ups first; never grow unbounded.

### Size estimate (order-of-magnitude)

Assumptions: ~40 series after top-N limits; 1m flush of ~120 bytes/series compressedâ‰ˆ40 bytes; 10s kept only in RAM.

| Item | Estimate |
|---|---|
| 1 minute raw aggregate set | ~2â€“5 KB |
| 1 day @ 1m resolution | ~3â€“7 MB |
| 1 day after 1h roll-up + compression | ~0.5â€“1.5 MB |
| 14 days retained (mixed 1m recent + 1h/1d older) | **~20â€“40 MB** |
| Human summary reports (14Ã— daily) | < 1 MB |
| **Budget headroom to 75 MB** | comfortable for spikes / extra series |

If the catalog grows, **cut cardinality** before raising the budget.

## Readable outputs

1. **Periodic human summary** (e.g. every 5â€“15 min + daily): top-N `map_update_ms`, top-N opcodes, DB pool p99, tick overruns, memory delta, and short â€œwhere to improveâ€ hints derived from thresholds in the catalog.
2. **Prometheus text** (later): `GET /metrics` on admin module **localhost only** (ADR-008) â€” no external push until justified.
3. Existing Metric file/log sink as the first local backend (M3).

## Performance acceptance

Measure â†’ find bottleneck â†’ baseline â†’ hypothesis â†’ small change â†’ same bench â†’ compare â†’ correctness â†’ document. Never claim wins without numbers.

## M3 local sink outputs

See [METRIC_CATALOG.md](METRIC_CATALOG.md). Includes compact .mlines windows, latest_report.txt, and one daily_YYYYMMDD.txt per day (avg/p95/max + error/warn summary) under the ~75 MB budget.

## M8 landed (admin harden + dense scrape)

Dense opcode/DB/script samples are **opt-in** (`Metric.DenseScrape`) with `SamplePercent` and `MaxSeries` caps under the **~75 MB** local budget. Retention continues to delete aged files then oldest-by-mtime until under budget. Admin exposes store bytes/budget and dense CPU counters for soak evidence. See [METRIC_CATALOG.md](METRIC_CATALOG.md) and [ADMIN_MONITORING_MODULE.md](ADMIN_MONITORING_MODULE.md).

## M9
Extended /health, metric store path exposure, crash dump budget, config schema/--check-config, optional operator drain.

