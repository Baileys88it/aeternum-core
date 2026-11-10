# IE-M2 Blocco5 - Legacy Excision (SMSG translator removed, block/stub flags in the dense table)

**Milestone:** IE-M2 Blocco5 (authorized by maintainer, t662u)
**Date:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`). **Local commit only - no push.**
**Production:** `_public_iem1` runs Blocco4 (`543b5dedc560`). Blocco5 is local + isolata only.
**IT:** [`M2_BLOCK5.it.md`](M2_BLOCK5.it.md) - **Blocco4:** [`M2_BLOCK4.md`](M2_BLOCK4.md)

## 1. What was removed

| Removed | Where |
|---|---|
| `ClassicOpcodes::TranslateServerOpcode` + `TranslateServerOpcode70245` (definition + declaration) | `ClassicOpcodes.cpp/.h` |
| `ClassicOpcodes::IsServerOpcodeBlocked`, `ClassicOpcodes::GetServerStubPayload` | `ClassicOpcodes.cpp/.h` |
| `ServerOpcodes` / `BlockedServerOpcodes` / `StubServerPayloads` tables, `AddServerExtraInserts`, server group constants | `ClassicOpcodes.cpp` |
| Blocco4 boot parity audit (1410 calls to `TranslateServerOpcode`) | `AdapterShadow.cpp` |
| `#include "ClassicOpcodes.h"` | `WorldSocket.cpp`, `AdapterShadow.cpp` |

Any leftover caller fails to compile (the declarations no longer exist).

## 2. Block / stub policy = dense-table flags

- `opcodes.json` SMSG rows already carried `kFlagBlocked` (0x0004, 3 rows) and `kFlagStub` (0x0008, 2 rows).
- New section `smsg_stub_payloads` (`logical`, `name`, `payload_hex`) + count `smsg_stub`; generator `gen_opcode_table.py` owns the source data
  (`SERVER_OVERRIDES`, `SERVER_BLOCKED`, `SERVER_STUB_PAYLOADS`, moved verbatim from `ClassicOpcodes.cpp`). SMSG/CMSG rows are byte-identical to Blocco4;
  `manifest.json` integrity re-signed; `--check` PASS.
- Loader (`profile_loader.hpp`): checks `smsg_blocked` / `smsg_stub` counts, every `kFlagStub` row has exactly one payload and vice versa,
  hex valid, size <= 64, blocked+stub on the same row rejected. Fail closed (table not loaded) otherwise.
- Table (`opcode_table.hpp`): fixed-capacity `StubPayload[16]` (64 bytes each), filled at boot, read-only; no heap.
- Send (`outbound.hpp`): `kFlagBlocked` -> `OutboundAction::Blocked` (not sent, no unknown counter, no log); `kFlagStub` -> `OutboundResult::stub`.
  `PeekOutboundWire` refuses blocked entries.
- `WorldSocket::SendPacket`: one call `AdapterShadow::TranslateOutbound(logical, OutboundPacket&)` -> blocked / unmapped = not queued;
  `stub` = placeholder payload enqueued; otherwise the packet. No `ClassicOpcodes::` on recv or send.

| SMSG | logical | wire | policy |
|---|---|---|---|
| SMSG_HEALTH_UPDATE | 0x45017D | 0x47017F | blocked |
| SMSG_CHANNEL_LIST | 0x4A001B | 0x4C001D | blocked |
| SMSG_GUILD_CHALLENGE_COMPLETED | 0x51001B | 0x53001B | blocked |
| SMSG_ACCOUNT_ITEM_COLLECTION_DATA | 0x450360 | 0x470362 | stub `00000000` |
| SMSG_DAILY_QUESTS_RESET | 0x650000 | 0x670000 | stub (empty) |

## 3. Boot line

`Aeternum ProtocolAdapter: SMSG outbound ACTIVE SmsgAdapterTable entries=1410 slots=1649 blocked=3 stub=2 (send=adapter SmsgAdapterTable)`
(`legacy_parity` is gone with the audit). Inbound line unchanged (`dispatch=adapter ProtocolAdapter`).

## 4. Remaining ClassicOpcodes (not on the network path)

- `TranslateClientOpcode` / `TranslateClientOpcode70245`: still defined, zero callers in `WorldSocket` (inbound uses the adapter since Blocco3). Out of scope.
- `IsBuild70291OrLater` / `IsBuild70425OrLater`: client build checks for packet/update-field layouts (DB2Stores, UpdateFields, Player, Spell, Guild/Party packets). Not translation.

## 5. Deploy note

Blocco5 binaries need the Blocco5 profile (`opcodes.json` with `smsg_stub_payloads` + new manifest hash); with the Blocco4 profile they fail closed
(`opcodes_count_mismatch:smsg_stub`). Blocco4 binaries ignore the new section, so the new profile is backward compatible.

## 6. QA

Catch2 `[m2][blocco5]` S-W (`tests/protocol_adapter/ProtocolAdapterBlocco5.cpp`): blocked never sent, stub payload from the table,
known SMSG still translates, unmapped dropped, loader fail-closed cases, no `ClassicOpcodes` in `WorldSocket.cpp` / `AdapterShadow.cpp`, lint still watching `opcode_table.hpp`.
Isolata results in the Master Report.

## 7. Rollback

Revert the Blocco5 local commit (restores the C++ server translator/helpers and the Blocco4 profile). No runtime toggle.
