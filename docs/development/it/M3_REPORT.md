# Report M3 — Measurement

**Data:** 2026-11-05 (Europe/Rome)  
**EN:** [../en/M3_REPORT.md](../en/M3_REPORT.md)

## Ambito consegnato

1. Metric in-tree con **local sink** (`Metric.Sink=local`): aggregati 10s → 1m → 1h → 1d, rotazione, gzip opzionale (wrapper deflate; lo zlib Windows bundled non ha `gz*`), retention, budget **~75 MB**, limite cardinalità.
2. Strumentazione base: `world_tick_ms` / overrun, `map_updater_queue_wait_ms`, sessioni, pkt in/out, code DB per pool.
3. Report umano v0 → log + `latest_report.txt`.
4. **File giornaliero** → un `daily_YYYYMMDD.txt` ogni ~24h con avg/p95/max e riepilogo warn/error/fatal (conteggi per categoria + top messaggi; hook da `Log::OutMessageImpl` per WARN+). Incluso nel budget disco.
5. Test Catch2: aggregazione, budget, roundtrip, report, overhead, daily rollup + p95 — **8 casi / 41 assert, tutti OK**.
6. Default sicuro: `Metric.Enable=0`.

## Evidenza overhead (RelWithDebInfo, Catch2)

| Misura | Risultato |
|---|---|
| Aggregator 200 000 `Record` | **6837 µs** totali (**~0,034 µs/sample**) |
| CPU a ~20 serie × 20 Hz | **~0,014 ms/s** wall (≪ 0,01% di un core) |
| Byte/giorno (20 serie, tutte le finestre) | **~12,7 MB/giorno** upper teorico |
| Retention mista 14g (modello) | **~2,7 MB** — sotto **75 MB** |
| File daily (test sintetico) | **490 byte** (2 serie + errori) |

Nessuna nuova dipendenza esterna. Niente HTTP admin / Prometheus / per-opcode / Tracy.

## Non eseguito

`worldserver` live isolato **non** avviato: sullo host è già attivo uno stack Forever di produzione; senza copia DB e porte dedicate il rischio di conflitto è alto. Il comportamento metriche è coperto da Catch2.

## Fuori da M3

HTTP admin, Prometheus, top-N per-opcode, Tracy.

## Verifica live (2026-11-05 Europe/Rome)

Copie MySQL isolate su 3308: `auth` (~1,5 MB), `characters` (~3,8 MB), `hotfixes` (~425,6 MB), `world` (~355,5 MB dopo import; 755 tabelle come la sorgente). Schema di produzione invariati.

`bnetserver` + `worldserver` Aeternum (RelWithDebInfo) con `Metric.Enable=1`, sink local, bind `127.0.0.1`, porte alternative **2119 / 18083 / 18085**. Avvio pulito; ready in ~10 s; nessun crash in soak **≥20 min**.

| Voce | Valore |
|---|---|
| Soak | ~20 min (01:30–01:50 Europe/Rome) |
| File metriche | 121×10s + 20×1m + report ≈ **285 KB** (≪ 75 MB) |
| Stima disco | ~**20 MB/giorno** prima dell’eviction |
| Report umano | OK; soglie catalogo quiet da idle |
| Daily file | Non in 20 min (serve ~24 h); coperto da Catch2 |
| DBErrors | **6650** righe (vs riferimento prod **7562**) |
| Login client | **Non possibile headless** |
| CPU idle | ~1 core-equivalent sul processo world |

Dopo: Aeternum stop; produzione riavviata via task; `Logger.network.opcode` **1→3** (backup, UTF-8 no BOM).
