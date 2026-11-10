# Protocol Profile map — Forever client 1.60.1.70790

**Milestone:** IE-M2 Discovery (branch `feat/m2-protocol`)  
**Date:** 2026-11-05 (Europe/Rome)  
**Worktree:** `<ENGINE_ROOT>\wt_m2` @ `545d9a2c93`  
**Policy:** one active profile per deployment (ADR-002). Boot-only load (no hot-swap).  
**IT:** [`PROTOCOL_PROFILE_70790.it.md`](PROTOCOL_PROFILE_70790.it.md)

This document inventories where Forever protocol data is hardcoded today, quantifies the opcode surface, and proposes the concrete on-disk Profile package for build **70790**. No production/Trinity behavior is changed by this Discovery.

---

## 1. Active profile today

| Item | Value |
|---|---|
| ACTIVE pointer | `forever/client_profiles/ACTIVE` → `1.60.1.70790` |
| Package | `forever/client_profiles/1.60.1.70790/` |
| Manifest | `manifest.json` (`client_build`: 70790, status `active`) |
| ExtraInserts | `extra_inserts.conf`: Client `0x440071,0x440072`; Server `0x470125` |
| Critical gate | `opcodes_table.md`: CREATE `0x440070`→`0x430070`; CHECK NAME `0x440073`→`0x430071` |
| C++ foundation | `src/server/game/Forever/{ClientProfile,ClassicOpcodeRemap,ProtocolApiAdapter,ProtocolSessionBridge}.*` |
| Frozen API | `include/aeternum/api/protocol/{opcode,profile}.hpp` |

---

## 2. Hardcoding inventory (file:line)

### 2.1 Opcode enums and handler tables

| Concern | Location | Notes |
|---|---|---|
| `OpcodeClient` enum values | `Opcodes.h:87+` (e.g. `CMSG_AUTH_SESSION=0x440001` L87, `CMSG_CREATE_CHARACTER=0x430070` L313, `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY=0x430071` L237, `CMSG_PING=0x440006` L772) | **1032** `CMSG_*` assignments |
| `OpcodeServer` enum values | `Opcodes.h` server enum block | **1411** `SMSG_*` assignments (1× `UNKNOWN_OPCODE`) |
| Dense client table size | `Opcodes.h:1079` `NUM_CMSG_OPCODES = 1984` | Preallocated `std::array` of unique_ptr handlers |
| Dense server table size | `Opcodes.h:2529` `NUM_SMSG_OPCODES = 1671` | |
| Array index helpers | `Opcodes.h:1081` / `2531` `GetOpcodeArrayIndex` | Hot-path O(1) |
| Handler registration | `Opcodes.cpp:136` `OpcodeTable::Initialize` + `DEFINE_HANDLER` / `DEFINE_SERVER_OPCODE_HANDLER` | Boot-only fill |
| Client / server handler statuses | `Opcodes.cpp` | See §3 |

### 2.2 Forever wire ↔ core translation (hardcoded Forever deltas)

| Concern | Location | Notes |
|---|---|---|
| Public translate API | `ClassicOpcodes.h:26-49` | `TranslateClientOpcode`, `TranslateServerOpcode`, `*70245`, `IsBuild70291OrLater`, `IsBuild70425OrLater`, block/stub helpers |
| Client translate (ExtraInserts → group shift → 70245 table) | `ClassicOpcodes.cpp:160-180` | Per-packet recv path |
| Server translate | `ClassicOpcodes.cpp:182-196` | Per-packet send path |
| 70245 client map + groupâˆ’1 fallback | `ClassicOpcodes.cpp:198-208` | Uses `ClientOpcodes` map from `.inc` |
| 70245 server special cases (health/chat/guild) | `ClassicOpcodes.cpp:210-239` | Forever-specific index surgery |
| Connection/main group constants | `ClassicOpcodes.cpp:36-88` | `0x45/0x44`, `0x4D/0x4C`, retail group ranges |
| ExtraGroupShift from realm build | `ClassicOpcodes.cpp:97-106` | `realm->Build >= 70291` → shift 1 |
| ExtraInserts source | `ClassicOpcodes.cpp:115-123` → `Forever::Effective*ExtraInserts()` | Profile auto-apply (M6) or conf |
| Pure Remap helpers | `Forever/ClassicOpcodeRemap.h:17-93` | Shared with Catch2 + API adapter |
| Generated CMSG wire→core pairs | `ClassicClientOpcodes.inc` (**687** entries) | Baseline Classic 70009→retail; later inserts layered on top |
| Session bridge seat | `Forever/ProtocolSessionBridge.*`; call site `ClassicOpcodes.cpp:128-133` | `IOpcodeRegistry` process seat; bit-identical Remap |
| World init | `World.cpp:1286` `Forever::ProtocolSession::InitAtStartup()` | Boot |

### 2.3 WorldSocket / WorldSession plug-in points

| Concern | Location | Notes |
|---|---|---|
| Recv translate | `WorldSocket.cpp:412-413` `rawOpcode` → `ClassicOpcodes::TranslateClientOpcode` | After decrypt |
| Unknown/unmapped Classic | `WorldSocket.cpp:414-420` | Invalid core opcode → log + **skip** (no disconnect) |
| Send translate | `WorldSocket.cpp:539`, `567`, `584` `TranslateServerOpcode` | Outbound |
| Skip build auth key | `WorldSocket.cpp:747-761` `Network.SkipBuildAuthKeyCheck` | **Required for 70790** (no `build_auth_key` rows) |
| EnterEncryptedMode RegionGroup | `WorldSocket.cpp:968-969`, `1054-1055` conf `Network.EnterEncryptedModeRegionGroup` | Ops value **8** for Forever |

### 2.4 Auth / build validation

| Concern | Location | Notes |
|---|---|---|
| `auth.build_info` + `build_auth_key` load | `ClientBuildInfo.cpp:151+` | Missing key → error unless skip |
| Realm `gamebuild` | Classic group shift uses `realm->Build` | Profile expects **70790** |
| Profile auth notes | `forever/client_profiles/1.60.1.70790/build_auth_keys.md` | Documents SkipBuildAuthKeyCheck |
| LoginREST Forever CA | bnet LoginREST Aeternum chain (ops) | Not opcode table |

### 2.5 Packet / update-field version switches (build gates)

| Concern | Location | Count |
|---|---|---|
| `ClassicOpcodes::IsBuild*` in `UpdateFields.cpp` | entity update layouts | **13** refs |
| Same in `Server/Packets/**` | Guild/Party packet layouts | **2** refs (`GuildPackets.cpp:98`, `PartyPackets.h:612`) |
| DB2 classic empty anim | `DB2Stores.cpp:2022` | build gate 70291 |

These are **core layout switches**, not wire-id remaps. M2 Adapter must eventually own Profile capability flags (e.g. `layout.unit_data=70425`) so new Forever builds do not scatter more `IsBuildXXXX` into gameplay TUs.

### 2.6 Existing profile / seam docs

| Artifact | Path |
|---|---|
| Opcode seams | `forever/client_profiles/1.60.1.70790/seams/opcode_seams.md` |
| Hotfix seams | `.../seams/hotfix_seams.md` |
| Case study CREATE | `.../CASE_STUDIES/70755_to_70790_create_char.md` |
| Golden corpus index | `.../goldens/corpus.manifest.json` + `samples/70790_create_remap.jsonl` |
| Prior coverage report | `docs/development/en/OPCODE_COVERAGE_REPORT.md` |
| Prior adapter sketch | `docs/independent_engine/architecture/modules/PROTOCOL_ADAPTER_ARCHITECTURE.md` |

---

## 3. Quantification (HEAD `545d9a2c93`)

### 3.1 Core handler table (`Opcodes.cpp`)

| Metric | Count |
|---:|---:|
| `CMSG_*` enum assignments (`Opcodes.h`) | **1032** |
| `DEFINE_HANDLER(CMSG_*)` | **1032** |
| `STATUS_LOGGEDIN` | 586 |
| `STATUS_AUTHED` | 39 |
| `STATUS_TRANSFER` | 2 |
| `STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT` | 1 |
| **Active status (sum above)** | **628** |
| `STATUS_UNHANDLED` | **380** |
| `STATUS_IGNORED` | 16 |
| `STATUS_NEVER` | 8 |
| `SMSG_*` enum assignments | **1411** |
| `DEFINE_SERVER_OPCODE_HANDLER(SMSG_*)` | **1410** |
| Server `STATUS_NEVER` | 750 |
| Server `STATUS_UNHANDLED` | 660 |
| Dense client slots `NUM_CMSG_OPCODES` | 1984 |
| Dense server slots `NUM_SMSG_OPCODES` | 1671 |
| `ClassicClientOpcodes.inc` map entries | **687** |

Interpretation: ~**37%** of CMSG handlers are explicitly `STATUS_UNHANDLED` (Forever client still sends many; baseline noise). Server table is send-side stubs.

### 3.2 Forever-specific deltas vs upstream (git-detectable)

| Commit | Signal |
|---|---|
| `c3b9a5e9ad` | Initial Forever/Trinity snapshot |
| `bd00b5bb39` | Config-driven opcode inserts + trace |
| `b81c5aa218` / `bc588fd9d5` | 70755 layouts, empty-probe opcode |
| `50f1635051` | M4 ClientProfile foundation |
| `c85b09c286` | M6 ExtraInserts auto-apply |
| `e1a761e9b6` / `8e359cced0` | IE-M1 protocol API + session bridge |

**Detectable Forever deltas (not in stock Trinity retail):**

1. Entire `ClassicOpcodes.*` + `ClassicClientOpcodes.inc` translation layer.
2. ExtraInserts conf/profile (`0x440071/72`, `0x470125` for 70790).
3. Group-shift and main-group insert index heuristics.
4. Forever empty-probe / Classic-only CMSG stubs (coverage report).
5. Auth/handshake ops knobs: `SkipBuildAuthKeyCheck`, `EnterEncryptedModeRegionGroup=8`, Aeternum TLS chain.
6. Scattered `IsBuild70291OrLater` / `70425` layout forks.

Exact opcode-by-opcode diff vs upstream Trinity retail is **not** a clean `git diff` (tree started as Forever snapshot). Use sniff corpus + `ClassicClientOpcodes.inc` as the Forever delta catalog going forward.

### 3.3 Known unknown wire

| Wire | Status | Source |
|---|---|---|
| `0x3D013D` | No enum / no `.inc` hit | Client test / IE-M1 captures; nearest retail `0x3D013B` / `0x3D0140` |

Policy: log-once + metric + drop (see Adapter design). Not a Core bug until Profile maps it.

---

## 4. Proposed Profile package structure (70790)

Canonical root (unchanged decision): `forever/client_profiles/1.60.1.70790/`.

```text
forever/client_profiles/1.60.1.70790/
├── manifest.json                 # identity + integrity + load policy (extended schema below)
├── extra_inserts.conf            # ConfigMgr-compatible lists (KEEP; runtime today)
├── opcodes/
│   ├── critical.json             # CREATE/CHECK gates (machine-readable opcodes_table)
│   ├── wire_to_core.table.json   # optional export of ClassicClientOpcodes.inc
│   └── unknown_policy.json       # drop|ignorelist for wires like 0x3D013D
├── handshake/
│   └── flags.json                # SkipBuildAuthKeyCheck, RegionGroup, OpcodeGroupShift, insert indices
├── layouts/
│   └── gates.json                # capability flags replacing naked IsBuild70291/70425
├── goldens/                      # existing corpus
├── seams/                        # existing
├── CASE_STUDIES/
└── tests/
```

Existing markdown remains human docs; JSON is the loader contract for M2 implementation blocks.

### 4.1 `manifest.json` schema (proposed extension)

```json
{
  "profile_id": "1.60.1.70790",
  "product": "1.60.1",
  "client_build": 70790,
  "status": "active",
  "load_policy": "boot_only",
  "integrity": {
    "files": [
      { "path": "extra_inserts.conf", "sha256": "<hex>" },
      { "path": "opcodes/critical.json", "sha256": "<hex>" },
      { "path": "handshake/flags.json", "sha256": "<hex>" }
    ]
  },
  "extra_inserts": {
    "Classic.ClientExtraInserts": ["0x440071", "0x440072"],
    "Classic.ServerExtraInserts": ["0x470125"]
  },
  "critical_client_opcodes": {
    "CMSG_CREATE_CHARACTER": { "wire": "0x440070", "core_after_remap": "0x430070" },
    "CMSG_CHECK_CHARACTER_NAME_AVAILABILITY": { "wire": "0x440073", "core_after_remap": "0x430071" }
  },
  "auth": {
    "realm_gamebuild": 70790,
    "skip_build_auth_key_check": true,
    "enter_encrypted_mode_region_group": 8
  },
  "unknown_opcode_policy": "log_once_metric_drop"
}
```

### 4.2 Opcode table format

- **critical.json:** array of `{ "name", "wire", "core", "gate": true }` — Catch2 must-not-break.
- **wire_to_core.table.json:** sorted pairs `{ "wire", "core" }` exported from `.inc` (generation script later).
- Runtime must **not** parse JSON on the hot path: boot loader fills dense arrays / insert snapshots once.

### 4.3 Build keys / flags (`handshake/flags.json`)

| Key | 70790 value | Today's conf |
|---|---|---|
| `skip_build_auth_key_check` | true | `Network.SkipBuildAuthKeyCheck` |
| `enter_encrypted_mode_region_group` | 8 | `Network.EnterEncryptedModeRegionGroup` |
| `opcode_group_shift` | auto / `1` if buildâ‰¥70291 | `Classic.OpcodeGroupShift` |
| `client_main_group_insert_index` | `0x13B` default | `Classic.ClientMainGroupInsertIndex` |
| `server_main_group_insert_index` | `0x98` default | `Classic.MainGroupInsertIndex` |

### 4.4 Loading policy

| Rule | Decision |
|---|---|
| When loaded | **Boot only** (worldserver start), after conf + before accepting world sockets |
| Multi-profile | Forbidden at runtime (ADR-002) |
| ACTIVE vs conf drift | Fail-closed or warn+refuse AutoApply (partially in `CheckProfileDataConsistency`) |
| Integrity | Reuse **M1** multi-file SHA-256 (`integrity.files[]`) on profile artifacts |
| Hot reload | Out of scope; restart to flip profile |

### 4.5 Validation

1. JSON schema + SHA-256 of listed files.
2. Catch2 `[classic][profile][active][gate]` CREATE/CHECK.
3. Remap parity: `OpcodeRemap` ↔ `OpcodeRegistryAdapter` (existing goldens).
4. Optional: refuse boot if `realmlist.gamebuild` â‰  `client_build`.

---

## 5. What stays / what moves in M2

| Stay in Core (for now) | Move behind Profile/Adapter |
|---|---|
| `OpcodeTable` handlers / session status | Wire numeric Forever ids + ExtraInserts |
| Gameplay packet Read()/handlers | Forever-only index surgery currently in `Translate*70245` |
| Crypt session keys | RegionGroup / skip-auth flags sourced from Profile |
| Anti-DoS pkt/tick caps | Unknown-opcode policy + telemetry |

---

## 6. References

- Design companion: [`PROTOCOL_ADAPTER_DESIGN.md`](PROTOCOL_ADAPTER_DESIGN.md)
- ADR: [`../adr/IE-ADR-013-protocol-profile-and-adapter.md`](../adr/IE-ADR-013-protocol-profile-and-adapter.md)
- Roadmap IE-M2: `docs/independent_engine/en/AETERNUM_TECHNICAL_ROADMAP.md` §IE-M2
- Coverage: `docs/development/en/OPCODE_COVERAGE_REPORT.md`
