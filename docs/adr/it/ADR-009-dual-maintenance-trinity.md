# ADR-009: Dual maintenance until Aeternum stable

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-009-dual-maintenance-trinity.md`](../ADR-009-dual-maintenance-trinity.md)


## Problema
Evolvere Aeternum senza destabilizzare i player su Trinity.

## Soluzione
Trinity resta produzione fino a criteri “Aeternum stable”; niente write/restart Trinity da Aeternum; porting controllato.

## Perché
Decisione maintainer; STABILITY.

## Alternative
Cutover immediato (no); fermare Trinity (no).

## Fonti
Operatività; cultura rollback Aeternum.

## Complessità / Perf / Manutenzione
Overhead di processo; perf nulla.

## Test
Checklist cutover; backup drill.

## Rischi & rollback
Drift; rollback = restare su Trinity.

## Decisione (Accettato M2 — 2026-11-05)

Doppia manutenzione confermata. Soak: **72h** run pulito continuo, poi **14 giorni** con player senza crash/regressioni. Cutover solo con OK esplicito di maintainer. Gli agent non scrivono/riavviano Trinity Engine.
