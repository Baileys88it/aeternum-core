# Report discovery repository

> Versione italiana. EN: [`../en/AETERNUM_REPOSITORY_DISCOVERY_REPORT.md`](../en/AETERNUM_REPOSITORY_DISCOVERY_REPORT.md)

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Scope:** Forever-only; read-only in IE-M0; IE-M1 ha introdotto addizioni documentate/contratti.

## Layout rilevante
- `src/server/game` — simulazione + Forever seams  
- `src/modules/admin_monitoring`, `src/modules/sdk` — addizioni Aeternum  
- `forever/client_profiles` — profilo attivo 1.60.1.70790 + goldens IE-M1  
- `include/aeternum/api` — contratti pubblici 1.0  
- `docs/independent_engine` — pacchetto IE  
- `tests/` — Catch2 (metric, classic remap, api, goldens, adapter)

## Lineage
Derivato TrinityCore (GPL-2.0) con lavoro Forever/Mobius e delta Aeternum additivo. CMake `project(TrinityCore)` resta (identità ancora legacy).

## Evidenze opcode
1032 / 380 / 626 come da reconteggio. JSON coverage esiste ma numeri interni stale.

## Policy
GitHub Actions disabilitate. Secret/cert fuori dal repo.
