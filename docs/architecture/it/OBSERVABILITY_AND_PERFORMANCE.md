# OsservabilitÃ  e performance

**Stato:** Accettato (decisioni M2) â€” misurare prima di ottimizzare; **poco spazio disco**  
**EN:** [`../en/OBSERVABILITY_AND_PERFORMANCE.md`](../en/OBSERVABILITY_AND_PERFORMANCE.md)  
**ADR:** [`../../adr/it/ADR-007-observability.md`](../../adr/it/ADR-007-observability.md)

## Obiettivo (maintainer)

Il core deve poter **registrare qualsiasi metrica del server** utile a capire con precisione come sta andando, in forma **facilmente leggibile** e che indichi **dove migliorare**, occupando **pochissimo spazio** su disco.

## Stato attuale

Log `Logger.*`; Metric Influx in-tree di solito spenta; Tracy/OTel/Prometheus non integrati; tick ancora UNMEASURED (baseline M0).

## Principi

1. CapacitÃ  ampia, rumore minimo.  
2. Solo aggregati (10sâ†’1mâ†’1hâ†’1d), niente log per-evento di default.  
3. Output leggibile: report umano periodico (top-N + suggerimenti) e piÃ¹ avanti Prometheus text via admin localhost.  
4. Budget disco rigido: default **~75 MB** (range 50â€“100 MB), rotazione per size + retention.  
5. modern â‰  better: prima Metric esistente â†’ file/log locale; niente servizi esterni senza giustificazione.  
6. Overhead misurato; toggle conf/compile; tracing dettagliato solo on-demand a tempo limitato.

## Milestone di implementazione (proposta)

| Fase | Milestone | Lavoro |
|---|---|---|
| Docs + catalogo | **M2** | Strategia e catalogo |
| Enable + sink locale | **M3 (completato)** | Sink locale + strumentazione base + report umano v0 â€” vedi METRIC_CATALOG.md |
| Catalogo denso + admin | **M5â€“M8** | top-N opcode, DB, script/AI; Prometheus su admin localhost; enforcement rotazione |

## Catalogo (capacitÃ )

Stesso elenco della versione EN: `world_tick_ms`, overrun, map update/queue, sessioni/latenza, opcode count/time (top-N), pkt in/out, latenza/coda DB per pool, conteggi map/grid/creature, costi script/AI/SmartAI, spell/aura, memoria, thread, crash.  
**Cardinality:** niente label per-player; top-N per opcode/map.

## Spazio disco

Aggregati; formato compatto (+ compressione opzionale); rotazione; budget 75 MB. Stima: ~20â€“40 MB per 14 giorni di retention mista â€” vedi tabella nella versione EN.

## Acceptance performance

Misura â†’ collo di bottiglia â†’ baseline â†’ ipotesi â†’ piccola modifica â†’ stesso bench â†’ confronto â†’ correttezza â†’ documentazione.

## Output local sink M3

Vedi [METRIC_CATALOG.md](METRIC_CATALOG.md). Finestre .mlines, latest_report.txt e un daily_YYYYMMDD.txt al giorno (avg/p95/max + riepilogo errori/warning) entro il budget ~75 MB.

## M8 (harden admin + dense scrape)

Campionamento denso opcode/DB/script **opzionale** (`Metric.DenseScrape`) con `SamplePercent` e `MaxSeries` sotto budget **~75 MB**. Retention + endpoint `/metrics/store` per evidenza bytes/CPU. Vedi catalogo metriche e guida admin.

## M9
Extended /health, metric store path exposure, crash dump budget, config schema/--check-config, optional operator drain.

