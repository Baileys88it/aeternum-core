# Networking & session flow

**Status:** Implemented (ExtraInserts Partially — config-driven, fragile)

## Happy path

1. Launcher / client authenticates via **bnetserver** (port **1119**) + LoginREST.  
2. Realm list from auth (`realmlist` id 70, gamebuild **70790**).  
3. Client opens **worldserver** socket (8085).  
4. `WorldSocket` handles connection, encryption, packet framing (`WorldSocket.cpp`).  
5. Packets reach `WorldSession`; opcode decoded through **ClassicOpcodes** remap.  
6. `Opcodes.cpp` handler table dispatches (`STATUS_*`, `PROCESS_*`, member fn).

## ClientExtraInserts mechanism

See also `docs/audit/FOREVER_COMPATIBILITY.md`.

- Config keys: `Classic.ClientExtraInserts`, `Classic.ServerExtraInserts`.  
- Implementation: `ClassicOpcodes.cpp` (`RemoveClientExtraInserts` / `AddServerExtraInserts`).  
- Purpose: account for opcodes **inserted** mid-group in newer Forever builds without rewriting the whole table.  
- **70790 production values:** client `0x440071,0x440072`; server `0x470125`.  
- **Incident:** wrong inserts remapped character create → pet battle (2026-11-04).

## Sources

`WorldSocket.*`, `WorldSession.*`, `Opcodes.*`, `ClassicOpcodes.*`, `Server/Packets/`, worldserver.conf.
