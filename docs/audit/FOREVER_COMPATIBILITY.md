# Forever Compatibility Audit (M0) — DEEP DIVE

**Status:** Implemented (documentation of *today’s* behavior)  
**Client in production:** World of Warcraft Forever **1.60.1.70790**  
**Auth evidence:** `auth.build_info` includes builds …70532, 70658, 70707, 70753, **70755**, **70790**; `auth.realmlist` id=70 `gamebuild=70790`.  
**Priority:** #1 together with Module SDK (maintainer steering 2026-11-04).

Goal of future M4 (not designed here): a Forever client update must **not** require reviewing the whole core — only compatibility components / profiles / handlers / data.

---

## 1. Where client-version-specific logic lives TODAY

| Area | Location | What it does | Isolation quality |
|---|---|---|---|
| Opcode enum (internal) | `src/server/game/Server/Protocol/Opcodes.h` | Numeric IDs used by handlers (`0x43iiii` style after remap) | Low — shared table |
| Opcode table / handlers | `src/server/game/Server/Protocol/Opcodes.cpp` | `DEFINE_HANDLER(...)` bind status/process/fn | Medium |
| Classic wire remap | `src/server/game/Server/Protocol/ClassicOpcodes.h/.cpp` | Maps client wire opcodes ↔ internal; **ClientExtraInserts / ServerExtraInserts** | **Critical hotspot** |
| Config inserts | `worldserver.conf` → `Classic.ClientExtraInserts`, `Classic.ServerExtraInserts` | Lists full wire opcodes (0xGGIIII) inserted in a new build that shift later indexes | Config-driven, fragile if wrong |
| Empty-probe stubs | `Opcodes.h` `CMSG_CLASSIC_EMPTY_PROBE_70755`, `Opcodes.cpp` → `Handle_NULL` | Absorbs new empty client probes | Ad-hoc per build |
| Packet structures | `src/server/game/Server/Packets/*.h/.cpp` | Serialize/deserialize layouts (WHO, party, arena, …) | Scattered; 70755 fixed several |
| World socket / session | `WorldSocket.cpp/.h`, `WorldSession.cpp/.h` | Framing, decrypt, dispatch to opcode handlers | Core path |
| Auth build gate | `auth.build_info` (+ bnet/login path) | Which client builds may authenticate | Data |
| Realm gamebuild | `auth.realmlist.gamebuild` | Advertised build (70 → 70790) | Data |
| Hotfix / DB2 | `hotfixes` DB + `DataStores` / DB2 loaders | Client data mirrors | Data-heavy |
| Player create | handlers + `playercreateinfo*` world tables | Character creation | Mix code+data |
| Aeternum fixes | `sql/custom` + NOTES | Operational Forever SQL / docs | Process |
| Extractors / client path | `(operator files, not included)` | Map/vmap from Forever client folder | Ops |

**Problem:** version deltas are partly config (inserts), partly enum comments, partly packet struct edits, partly SQL. There is **no** first-class `client_profiles/<build>/` boundary yet. Wrong inserts silently remap unrelated opcodes (see case study).

---

## 2. ClientExtraInserts / ServerExtraInserts — mechanism

Source: `ClassicOpcodes.cpp` (comments ~L109–110, functions `ClientExtraInserts`, `ServerExtraInserts`, `RemoveClientExtraInserts`, `AddServerExtraInserts`).

**Idea:** Forever client builds after a baseline can **insert** new messages inside an opcode group. Every later index in that group shifts by one on the wire. Instead of rewriting the entire opcode table for each build, the server:

1. Reads comma-separated full wire opcodes from config (`ParseOpcodeList("Classic.ClientExtraInserts")` / `ServerExtraInserts`).
2. On **inbound** client→server: `RemoveClientExtraInserts(wire)` subtracts the shift so the internal enum still matches handlers.
3. On **outbound** server→client: `AddServerExtraInserts(internal)` re-applies the shift for the wire.

**Current production values (Trinity Engine conf, 2026-11-04 after fix):**

```text
Classic.ClientExtraInserts = "0x440071,0x440072"
Classic.ServerExtraInserts = "0x470125"
```

**Operational hazard observed:** an inline comment appended on the same conf line can be ingested by the parser (Server.log showed a bogus token from the comment). Prefer comments on **separate** lines.

---

## 3. CASE STUDY: 70755 → 70790 character creation break (2026-11-04)

### Timeline (Europe/Rome)

| Time | Event |
|---|---|
| ~18:21 | Last known OK create (character “Arthas”) with older inserts `0x4400C2,0x4400C3` |
| ~20:00 | `apply_70790` set ClientExtraInserts to **`0x440063,0x440064`** (copied from advocaite PR#271 lower-bound for **70755**) |
| 21:54 | Packet log: Finish create no-op |
| ~22:16 | BOM incident on conf + correct inserts applied; worldserver restarted (Trinity Engine) |
| Later | maintainer confirmed character creation works again |

### Evidence (Opcodes log / NOTES)

From `sql/custom` and `_fixes/charcreate_70790_20261104/NOTES.md`:

- Wire `0x440073` → correctly `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY`
- Wire `0x440070` → **wrongly** mapped to `CMSG_PET_BATTLE_REPLACE_FRONT_PET` (size 101)
- Result: `Received not handled opcode` / `Handle_NULL` → **Finish does nothing**

### Root cause

On **70790**, create stays at wire `0x440070` and check at `0x440073`. The two new main-group opcodes sit at **`0x71` / `0x72`** (between them), **not** at `0x63` / `0x64`. Using 70755’s insert numbers shifted the create opcode into the pet-battle handler.

### Minimal fix (conf only)

```text
Classic.ClientExtraInserts = "0x440071,0x440072"
# do NOT restore 0xC2,0xC3 on 70790 (breaks CHECK)
# do NOT revert the whole apply_70790
```

### Lessons for Aeternum (requirements for M4 — listed as audit conclusions, not designs)

1. Insert lists are **build-profile data**, not tribal knowledge.  
2. Upstream/community PR numbers for build N must be **re-validated** with a sniff against build N+1.  
3. Need automated **opcode remap tests** (wire→handler) for critical CMsgs: create, who, party, arena join, …  
4. A wrong remap fails **silently** from the player’s POV (button does nothing) — observability must flag UNHANDLED on expected flows.  
5. Client update procedure today is manual (sniff + conf + NOTES). It must become a **repeatable checklist** with artifacts under a profile directory.

---

## 4. Other Forever protocol fixes already in lineage (preserve)

| Topic | Commit / note | Status |
|---|---|---|
| CMSG_WHO / PARTY_UNINVITE / BATTLEMASTER_JOIN_ARENA layouts | `994fc711e0` (70755) | Implemented |
| Empty probe `0x4401B5` → internal empty probe / Handle_NULL | `f48d68094b` + later bind shift notes | Implemented |
| GetTitleIcons stub (bnet/world) | `81da06a919` / `8ebd77208b` | Partial stub |
| Config-driven opcode inserts | `bd00b5bb39` | Implemented |
| 70790 support + PR#271 inserts (then corrected) | `91d76624ff` + NOTES | Implemented + hotfix |

---

## 5. Launcher / portal compatibility (MUST keep working)

| Component | Contract |
|---|---|
| bnetserver | TCP **1119** (`BattlenetPort`) |
| LoginREST | HTTP(S) login ticket service (conf `LoginREST.*`; firewall script also mentions 8083) |
| worldserver | **8085** (game), hosts entry pattern `<login-host>` in setup script |
| Website | `https://<your-site>` (`FOREVER_WEBSITE` in `build_launcher_everforge.ps1`) |
| Launcher | Project ItaPatch / third-party (`<LAUNCHER_ROOT>`) + Aeternum `build_launcher*.ps1` reading `auth.realmlist` address |
| Client build | **70790** must remain accepted in `build_info` + realm `gamebuild` |

Aeternum must speak the **same** external contracts unless maintainer approves a coordinated launcher change.

---

## 6. Data plane Forever

- Databases on MySQL **8.0.46** port **3308** (Trinity Engine): `auth`, `characters`, `world`, `hotfixes`.
- Hotfixes carry Forever ItemSparse/etc.; vendor cleanup 2026-11-04 used ItemSparse presence as Forever ground truth.
- SQL update process: Trinity `sql/updates` + **`sql/custom`** with paired rollbacks and README/NOTES (operational strength to preserve).

---

## 7. Gaps (compatibility)

| Gap | Risk |
|---|---|
| No formal client profile directory | Knowledge lives in conf comments + NOTES |
| No automated wire↔handler matrix | Regressions like create-char recur |
| Packet structs still edited ad-hoc in `Packets/` | Hard to review per build |
| Upstream Forever forks **not** on 70790 (advocaite PR#271 = 70755; forever branch ~70658) | Cannot copy inserts blindly |
| Remaining ~6.4–7.5k DBErrors after pending restart | Mix of safe leftovers and Forever incompleteness |
| Guides still mention older builds | Ops confusion |

---

## 8. Ranking implication

In `AUDIT_REPORT.md` section G, **Client compatibility architecture + Module SDK** are priority **#1** (criticality max). Performance work must not precede isolation of this surface.
