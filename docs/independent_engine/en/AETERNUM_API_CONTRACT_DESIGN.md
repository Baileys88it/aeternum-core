# AETERNUM_API_CONTRACT_DESIGN

**Status:** Architectural sketches only — not implemented  
**Date:** 2026-11-05 (Europe/Rome)  
**Constraint:** Public headers MUST NOT expose `Player*`, `WorldSession*`, `Map*`, `Unit*`, `Creature*`, `WorldObject*`.

## 0. Cross-cutting contract rules

| Concern | Rule |
|---|---|
| Ownership | Interfaces owned by Aeternum domain libraries; modules never own engine singletons |
| Lifetime | Host outlives modules; EntityId validity ends at despawn/invalidate event |
| Threads | Documented affinity per method; cross-thread calls either forbidden or marshaled |
| Exceptions | No exceptions across module SDK boundary — `Status` / `Result<T>` only |
| ABI | IE-M1..M3: static link, no C ABI promise; dynamic ABI only if IE-ADR-012 accepts |
| Versioning | `AETERNUM_SDK_VERSION_*` + module `sdkRange`; HostDiscover rejects incompatible |
[redacted-sensitive-line]
| Trinity | Allowed only inside `adapters/trinitycore` TUs |

```cpp
// common sketch
namespace aeternum {
enum class Status : int { Ok=0, InvalidArgument, NotFound, NotReady, Denied, Unsupported, Internal };
template<class T> struct Result { Status status; T value; char const* message; };
}
```

---

## 1. Engine lifecycle — `aeternum::kernel`

**Purpose:** boot/drain/shutdown without including World.h  
**Owner:** Kernel domain  
**Today’s reality:** `worldserver/Main.cpp`, `World::StopNow`, STOP bat

```cpp
namespace aeternum::kernel {
struct EngineConfigSnapshot {
  std::string profileId;
  std::string confPath;
  // ports as numbers only — no connection strings
  uint16 worldPort; uint16 bnetPort;
};

class IEngineHost {
public:
  virtual ~IEngineHost() = default;
  virtual Status Configure(EngineConfigSnapshot const&) = 0; // main thread
  virtual Status Start() = 0;                                 // main thread
  virtual Status RequestDrain(std::chrono::seconds grace) = 0; // thread-safe
  virtual void Shutdown() = 0;                                // main thread after drain
  virtual bool IsRunning() const = 0;
};
}
```

| Method | Thread | Errors | Notes |
|---|---|---|---|
| Configure | boot main | InvalidArgument | validates profile ACTIVE vs conf |
| Start | boot main | NotReady/Internal | wraps SetInitialWorldSettings failure |
| RequestDrain | any | Ok/Denied | maps to shutdown initiate / admin drain |
| Shutdown | main | — | joins map updater, stops sockets |

**ABI:** abstract class + factory in adapter `CreateTrinityEngineHost()`.

---

## 2. Service registry

**Purpose:** locate domain façades without god-object `sWorld`  
**Owner:** Kernel  
**Thread:** register at boot only; Get may be concurrent if services immutable after Start

```cpp
class IServiceRegistry {
public:
  virtual void Register(std::type_index, std::shared_ptr<void>) = 0;
  virtual std::shared_ptr<void> Get(std::type_index) const = 0;
};
// typed helpers Register<T>/Get<T> in header-only wrapper
```

**Forbidden:** registering raw `Player*` or `World*`.

---

## 3. Client profile & sessions (protocol)

```cpp
namespace aeternum::protocol {
struct BuildId { uint32 value; };
struct WireOpcode { uint32 raw; };

class IClientProfile {
public:
  virtual std::string_view ProfileId() const = 0;
  virtual BuildId Build() const = 0;
  virtual std::vector<uint32> ClientExtraInserts() const = 0;
  virtual std::vector<uint32> ServerExtraInserts() const = 0;
  virtual Status ReloadFromDisk() = 0; // boot or GM-controlled
};

class IOpcodeMap {
public:
  virtual Result<WireOpcode> WireToCore(WireOpcode) const = 0;
  virtual Result<WireOpcode> CoreToWire(WireOpcode) const = 0;
};

struct DecodedMessage {
  WireOpcode coreOpcode;
  std::vector<std::byte> payload; // still opaque until typed codecs exist
};

class IPacketCodec {
public:
  virtual Result<DecodedMessage> Decode(WireOpcode wire, std::span<std::byte const>) const = 0;
  virtual Result<std::vector<std::byte>> Encode(DecodedMessage const&) const = 0;
};

// Session handle — NOT WorldSession*
struct SessionId { uint64 value; };
class ISessionDirectory {
public:
  virtual Result<SessionId> FindByAccount(uint32 accountId) const = 0;
  virtual Status Disconnect(SessionId, char const* reason) = 0; // marshaled to world thread
};
}
```

**Maps to:** `Forever::ClientProfile*`, `ClassicOpcodeRemap`, `ClassicOpcodes::Translate*`, future Packets extraction.  
**Thread:** Decode/Encode pure/reentrant; Disconnect marshaled.  
**Tests:** existing Catch2 remap + new golden vectors (IE-M2/M6).

---

## 4. Entity handles & world query

```cpp
namespace aeternum::world {
struct EntityId { uint64 guid; }; // opaque; adapter maps ObjectGuid
struct MapId { uint32 id; uint32 instanceId; };

struct EntitySnapshot { // POD for modules/ops
  EntityId id; MapId map; uint32 entry; float x,y,z; uint8 level;
  // no pointers
};

class IWorldView { // read-only
public:
  virtual bool Exists(EntityId) const = 0;          // world thread or synced snapshot
  virtual Result<EntitySnapshot> Get(EntityId) const = 0;
  virtual Result<MapId> MapOf(EntityId) const = 0;
};

class IWorldCommands { // mutations
public:
  virtual Status Enqueue(EntityId, /*command POD*/) = 0; // always async to world thread
};
}
```

**Ownership:** World domain; adapter holds `ObjectAccessor` internally.  
**Lifetime:** snapshots may be copied under lock; never hand out Trinity pointers.  
**Risk:** stale snapshots — document TTL / generation counters (open question).

---

## 5. Gameplay services (sketch)

```cpp
namespace aeternum::gameplay {
struct CastSpellRequest { EntityId caster; EntityId target; uint32 spellId; };
struct CastSpellResult { Status status; uint32 spellId; };
class ISpellService {
public:
  virtual Result<CastSpellResult> TryCast(CastSpellRequest) = 0; // world thread
};
}
```

Initial surface intentionally tiny — expand only with tests.

---

## 6. Persistence repositories

```cpp
namespace aeternum::persist {
struct CharacterSummary { EntityId id; std::string name; uint8 level; uint8 classId; };

class IUnitOfWork {
public:
  virtual Status Commit() = 0;
  virtual void Rollback() = 0;
};

class ICharacterRepository {
public:
  virtual Result<CharacterSummary> LoadSummary(EntityId) = 0;
  virtual Result<std::vector<CharacterSummary>> ListForAccount(uint32 accountId) = 0;
  virtual Status SaveSummary(CharacterSummary const&) = 0; // rare; prefer engine save path
};
}
```

**Owner:** Persistence domain  
**Adapter:** wraps CharacterDatabase prepared statements  
**Thread:** DB pool async; completion on world/session thread via callbacks  
**Forbidden in public headers:** `Field`, `PreparedStatement`, `QueryResult`

---

## 7. Data catalog

```cpp
namespace aeternum::data {
struct CreatureTemplatePOD { uint32 entry; std::string name; uint32 faction; /*…*/ };
class IDataCatalog {
public:
  virtual Result<CreatureTemplatePOD> CreatureTemplate(uint32 entry) const = 0;
};
}
```

Backed by ObjectMgr/DB2 adapters initially.

---

## 8. Module ABI / versioning

Evolve current draft:

```cpp
namespace aeternum::sdk {
#define AETERNUM_SDK_VERSION_MAJOR 0 // until 1.0 freeze
struct ModuleIdentity { char const* id; char const* version; char const* sdkRange; };
class IModule {
public:
  virtual ModuleDescriptor const& GetDescriptor() const = 0;
  virtual ModuleResult OnLoad() = 0;
  virtual ModuleResult OnInit() = 0;
  virtual ModuleResult OnStart() = 0;
  virtual ModuleResult OnStop() = 0;
  virtual ModuleResult OnShutdown() = 0;
};

// HostServices injected — no ScriptMgr.h
struct HostServices {
  IWorldView* worldView;          // non-owning, valid between OnStart..OnStop
  IMetricProbe* metrics;
  void (*logInfo)(char const*, char const*);
};
}
```

**Versioning:** before 1.0, breaking changes allowed with draft banner; at 1.0, semver + HostDiscover enforce sdkRange.  
**ABI:** static only until IE-ADR-012.  
**Exceptions:** none across boundary.  
**Today gap:** HostServices incomplete; modules use Trinity includes.

---

## 9. Monitoring / operations

```cpp
namespace aeternum::ops {
class IMetricProbe {
public:
  virtual void Gauge(char const* name, double value) = 0;
  virtual void TimingMs(char const* name, double ms) = 0;
};
struct HealthReport { bool ready; uint32 players; uint32 sessions; uint64 dbQueueWorld; };
class IHealth {
public:
  virtual HealthReport Snapshot() const = 0; // thread-safe snapshot
};
class IAuditLog {
public:
  virtual void Write(char const* actor, char const* action, char const* detail) = 0;
};
}
```

Admin HTTP should depend on these ports only (today reaches `sWorld` / DB queues directly).

---

## 10. Compatibility requirements

- Forever launcher LoginREST + bnet RPC remain external contracts (ports/certs).  
- Single active profile (Accepted ADR-002 spirit).  
- Do not require modules to link `game` for read-only ops after IE-M3.

## 11. Open questions for owner / IE-M1

1. EntityId generation counter vs guid reuse?  
2. Snapshot vs live query for IWorldView?  
3. In-repo `engine/contracts` vs docs-only until M2?


---

## Verified opcode recount note (2026-11-05)

Independent recount (+ QA agree): `DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626** (628 active-status − 2 Handle_NULL). Evidence JSON at ``<buildlogs>`` exists but contains stale 1031/379 — **do not prefer JSON over recount**. Unknown wire `0x3D013D` = **PARTIALLY CONFIRMED**. Catch2 current green = **NOT VERIFIED** this phase; last green log ``<archive>`` (1841 assertions / 110 cases).


## Mapping table: sketch → current code

| Sketch | Current anchor | Gap |
|---|---|---|
| IEngineHost | `worldserver/Main.cpp`, `World::StopNow` | No interface |
| IClientProfile | `Forever::ClientProfile*` + ACTIVE file | Exists as functions, not interface |
| IOpcodeMap | `ClassicOpcodes::Translate*` + OpcodeRemap | Partial pure core |
| IPacketCodec | `Server/Packets/*` + handlers | Not extracted |
| SessionId / ISessionDirectory | `WorldSession` / session mgr | Pointer-based today |
| EntityId / IWorldView | `ObjectGuid` / `ObjectAccessor` | Leak Player* via scripts |
| ICharacterRepository | CharacterDatabase stmts | No port |
| IDataCatalog | ObjectMgr / DB2Stores | No port |
| IModule + HostServices | sdk module.hpp + incomplete HostServices | Host still ScriptMgr |
| IMetricProbe / IHealth | Metric macros / sWorld getters in admin | Admin bypasses ports |

## ABI stability timeline (proposed)
- **Draft (now–IE-M3):** break freely with version bump in draft macros.  
- **1.0 candidate:** only after scenarios 3 and 9 PASS.  
- **Dynamic ABI:** only after IE-ADR-012 acceptance.
