# Architettura attuale Aeternum

> Versione italiana. EN: [`../en/AETERNUM_CURRENT_ARCHITECTURE.md`](../en/AETERNUM_CURRENT_ARCHITECTURE.md)

**Data:** 2026-11-05 (Europe/Rome)  
**Classificazione:** evidenza di discovery (poi aggiornata da IE-M1)

## Sintesi
Aeternum oggi è uno **stack Forever multiprocesso** (`bnetserver` + `worldserver`) derivato da TrinityCore/Mobius, con un **layer additivo** Aeternum (profili client, ExtraInserts, Metric local sink, Admin monitoring, SDK draft, tool/CI locali).

«Motore indipendente» è l’obiettivo della traccia IE, non una descrizione dello stato corrente.

## Processi e porte (lab Aeternum)
Lab isolato tipico: bnet/LoginREST/world su porte dedicate (non produzione Trinity). Soak lungo in corso su quel lab — i binari RelWithDebInfo del soak non vanno sovrascritti durante IE-M1.

## Layering
`common` → `game` → `worldserver` / `bnetserver`. Script statici; moduli Aeternum (`admin_monitoring`, `sdk`) agganciati come consumatori statici.

## Forever protocol seams (oggi)
- `forever/client_profiles/<id>/` con `ACTIVE`, `extra_inserts.conf`, manifest  
- `Forever::OpcodeRemap` helper puri  
- `ClassicOpcodes` traduce wire↔core (tabelle + ExtraInserts + group shift)  
- IE-M1 aggiunge `aeternum/api/protocol` + `ProtocolApiAdapter` (thin, zero cambio wire)

## Observability
Metric local sink (default off, budget ~75 MB). Admin HTTP + Prometheus text (default off, loopback).

## Testing
Catch2 in-tree; CI GitHub disabilitata per policy; `contrib/aeternum/ci/local_ci.ps1` per lab.

## Cosa non è ancora vero
- Boundary Trinity fisico `adapters/trinitycore` popolato  
- Host SDK senza `World.h` / `ScriptMgr.h`  
- Codec tipizzati e corpus capture live  
- Indipendenza di licenza oltre GPL-2.0
