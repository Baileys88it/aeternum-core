# ADR-007: Measure with existing Metric first

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-007-observability.md`](../ADR-007-observability.md)


## Problema
Poche misure; stack pesanti rischiano overhead.

## Soluzione
Prima Metric in-tree; Tracy/OTel solo investigazione con budget.

## Perché
modern ≠ better.

## Alternative
OTel-first (rimandato); Tracy sempre on (no).

## Fonti
Metric in-tree; OTel; Tracy; Prometheus docs.

## Complessità / Perf / Manutenzione
Bassa-media; overhead da misurare.

## Test
Before/after CPU/RAM.

## Rischi & rollback
`Metric.Enable` off.

## Decisione (Accettato M2 — 2026-11-05)

1. Partire da `Metric` TrinityCore → sink **locale** file/log (niente servizi esterni senza giustificazione).  
2. Capacità: il core deve poter registrare l’intero set metriche server (tick, map, sessioni/latenza, opcode top-N, pkt, DB pool, conteggi, script/AI, spell/aura, memoria, thread, crash) con catalogo nome/unità/significato/soglia/azioni — vedi doc osservabilità.  
3. Output leggibile: report umano periodico + in seguito Prometheus text via admin localhost.  
4. Poco disco: solo aggregati; niente log per-evento di default; formato compatto; rotazione; budget **~75 MB**; limiti di cardinality; tracing dettagliato solo on-demand a tempo limitato.  
5. **Codice non in M2.** Enable+sink+summary in **M3**; catalogo denso + `/metrics` admin in **M5–M8**.
