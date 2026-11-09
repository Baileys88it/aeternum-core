# ADR-005: Preserve MapUpdater ownership; measure first

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-005-threading-strategy.md`](../ADR-005-threading-strategy.md)


## Problema
Modernizzare i thread senza misure rischia race.

## Soluzione
Preservare MapUpdater; misurare in M3; no thread-per-modulo per admin.

## Perché
Preserve / optimize what matters.

## Alternative
Rewrite actor (no ora).

## Fonti
MapUpdater, World::Update; CppCoreGuidelines.

## Complessità / Perf / Manutenzione
Bassa; perf TBD; stabile.

## Test
Shutdown; stress futuri.

## Rischi & rollback
Flag; default = attuale.

## Decisione (Accettato M2 — 2026-11-05)

MapUpdater invariato; prima misurare (ADR-007). Niente redesign threading in M2.
