# ADR-001: Engine / Forever / Framework / Modules boundaries

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-001-engine-boundaries.md`](../ADR-001-engine-boundaries.md)


## Problema
Il tree Forever mescola runtime, delta client e script senza confini chiari.

## Soluzione
Confini logici su albero esistente: engine / forever (un profilo) / framework / moduli statici. Niente rename di massa.

## Perché
Preserva ciò che funziona; localizza gli update client.

## Alternative
Big-bang refolder (no); status quo (no — incidente 70790).

## Fonti
TrinityCore tree; CppCoreGuidelines; conclusione nostra: façade incrementali.

## Complessità / Perf / Manutenzione
Media / nulla / migliora se rispettata in review.

## Test
Checklist review; lint Planned contro `#ifdef` build sparsi.

## Rischi & rollback
Ora solo docs; rollback = revert ADR.

## Decisione (Accettato M2 — 2026-11-05)

Confini confermati. Dati profilo in `forever/client_profiles/<id>/`. Adapter C++ in `src/server/game/Forever/` (M4). Niente check di versione Forever sparsi nel core.
