# Architettura moduli (M1)

**Stato:** Proposta  
**Pilota:** Administration & Monitoring  
**EN:** [`../en/MODULE_ARCHITECTURE.md`](../en/MODULE_ARCHITECTURE.md)

## Problema

Oggi l’estensione = C++ in `src/server/scripts` + hook `ScriptMgr`, senza SDK versionato né dominio di fallimento isolato.

## Soluzione a fasi

**A — Statici (M5):** ancora linkati nel worldserver; manifest + façade SDK su ScriptMgr; script esistenti restano validi.  
**B — Native dynamic (rimandato):** solo con ABI stabile, lifecycle documentato, niente hot-unload iniziale.  
**C — WASM (solo investigazione):** Wasmtime valutato in M0; adottare solo con beneficio di isolamento dimostrato.

## Manifest (Proposta)

Campi: `id`, `version`, `sdk`, `engine_api`, `runtime` (`static|native|wasm`), `critical`, dipendenze, `permissions`, `init_order`, migrazioni DB, schema conf.

## Lifecycle

`DISCOVER → VALIDATE → RESOLVE_DEPS → INITIALIZE → REGISTER_SERVICES → ACTIVATE → RUNNING → DEACTIVATE → SHUTDOWN`

Fallimento di modulo `critical:false` → server continua; log + metrica.

## SDK

Semver indipendente; deprecazioni su minor; breaking su major; vietare include di header privati engine.
