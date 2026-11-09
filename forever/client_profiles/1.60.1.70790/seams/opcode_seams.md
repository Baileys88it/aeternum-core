# Opcode seams — profile 1.60.1.70790

| Seam | Location | Role |
|---|---|---|
| ExtraInserts conf | `Classic.ClientExtraInserts` / `ServerExtraInserts` in worldserver.conf | Build-specific inserts |
| Profile canonical values | `extra_inserts.conf` (this profile) | Source of truth for the active profile |
| Remap algorithm | `ClassicOpcodes.cpp` + `Forever/ClassicOpcodeRemap.h` | Wire → core handler |
| Handler table | `Opcodes.h` / `Opcodes.cpp` | Core OpcodeClient / OpcodeServer |
| Packet layouts | `Server/Packets/*` | Structure deltas per build |
| Expectations table | `opcodes_table.md` | Critical CMSG map for gates |

**Must-not-break gate:** Catch2 `[classic][profile][active][gate]` loads `forever/client_profiles/ACTIVE` and fails CI/local tests if CREATE/CHECK remap drifts.

IT: `opcode_seams.it.md`
