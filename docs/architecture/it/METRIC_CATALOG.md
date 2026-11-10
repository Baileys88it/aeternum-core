# Catalogo metriche (operazioni M3)

**English:** [../en/METRIC_CATALOG.md](../en/METRIC_CATALOG.md)

## Abilitazione (default sicuri)

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

Impostare `Metric.Enable = 1` solo su un processo Aeternum isolato. Non condividere porte o database con un altro emulator in produzione.

## Serie registrate in M3

| Nome | Unità | Note |
|---|---|---|
| world_tick_ms | ms (double, sotto-ms) | Tempo wall di World::Update (steady_clock come millisecondi double) |
| world_tick_overrun_total | count | +1 se il tick supera il budget |
| world_update_time_total / world_update_time | ms | Timer esistenti (label type; map_id rimosso) |
| update_time_diff | ms | Diff del tick |
| map_updater_queue_wait_ms | ms | MapUpdater::wait |
| map_update_time_diff | ms | Update per mappa (map_id **rimosso**) |
| sessions_active / online_players | gauge | Intervallo overall status |
| pkt_in_packets / pkt_out_packets | count/flush | Per finestra di flush |
| db_queue_login/character/world/hotfixes | gauge | Profondità coda |

## Output

- `Logs/metrics/10s_*.mlines`, `1m_*`, `1h_*`, `1d_*` (+ `.gz` se compress)
- `Logs/metrics/latest_report.txt` — report umano v0
- **`Logs/metrics/daily_YYYYMMDD.txt`** — un file per giorno UTC: `name|labels|count|avg|p95|max`, più totali warn/error/fatal, conteggi per categoria e top messaggi. Conta nel budget ~75 MB / retention.
- Logger category `metric`

p95: approssimazione a reservoir (32 campioni); se vuoto usa max.

## Stima byte/giorno

~3–7 MB/giorno a risoluzione 1m prima del roll-up; con retention 14 giorni e `MaxSeries=256` resta sotto **75 MB**.

## M8 dense scrape (cardinalita limitata)

```ini
Metric.DenseScrape = 1
Metric.Dense.SamplePercent = 10
Metric.Local.MaxSeries = 256
Metric.Local.BudgetMB = 75
```

Serie aggiuntive: `opcode_handler_ms`, `db_query_ms`, `script_map_update_ms`. Endpoint admin `/metrics/dense` e `/metrics/store`. Misurare overhead in soak isolato.

## Note M9 � percorso local metric store
- I file vanno in `Metric.Local.Directory` (soak: `<ENGINE_ROOT>/_run/logs/metrics`), **non** `_run/metrics`.
- Risoluzioni: `10s_*.mlines`, `1m_*.mlines`, `1h_*.mlines`, `1d_*.mlines`, pi� `daily_YYYYMMDD.txt` e `latest_report.txt`.
- `/metrics/store` espone `directory`, `directory_abs`, `file_count`, `bytes`, budget/retention, gauge dense.
- Falso allarme M8: la verifica guardava `_run/metrics` mentre il sink scriveva correttamente in `_run/logs/metrics`.
