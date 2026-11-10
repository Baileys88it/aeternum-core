# IE-M2 Blocco3 — Switch del Dispatch ProtocolAdapter (Distacco Legacy)

**Milestone:** IE-M2 Blocco3 (autorizzato da maintainer, t659u)  
**Data:** 2026-11-06 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). **Solo commit locale — nessun push.**  
**Produzione:** `_public_iem1` resta su **Blocco2 Dark Launch** (Shadow Mode, dispatch legacy). Questo blocco è solo locale + QA isolata.  
**EN:** [`M2_BLOCK3.md`](M2_BLOCK3.md) · **Blocco2:** [`M2_BLOCK2.it.md`](M2_BLOCK2.it.md)

## 1. Cosa cambia

Il dispatch primario inbound in `WorldSocket::ReadDataHandler` usa **solo** il risultato O(1) della tabella densa ProtocolAdapter. Il confronto Shadow Mode è rimosso. `ClassicOpcodes::TranslateClientOpcode` non è più nel percorso critico inbound (anche il gate HOTFIX oversized ora fa peek sulla tabella adapter senza contatori).

| Artefatto | Path |
|---|---|
| Translate inbound (puro, header-only, hot path) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/shadow.hpp` |
| Glue worldserver | `src/server/game/Server/Protocol/AdapterShadow.h/.cpp` |
| Dispatch per pacchetto | `src/server/game/Server/WorldSocket.cpp` |
| Boot | `src/server/game/World/World.cpp` dopo `Forever::LogClientProfileSeams` |
| Catch2 `[m2][blocco3]` K–N | `tests/protocol_adapter/ProtocolAdapterBlocco3.cpp` |

SMSG / `TranslateServerOpcode` invariati (fuori scope).

## 2. Decisioni inbound

| Azione | Condizione | Effetto |
|---|---|---|
| `Drop` | tabella non caricata | fail closed, nessun fallback ClassicOpcodes |
| `Drop` | `IgnoredKnown` | solo contatore, zero log, interrompe la catena |
| `Drop` | `Unknown` / OOR | contatore + ERROR rate-limited; interrompe la catena |
| `Dispatch` | `Handled` / `KnownUnhandled` | logical adapter → tabella handler core |

## 3. Nota produzione

Non deployare Blocco3 su `_public_iem1` in questa fase. Public resta Blocco2 Dark Launch. QA isolata su porte isolate (tipicamente 3119/28083/28085).