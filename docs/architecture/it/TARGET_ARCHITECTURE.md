# Architettura obiettivo (M1) — Mappatura incrementale sull’albero reale

**Stato:** Accettato (M2) (solo design — nessun move di directory in M1)  
**Albero base:** TrinityCore Forever `113226264c` / `aeternum/main`  
**EN:** [`../en/TARGET_ARCHITECTURE.md`](../en/TARGET_ARCHITECTURE.md)

## Decisioni approvate da maintainer

1. **Un solo build client Forever attivo** (singolo profilo). Cambiare build = sostituire/attivare il profilo successivo, non multi-profilo a runtime.  
2. **Trinity Engine** (`<SERVER_ROOT>`) resta produzione in parallelo finché Aeternum non è stabile — dual maintenance; nessun touch/restart da questo filone.  
3. **Priorità #1:** architettura update client + Module SDK; modulo pilota = **Administration & Monitoring**.  
4. **Modern ≠ better:** nessuna tech senza beneficio dimostrato.  
5. Mappare i concetti `engine/forever/framework/modules` sull’albero **reale** — niente big-bang.

## Mappatura concettuale → reale

| Zona | Ancoraggi OGGI | Cosa può muoversi dopo | Cosa NON si muove senza ADR |
|---|---|---|---|
| engine/runtime | `worldserver/Main.cpp`, `bnetserver/Main.cpp`, `World/World.cpp` | Wrapper sottili | Contratti exe/porte finché launcher non cambia insieme |
| engine/networking | `WorldSocket*`, `WorldSession*`, `Opcodes*`, `ClassicOpcodes*`, `Packets/` | Isolare adapter in profilo client (M4) | Porte **1119** / **8085** |
| engine/threading | `MapUpdater.*`, `MapManager.*` | Solo hook di misura | Ownership del tick di mappa |
| engine/database | `database/**`, `DatabaseWorkerPool`, `DBUpdater` | Tooling migrazioni | MySQL SoR; DB auth/characters/world/hotfixes |
| forever/protocol | ClassicOpcodes + ExtraInserts conf | **Primo target di isolamento** | Un solo profilo attivo |
| forever/client_profiles | *Non esiste* (Pianificato) | `forever/client_profiles/<build>/` | — |
| framework/hooks | `ScriptMgr.*` | Façade SDK (M5) | Registrazione script esistente |
| modules/admin | Comandi GM + Metric spenta | Pilota Admin & Monitoring | Niente exec remoto arbitrario |

## Fasi di move (intenzione)

- M1: solo docs/ADR  
- M2: qualità/test — niente reshuffle  
- M3: misure  
- M4: pacchetto profilo client  
- M5: SDK + pilota statico admin  

Diagramma mermaid completo: versione EN.
