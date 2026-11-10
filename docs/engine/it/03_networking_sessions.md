# Networking e sessioni

**Stato:** Implementato (ExtraInserts: Parzialmente — via conf, fragile)

1. Auth su bnetserver **1119** + LoginREST.  
2. Realmlist (`gamebuild` **70790**).  
3. Socket world **8085** → `WorldSocket` → `WorldSession`.  
4. Remap **ClassicOpcodes** (ClientExtraInserts / ServerExtraInserts).  
5. Dispatch `Opcodes.cpp`.

Valori 70790: client `0x440071,0x440072`; server `0x470125`.  
Caso studio: insert sbagliati `0x63/0x64` rompevano la creazione personaggio (vedi audit Forever).

**Sorgenti:** `WorldSocket.*`, `WorldSession.*`, `ClassicOpcodes.*`, `Opcodes.*`, conf.
