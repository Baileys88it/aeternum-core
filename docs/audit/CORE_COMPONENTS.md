# Core Components (M0)

**Status:** Implemented (as TrinityCore Forever tree)

| Component | Paths | Role |
|---|---|---|
| bnetserver | `src/server/bnetserver/Main.cpp`, `REST/`, `Services/` | Battlenet + LoginREST auth entry |
| worldserver | `src/server/worldserver/Main.cpp` | Game server entry |
| World | `src/server/game/World/World.cpp` | World::Update — sessions, maps, callbacks |
| WorldSocket / Mgr | `Server/WorldSocket*.cpp` | TCP game sockets |
| WorldSession | `Server/WorldSession.*` | Per-account session, opcode dispatch |
| Opcodes | `Server/Protocol/Opcodes.*` | Handler table |
| ClassicOpcodes | `Server/Protocol/ClassicOpcodes.*` | Wire remap + ExtraInserts |
| Packets | `Server/Packets/` | Message layouts |
| Map / MapUpdater | `Maps/Map*.cpp`, `MapUpdater.*` | Map ticks; worker queue/threads |
| Entities | `Entities/` | Player, Creature, GameObject, … |
| Movement | `Movement/` | Motion masters, splines |
| Combat / Spells | `Combat/`, `Spells/` | Threat, spell system |
| Quests | `Quests/` | Quest state machine |
| AI | `AI/` incl. SmartAI | CreatureAI, SmartScript |
| ScriptMgr | `Scripting/ScriptMgr.*` | Hook registration |
| Scripts | `src/server/scripts/` | C++ content scripts |
| DatabaseWorkerPool | `database/Database/DatabaseWorkerPool.*` | Async DB |
| DBUpdater | `database/Updater/DBUpdater.*` | SQL update apply |
| Config / Logging / Metric | `src/common/Configuration`, `Logging`, `Metric` | Conf, logs, optional Influx metrics |
| DataStores | `game/DataStores`, `common/DataStores` | DB2/DBC loaders |
| Aeternum | `(operator files, not included)` | fixes, installers, launcher helpers, docs |

Entry flow (high level): bnetserver authenticates → client connects worldserver → WorldSocket → WorldSession → opcode handler → game systems → Map update via MapUpdater.
