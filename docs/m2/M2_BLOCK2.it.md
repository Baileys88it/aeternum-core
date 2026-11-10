# IE-M2 Blocco2 — ProtocolAdapter Shadow Mode in WorldSocket

**Milestone:** IE-M2 Blocco2 (autorizzato da maintainer, t656u)  
**Data:** 2026-11-06 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). Blocco1 pushato (`origin/feat/m2-protocol` = `89bc307a00`); Blocco2 è un commit **locale**.  
**EN:** [`M2_BLOCK2.md`](M2_BLOCK2.md) · **Blocco1:** [`M2_BLOCK1.it.md`](M2_BLOCK1.it.md)

## 1. Cosa cambia

Shadow Mode = **sola osservazione**. Per ogni pacchetto in ingresso `WorldSocket::ReadDataHandler` continua a fare dispatch sul risultato legacy
`ClassicOpcodes::TranslateClientOpcode`. In parallelo il wire id grezzo viene risolto nella `AdapterOpcodeTable` densa (O(1), zero heap)
e i due risultati vengono confrontati. L'adapter non cambia mai il dispatch e non scarta mai pacchetti.

| Artefatto | Path |
|---|---|
| Confronto shadow (puro, header-only, hot path, lint no-map/no-heap) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/shadow.hpp` |
| Glue worldserver (load al boot + sink logger nativo) | `src/server/game/Server/Protocol/AdapterShadow.h/.cpp` |
| Hook per pacchetto | `src/server/game/Server/WorldSocket.cpp` — `WorldSocket::ReadDataHandler()` subito dopo `TranslateClientOpcode` |
| Load al boot (una volta; reload conf = no-op) | `src/server/game/World/World.cpp` — dopo `Forever::LogClientProfileSeams` |
| Include dir game (private) | `src/server/game/CMakeLists.txt` |
| Lint: `shadow.hpp` aggiunto agli header hot path | `contrib/aeternum/ci/check_adapter_no_map.ps1` |
| Catch2 `[m2][blocco2]` G–J | `tests/protocol_adapter/ProtocolAdapterBlocco2.cpp` |

## 2. Verdetti

| Verdetto | Condizione | Effetto |
|---|---|---|
| `Inactive` | tabella non caricata | nessun lookup, nessun contatore, **nessun log** |
| `Match` | logico adapter == logico legacy | contatore |
| `IgnoredKnown` | `ignored_known` del profilo (`0x3D013D`, wire `0x3E013D`) | solo contatore, **mai log, mai mismatch** (regola d'oro) |
| `BothUnknown` | nessuna riga adapter e nessun handler core | contatore unknown + ERROR **rate-limited** (`unknown_policy.json`) |
| `Mismatch` | logico adapter != legacy, oppure adapter unknown con handler core presente | contatore + riga ERROR (limitata per finestra) |

Il dispatch usa sempre il legacy (`ShadowDispatchOpcode`).

## 3. Logging nativo

- **Mismatch:** `TC_LOG_ERROR("network.opcode", "Aeternum ProtocolAdapter shadow mismatch raw=… legacy=… adapter=…|UNKNOWN (dispatch=legacy)")` (`TC_LOG_FATAL` in build Debug, solo log). Max 10 righe per finestra di 10000 ms; ogni mismatch è comunque contato.
- **Rate limiter unknown → logger nativo:** sink `NativeUnknownSink` → `TC_LOG_ERROR("network.opcode", "Aeternum ProtocolAdapter unknown opcode rate limit: …")`. Policy: finestra 10000 ms, soglia 50, max 1 ERROR per finestra.
- **ignored_known:** zero righe. Le righe legacy "sent wrong opcode" / "No defined handler" sono saltate **solo** per pacchetti classificati `ignored_known`; l'esito del dispatch legacy non cambia.
- **Boot:** `TC_LOG_WARN("server.loading", "Aeternum ProtocolAdapter shadow: ACTIVE profile …")` oppure riga esplicita inactive / load failed.

## 4. Posizione profilo a runtime

`Classic.ProfileDataRoot` (poi cwd) + `forever/client_profiles/<Classic.ActiveClientProfile>/`. Senza i file Blocco1 il loader fallisce chiuso e lo shadow resta `Inactive` (comportamento legacy).

## 5. QA (Catch2, build_m2 RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`)

| Tag | Casi | Asserzioni | Esito |
|---|---|---|---|
| `[m2][blocco2]` | 4 (G, H, I, J) | 230058 | PASS |
| `[m2][blocco1]` | 6 (A–F) | 50200 | PASS |
| `[m2]` | 10 | 280258 | PASS |
| `[m1]` | 39 | 258 | PASS |
| suite completa | 179 | 282474 | PASS |

Lint `aeternum-adapter-nomap-lint`: PASS. Esiti isolata e client reale: nel Master Report.

## 6. Rollback

Revert del commit Blocco2. Kill switch senza rebuild: `Classic.ProfileDataRoot` verso un tree senza i file Blocco1 (shadow `Inactive`).
