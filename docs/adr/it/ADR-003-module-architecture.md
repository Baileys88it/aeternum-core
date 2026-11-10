# ADR-003: Static modules first via ScriptMgr

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-003-module-architecture.md`](../ADR-003-module-architecture.md)


## Problema
Servono estensioni senza unload dinamico unsafe né un secondo sistema script.

## Soluzione
Moduli statici via ScriptMgr/SDK; native/WASM rimandati.

## Perché
Sicurezza e no duplicazione.

## Alternative
DLL immediate (no); WASM-first (no).

## Fonti
ScriptMgr; CppCoreGuidelines; Wasmtime (valutazione futura).

## Complessità / Perf / Manutenzione
Bassa-media / neutra / migliore ownership.

## Test
Lifecycle; fallimento modulo opzionale non kill.

## Rischi & rollback
Feature-flag; path ScriptMgr diretto resta.

## Decisione (Accettato M2 — 2026-11-05)

Moduli statici via ScriptMgr: invariato. Niente codice moduli in M2.
