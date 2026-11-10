# IE-M1 Step 3 — Bridge di sessione Protocol + istanza di test isolata

**Data:** 2026-11-05 (Europe/Rome)  
**Target commit:** `aeternum/main`  
**Soak:** intatto (bnet 24052 / world 20024 / monitor 11960; porte 2119 / 18083 / 18085 / admin 9100)

## Cosa è stato consegnato

1. **Bridge di sessione** (`Forever/ProtocolSessionBridge`): seat di processo per `IOpcodeRegistry` / `IClientProfile`.  
   `ClassicOpcodes::RemoveClientExtraInserts` passa dal bridge se `Classic.ProtocolApiAdapter.Enable=1` (default **ON** — parità Remap/adapter provata da Catch2).  
   La matematica resta `Forever::OpcodeRemap::RemoveClientExtraInserts` con le stesse liste Effective/profile → **zero cambio wire**.

2. **Istanza di test isolata** (non soak):  
   - Dir: `<ENGINE_ROOT>\_testinst_iem1` (conf + log; fuori dal `build\bin\RelWithDebInfo` dello soak)  
   - Binari: `build_iem1\bin\RelWithDebInfo` via `-c`  
   - Porte (solo 127.0.0.1): bnet **3119**, REST **28083**, world **28085**, admin **9200**  
   - DB (MySQL 3308, **conservati**): `aeternumtest_auth`, `aeternumtest_characters`, `aeternumtest_world`, `aeternumtest_hotfixes`  
   - Credenziali: root documentato (DPAPI) solo per CREATE/GRANT; runtime con utente trinity sulle schema di test. Segreti mai in commit.

3. **Admin smoke:** chiavi `/health` identiche soak↔test (`status,db,modules,scripts`). Su test `/uptime`, `/sessions`, `/metrics` OK con token solo-test. Su soak `/metrics` richiede auth (token solo in env del processo soak — non disponibile all’agente); i **nomi** gauge sul test rispettano il contratto `BuildPrometheus`.

## Numeri di verifica

| Check | Risultato |
|-------|-----------|
| Catch2 `[protocol]` | 57 assertion / 12 case PASS |
| Catch2 full | **1926 assertion / 126 case PASS** |
| Avvio test | listen admin 9200; log: `ProtocolApiAdapter session bridge: enable=1 profile='1.60.1.70790' client_inserts=2` |
| ERROR/FATAL Server.log test | **0** |
| ERROR soak (campione) | **0** recenti |
| Impatto soak durante test (5s) | world ΔCPU **0.109** s, bnet ΔCPU **0.031** s; WS soak stabile ~428 MB |
| RAM | prima ~14.7 GB liberi → durante test ~10.1 GB → dopo stop ~12.9 GB |
| Login client reale | **non possibile** su porte loopback alternate — da schedulare a parte |

## Flag

`Classic.ProtocolApiAdapter.Enable` — default `1`. Con `0` si torna al path diretto `EffectiveClientExtraInserts` (stesso Remap).

## Non fatto

- Login client reale su porte test o soak  
- Sniff / 0x3D013D  
- Alberi / DB Trinity di produzione