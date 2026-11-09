# IE-M2 Protocol Compatibility

**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`)  
**Base:** tag `M1-Complete` (`a52e748262`)  
**Status:** Discovery + Blocco1-3 pushed (`origin/feat/m2-protocol` = `03b8c83c6aee`); public runs Blocco3 hard switch. Blocco4 (SMSG outbound) **local only**.

| Doc | Purpose |
|---|---|
| [PROTOCOL_PROFILE_70790.md](PROTOCOL_PROFILE_70790.md) | Inventory + Profile package map for 1.60.1.70790 |
| [PROTOCOL_ADAPTER_DESIGN.md](PROTOCOL_ADAPTER_DESIGN.md) | Adapter design, blocks, tests, rollback, golden rule |
| [M2_BLOCK1.md](M2_BLOCK1.md) | Blocco1: Profile loader + dense AdapterOpcodeTable + ignored_known / unknown policy |
| [M2_BLOCK2.md](M2_BLOCK2.md) | Blocco2: Shadow Mode in WorldSocket (compare only, legacy dispatch) + rate limiter on native logger |
| [M2_BLOCK3.md](M2_BLOCK3.md) | Blocco3: Dispatch switch to ProtocolAdapter; ClassicOpcodes removed from inbound path |
| [M2_BLOCK4.md](M2_BLOCK4.md) | Blocco4: SMSG outbound via dense SmsgAdapterTable; TranslateServerOpcode removed from send path (local/isolata) |
| [M2_BLOCK5.md](M2_BLOCK5.md) | Blocco5: legacy excision - TranslateServerOpcode / IsServerOpcodeBlocked / GetServerStubPayload removed; block/stub via dense-table flags (local/isolata) |
| [../adr/IE-ADR-013-protocol-profile-and-adapter.md](../adr/IE-ADR-013-protocol-profile-and-adapter.md) | ADR (Proposed) |

Italian siblings: `*.it.md`.