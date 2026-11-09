# Guida Module SDK (statico v0.2, M7)

**Stato:** Draft implementato (`AETERNUM_SDK_API_DRAFT`) — **non congelato**
**EN:** [`../en/MODULE_SDK_GUIDE.md`](../en/MODULE_SDK_GUIDE.md)

## Lifecycle

`DISCOVER → LOAD → INIT → START → STOP → SHUTDOWN`
Dipendenze prima in start; ordine inverso in stop. I cicli abortiscono l'avvio.

## `critical`

- `true` → abort pulito (`World::StopNow`)
- `false` → modulo disabilitato + log, il world continua

## Compatibilità profilo

Lista `supportedProfiles`; vuota/nullptr = tutti. Altrimenti deve coincidere con `Classic.ActiveClientProfile`.

## Esempio

Vedi guida EN e modulo `admin.monitoring`.

## Logging

Logger.root di default e solo ERROR. Per vedere il lifecycle in Server.log:

`
Logger.module.sdk=4,Console Server
Logger.module.admin=4,Console Server
`
