# Design contratti API Aeternum

> Versione italiana. EN: [`../en/AETERNUM_API_CONTRACT_DESIGN.md`](../en/AETERNUM_API_CONTRACT_DESIGN.md)

**Stato:** bozze architetturali; superficie 1.0 congelata in `include/aeternum/api` dove esiste consumatore reale  
**Data:** 2026-11-05 (Europe/Rome)  
**Vincolo:** gli header pubblici NON DEVONO esporre `Player*`, `WorldSession*`, `Map*`, `Unit*`, `Creature*`, `WorldObject*`.

## 0. Regole trasversali

| Concern | Regola |
|---|---|
| Ownership | Interfacce di proprietà delle librerie di dominio Aeternum; i moduli non possiedono singleton engine |
| Lifetime | L’host sopravvive ai moduli; validità EntityId termina a despawn/invalidate |
| Thread | Affinità documentata per metodo; chiamate cross-thread vietate o marshalate |
| Eccezioni | Niente eccezioni attraverso il boundary SDK — solo `Status` / `Result<T>` |
| ABI | IE-M1..M3: static link, niente promessa C ABI; ABI dinamica solo se IE-ADR-012 lo consente |
| Versioning | `aeternum/api` 1.0.0 congelata; draft `modules/sdk` separato |
| Secret | Mai loggare token/password; gli snapshot config non portano DatabaseInfo |
| Trinity | Consentito solo dentro adapter / TU legacy |

## 1. Lifecycle engine — `aeternum::kernel` (bozza, non ancora in freeze 1.0)
Scopo: boot/drain/shutdown senza includere `World.h`. Consumatore reale arriverà con Kernel extraction.

## 2. Service registry (bozza)
Scopo: localizzare façade di dominio senza god-object `sWorld`. Vietato registrare puntatori Trinity grezzi.

## 3. Profilo client e sessioni (protocol) — **in freeze 1.0**
Implementato come header `aeternum/api/protocol/*` + adapter Forever `ProtocolApiAdapter` su ExtraInserts/Remap.

- `IClientProfile` — profile id, build, ExtraInserts, reload  
- `IOpcodeRegistry` — WireToCore / CoreToWire / HandlerOf  
- `IPacketCodec` — ancora opaco (payload bytes) fino ai codec tipizzati  

**Thread:** Decode/Encode puri/rientranti; Disconnect marshalato.  
**Test:** Catch2 remap + corpus golden + test adapter.

## 4. Entity handles & world query (bozza)
`EntityId` / `MapId` / snapshot POD; niente puntatori. Da estrarre dopo Protocol/Operations/Module/Kernel.

## 5. Servizi gameplay (bozza)
Superficie intenzionalmente piccola; espandere solo con test. Moduli gameplay **dopo** estrazione World (decisione #8).

## 6. Repository di persistenza (bozza)
Porte graduali; MySQL resta system of record.

## 7. Operations — **in freeze 1.0**
`aeternum::ops::IAdminService`, `IMetricSink`, `IPrometheusText` — consumatori: `admin_monitoring`, Metric local sink.

## 8. Config snapshot — **in freeze 1.0**
`EngineConfigSnapshot` senza connection string; consumatore: Config.Schema / `--check-config`.

## 9. Module lifecycle — **in freeze 1.0**
`aeternum::api_sdk::IModule` e descriptor; draft host in `src/modules/sdk` resta accoppiato a Trinity fino a clearance debito.
