# Design Protocol Adapter — isolare gli opcode Forever dal core

**Milestone:** IE-M2 Discovery  
**Data:** 2026-11-05 (Europe/Rome)  
**EN:** [`PROTOCOL_ADAPTER_DESIGN.md`](PROTOCOL_ADAPTER_DESIGN.md)  
**ADR:** [`../adr/it/IE-ADR-013-protocol-profile-and-adapter.md`](../adr/it/IE-ADR-013-protocol-profile-and-adapter.md)

Stabilità prima di tutto. Estrazione strangler. Un solo client build alla volta. Zero heap sul path per-packet. Niente event bus su movement/aure fino a M3.

## Obiettivo

Numeri wire Forever, ExtraInserts, group-shift e flag di handshake diventano **dati del Protocol Profile**, consumati da un **Protocol Adapter**, così un update client non impone di rivedere tutto il core.

## Traduzione

Ordine recv (invariato rispetto a oggi): RemoveClientExtraInserts → ExtraGroupShift → Translate70245 → validate/unknown policy.

API già frozen: `WireOpcode` / `CoreOpcode` / `IOpcodeRegistry` / `IClientProfile`. Skeleton nuovo: `adapter.hpp` (`LogicalOpcode`, policy unknown).

Hot path: niente alloc; tabelle dense preallocate al boot; la `unordered_map` dello `.inc` va sostituita in un blocco dedicato dietro flag.

## Plug-in

- Recv: `WorldSocket.cpp:412-413`
- Send: `WorldSocket.cpp:539/567/584`
- Seat ExtraInserts: `ProtocolSession` + `ClassicOpcodes.cpp:128-133`
- Auth flags: SkipBuildAuthKey / RegionGroup da Profile

## Policy unknown — regola d'oro (implementata in M2-Blocco1)

| Classe | Decisione | Effetti |
|---|---|---|
| `Handled` | Dispatch | contatore hit |
| `KnownUnhandled` | Dispatch (decide il core come oggi) | contatore hit |
| `IgnoredKnown` (Profile `opcodes/unknown_policy.json` → `ignored_known[]`, coperto da integrity) | **Drop** | solo contatore — **ZERO log, ZERO I/O, ZERO heap** |
| `Unknown` (buco, gruppo assente, key > 0xFFFFFF) | **Drop** | contatore + **ERROR rate-limited** |

**Regola d'oro:** un opcode *noto* ignorato non deve mai produrre rumore nei log; un opcode davvero *sconosciuto* non deve restare muto per sempre, ma nemmeno inondare il log.

Rate limiter (`unknown`): `window_ms` 10000, `threshold_per_window` 50, `max_errors_per_window` 1. Sotto/uguale soglia: nessun log; sopra: al massimo N ERROR per finestra via sink (puntatore a funzione, niente heap), il resto conteggiato come soppresso. Nessun disconnect.

`0x3D013D` (dec 3998013) = id CMSG **logico (post-traduzione)** in un buco senza nome (wire `0x3E013D`) → `IgnoredKnown`. Sostituisce il default Discovery `log_once_metric_drop`.

Tabella densa implementata: 4 × `DenseOpcodeMap` (`GroupSpan[256]` + `OpcodeEntry[4096]` + contatori), lookup O(1) `noexcept`, ≈ 277 KB (283 800 B misurati). `std::map` / `unordered_map` / `set` vietati nell'adapter e heap vietato in `opcode_table.hpp` (lint `aeternum-adapter-nomap-lint`). Dettagli: [M2_BLOCK1.it.md](M2_BLOCK1.it.md).

## Blocchi M2

1. JSON Profile + SHA-256 (nessun cambio wire)  
2. Façade Translate* via Adapter (parity bit)  
3. Unknown + metriche  
4. Dense table al posto della map `.inc`  
5. Flag auth da Profile  
6+. Layout flags + primi codec + replay goldens  

## Test / rollback

Catch2 gate CREATE/CHECK + goldens Remap; QA isolata login; flag conf / cartella profile precedente; nessun deploy senza ok di maintainer.

## M2-Blocco1 — consegnato (locale)

Loader Profile solo-boot + `opcodes.json` generato e coperto da integrity + `unknown_policy.json` + `AdapterOpcodeTable` densa + regola d'oro + unknown rate-limited + lint no-map + Catch2 `[m2][blocco1]` A–F; **zero** cambi a WorldSocket / core. Vedi [M2_BLOCK1.it.md](M2_BLOCK1.it.md).
