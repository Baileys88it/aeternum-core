# ADR-008: Admin & Monitoring as first SDK module

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-008-admin-monitoring-pilot.md`](../ADR-008-admin-monitoring-pilot.md)


## Problema
Serve un primo modulo concreto per validare l’SDK.

## Soluzione
Admin & Monitoring: localhost, RBAC, metriche read-only, niente comandi arbitrari.

## Perché
Scelta maintainer; rischio contenuto.

## Alternative
Bots/eventi prima (più rischio); nessun pilota (ritarda apprendimento).

## Fonti
Pratiche security; Metric; `.online`; contratto launcher.

## Complessità / Perf / Manutenzione
Media; scrape moderato; ops.

## Test
Auth deny; localhost; audit; launcher OK.

## Rischi & rollback
`critical:false`; disable conf.

## Decisione (Accettato M2 — 2026-11-05)

Stack HTTP admin: **Boost.Beast** (Boost già dipendenza — nessuna lib nuova). Solo localhost; token; RBAC; audit. **Implementazione NON in M2** (M5+).
