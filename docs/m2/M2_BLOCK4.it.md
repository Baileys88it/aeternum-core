# IE-M2 Blocco4 - Strato di invio SMSG (ProtocolAdapter in uscita)

**Milestone:** IE-M2 Blocco4 (autorizzato da maintainer, t661u)
**Data:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). **Solo commit locale - nessun push.**
**Produzione:** `_public_iem1` resta su Blocco3 (inbound adapter, outbound legacy). Blocco solo locale + isolata.
**EN:** [`M2_BLOCK4.md`](M2_BLOCK4.md)

## 1. Cosa cambia

La traduzione degli opcode in uscita (id SMSG logico/core -> wire del client 1.60.1.70790) usa **solo** la tabella densa del ProtocolAdapter.
`ClassicOpcodes::TranslateServerOpcode` non e' piu' sul percorso critico di invio (`SendPacket`, `WritePacketToBuffer`, `CompressPacket`).

| Artefatto | Percorso |
|---|---|
| Accessor `SmsgAdapterTable()` + `ClassifyServerLogical` + finestra rate-limit outbound separata | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/opcode_table.hpp` |
| Traduzione outbound (header-only, hot path, lint no-map/no-heap) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/outbound.hpp` |
| Glue worldserver `TranslateOutbound` / `PeekOutboundWire` + audit di parita' al boot | `src/server/game/Server/Protocol/AdapterShadow.h/.cpp` |
| Hook di invio | `src/server/game/Server/WorldSocket.cpp` - `SendPacket`, `WritePacketToBuffer`, `CompressPacket` |
| Lint | `contrib/aeternum/ci/check_adapter_no_map.ps1` - `outbound.hpp` nella lista hot-path; `AdapterShadow.h/.cpp` scansionati |
| Catch2 `[m2][blocco4]` O-R | `tests/protocol_adapter/ProtocolAdapterBlocco4.cpp` |

## 2. Scelta della tabella: riuso, nessun duplicato

Blocco1 carica gia' al boot `AdapterOpcodeTable::smsgLogicalToWire`: `DenseOpcodeMap` a storage fisso, O(1), zero heap, con tutte le 1410 righe SMSG di
`opcodes.json`. Blocco4 **espone quell'array come `SmsgAdapterTable()`** e lo usa per l'invio, senza allocare una seconda copia (una sola fonte, un solo caricamento verificato SHA-256).

## 3. Policy opcode logici non mappati

Il legacy restituiva l'id core invariato fuori dai gruppi retail (wire fasullo verso il client). Adapter: **non si invia**. Contatore outbound +
`TC_LOG_ERROR` rate-limited (`smsg_logical=0x...`, policy 10000 ms / 50 / 1) su **finestra outbound separata** (inbound invariato).
Mai l'id logico grezzo, mai il traduttore legacy. Tabella non caricata -> fail closed: nessun invio.

## 4. Righe di boot

- `... ACTIVE profile ... (dispatch=adapter ProtocolAdapter)` (inbound, invariata)
- `... SMSG outbound ACTIVE SmsgAdapterTable entries=N slots=N legacy_parity=X/N (send=adapter SmsgAdapterTable)`
- Audit di parita' solo al boot contro `TranslateServerOpcode` con la conf reale; mismatch -> `TC_LOG_ERROR`. Non e' sul percorso di invio e non e' un fallback.

## 5. ClassicOpcodes residui lato invio

`IsServerOpcodeBlocked` e `GetServerStubPayload` (policy, non traduzione) invariati. `TranslateServerOpcode` chiamato solo dall'audit di boot.

## 6. Rollback

Revert del commit locale Blocco4.
