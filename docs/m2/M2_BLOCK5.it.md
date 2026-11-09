# IE-M2 Blocco5 - Estirpazione legacy (traduttore SMSG rimosso, flag block/stub nella tabella densa)

**Milestone:** IE-M2 Blocco5 (autorizzato da maintainer, t662u)
**Data:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). **Solo commit locale - nessun push.**
**Produzione:** `_public_iem1` gira Blocco4 (`543b5dedc560`). Blocco5 solo locale + isolata.
**EN:** [`M2_BLOCK5.md`](M2_BLOCK5.md)

## 1. Rimosso

- `ClassicOpcodes::TranslateServerOpcode` + `TranslateServerOpcode70245` (definizione + dichiarazione).
- `ClassicOpcodes::IsServerOpcodeBlocked`, `ClassicOpcodes::GetServerStubPayload` e le tabelle `ServerOpcodes` / `BlockedServerOpcodes` / `StubServerPayloads`.
- Audit di parita' al boot di Blocco4 (1410 chiamate) in `AdapterShadow.cpp`.
- `#include "ClassicOpcodes.h"` da `WorldSocket.cpp` e `AdapterShadow.cpp`. Un chiamante residuo non compila.

## 2. Policy block / stub = flag della tabella densa

- Le righe SMSG di `opcodes.json` avevano gia' `kFlagBlocked` (3) e `kFlagStub` (2).
- Nuova sezione `smsg_stub_payloads` (`logical`, `name`, `payload_hex`) + conteggio `smsg_stub`; i dati sorgente stanno ora nel generatore
  (`SERVER_OVERRIDES`, `SERVER_BLOCKED`, `SERVER_STUB_PAYLOADS`). Righe SMSG/CMSG identiche a Blocco4; manifest ri-firmato; `--check` PASS.
- Loader: verifica conteggi, un payload per ogni riga `kFlagStub` e viceversa, hex valido, max 64 byte; altrimenti fail closed.
- Tabella: `StubPayload[16]` a capacita' fissa, riempita al boot, zero heap.
- Invio: `kFlagBlocked` -> non inviato (nessun contatore unknown, nessun log); `kFlagStub` -> payload segnaposto dalla tabella.
- `WorldSocket::SendPacket`: una sola chiamata `TranslateOutbound(logical, OutboundPacket&)`. Nessun `ClassicOpcodes::` su recv o send.

## 3. Riga di boot

`Aeternum ProtocolAdapter: SMSG outbound ACTIVE SmsgAdapterTable entries=1410 slots=1649 blocked=3 stub=2 (send=adapter SmsgAdapterTable)`

## 4. ClassicOpcodes rimasti (fuori dal percorso di rete)

- `TranslateClientOpcode(70245)`: ancora definiti, zero chiamanti in `WorldSocket` (inbound via adapter da Blocco3). Fuori scope.
- `IsBuild70291OrLater` / `IsBuild70425OrLater`: controlli di build client per layout pacchetti/update field. Non traduzione.

## 5. Nota deploy

I binari Blocco5 richiedono il profilo Blocco5 (`smsg_stub_payloads` + nuovo hash nel manifest); col profilo Blocco4 vanno in fail closed.
I binari Blocco4 ignorano la nuova sezione (profilo retrocompatibile).

## 6. QA

Catch2 `[m2][blocco5]` S-W. Esito isolata nel Master Report.

## 7. Rollback

Revert del commit locale Blocco5. Nessun toggle runtime.
