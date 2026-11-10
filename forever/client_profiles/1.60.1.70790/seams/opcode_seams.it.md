# Seam opcode — profilo 1.60.1.70790

| Seam | Dove | Ruolo |
|---|---|---|
| ExtraInserts conf | `Classic.ClientExtraInserts` / `ServerExtraInserts` in worldserver.conf | Insert specifici della build |
| Valori canonici profilo | `extra_inserts.conf` (questo profilo) | Fonte di verità del profilo attivo |
| Algoritmo remap | `ClassicOpcodes.cpp` + `Forever/ClassicOpcodeRemap.h` | Wire → handler core |
| Tabella handler | `Opcodes.h` / `Opcodes.cpp` | OpcodeClient / OpcodeServer |
| Layout pacchetti | `Server/Packets/*` | Delta strutturali per build |
| Tabella aspettative | `opcodes_table.md` | Mappa CMSG critici per i gate |

**Gate must-not-break:** Catch2 `[classic][profile][active][gate]` legge `forever/client_profiles/ACTIVE` e fallisce se CREATE/CHECK driftano.

EN: `opcode_seams.md`
