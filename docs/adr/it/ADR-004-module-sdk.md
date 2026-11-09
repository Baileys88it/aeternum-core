# ADR-004: Versioned Module SDK (semver)

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-004-module-sdk.md`](../ADR-004-module-sdk.md)


## Problema
Nessun contratto API versionato per i moduli.

## Soluzione
SDK Aeternum con semver; primo consumatore Admin & Monitoring.

## Perché
Priorità #1 insieme ai profili client.

## Alternative
Solo ScriptMgr (insufficiente); ABI plugin anno-1 (troppo presto).

## Fonti
https://semver.org/ ; ScriptMgr.

## Complessità / Perf / Manutenzione
Media / nulla / richiede disciplina.

## Test
Mismatch versione; modulo sample solo con header SDK.

## Rischi & rollback
Pin range; freeze SDK in emergenza.

## Decisione (Accettato M2 — 2026-11-05)

SDK versionato resta priorità #1 con client-update. Implementazione non in M2.
