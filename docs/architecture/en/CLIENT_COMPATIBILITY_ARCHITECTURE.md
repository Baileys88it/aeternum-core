# Client Compatibility Architecture (M1)

**Status:** Accepted (M2) — Priority #1 with Module SDK  
**Policy:** **Single active client profile** per server deployment (maintainer). Switching Forever builds = activate the next profile artifacts, not run N profiles at once.  
**Italian:** [`../it/CLIENT_COMPATIBILITY_ARCHITECTURE.md`](../it/CLIENT_COMPATIBILITY_ARCHITECTURE.md)

## Problem

Forever client builds insert/reorder opcodes mid-group. Today remap lives in:

- `ClassicOpcodes.cpp` (`ClientExtraInserts` / `ServerExtraInserts` via conf)
- `Opcodes.h/.cpp` handler table + empty-probe stubs
- `Server/Packets/*` layout structs
- `auth.build_info` + `realmlist.gamebuild`
- hotfixes / DB2 expectations
- tribal NOTES under `sql/custom`

Wrong inserts **silently** remap unrelated handlers (see case study).

## Solution (Proposed)

Introduce a **Client Profile** package (canonical path (Accepted): `forever/client_profiles/<product>.<build>/` e.g. `forever/client_profiles/1.60.1.70790/`):

```text
forever/client_profiles/
└── 70790/                    # example — one active
    ├── manifest.yaml         # build id, min/max, created, notes
    ├── extra_inserts.conf    # ClientExtraInserts + ServerExtraInserts values only
    ├── opcodes_expectations.md / .json   # critical CMsg wire→handler map
    ├── packet_adapters/      # docs + future code pointers for layout deltas
    ├── auth_expectations.md  # build_info rows, realm gamebuild
    ├── data_expectations.md  # hotfix/DB2 tables required
    ├── tests/                # Catch2 cases + manual checklist
    └── CASE_STUDIES/         # e.g. 70755_to_70790_create_char.md
```

**Runtime rule:** exactly one profile marked `active` in conf (e.g. `Classic.ActiveClientProfile = 70790`). Loading two = hard fail at startup (Planned enforcement).

### Profile contents

| Artifact | Purpose |
|---|---|
| Manifest | Build number, Forever product label, author, date, git rev |
| ExtraInserts | Canonical insert lists (comments **forbidden** on value line) |
| Opcode expectations | Table of critical wire opcodes → internal handler names |
| Packet adapters | Documented deltas vs previous profile; code changes localized |
| Auth keys | Required `build_info` entries; realm `gamebuild` |
| Data/hotfix expectations | Minimum hotfixes (e.g. ItemSparse presence rules) |
| Tests | Automated remap unit tests + mandatory create-char smoke |

## Client-update workflow (1–12)

| Step | Action | Automatable? |
|---|---|---|
| 1 | Discover new Forever client build | Partial (manual announce + folder detect) |
| 2 | Identify protocol differences vs active profile | Partial — sniff/log diff tooling **Planned** |
| 3 | Compare opcodes (wire list) | **Yes** — parse opcode logs / packet captures |
| 4 | Compare packet structures | Partial — needs structured fixtures |
| 5 | Compare data formats / DB2 | Partial — extract + schema diff |
| 6 | Identify affected handlers | Partial — intersect shifted ranges with handler table |
| 7 | Generate compatibility report | **Yes** — report generator from diffs |
| 8 | Implement adapter changes in **new profile** only | Manual (code) + review |
| 9 | Run automated protocol tests | **Yes** — Catch2 remap + packet fixtures |
| 10 | Run integration tests | Partial |
| 11 | Validate against real client (login, **character create**, who/party/arena) | Manual gate — **mandatory** |
| 12 | Approve & publish profile; flip `ActiveClientProfile`; retire old | Manual approval |

### Validation gates (must pass before flip)

1. Conf parser accepts ExtraInserts (no BOM; no inline comment tokens).  
2. Unit: wire `CREATE` / `CHECK_NAME` / `WHO` / party / arena map to expected handlers.  
3. **Character-creation smoke on real client** (regression mandatory after 70790 incident).  
4. Launcher still reaches bnet 1119 + world 8085.  
5. `build_info` contains new build; realm `gamebuild` updated in same change set.

## Case study: 70755 → 70790 character create (2026-11-04)

**Evidence:** `sql/custom`, `_fixes/charcreate_70790_20261104/NOTES.md`, `ClassicOpcodes.cpp`.

| | |
|---|---|
| Broken inserts | `0x440063,0x440064` (copied from 70755 / PR#271 lower bound) |
| Symptom | Wire `0x440070` → `CMSG_PET_BATTLE_REPLACE_FRONT_PET`; Finish no-op |
| Fix | `0x440071,0x440072` (inserts **between** CREATE `0x440070` and CHECK `0x440073`) |
| Lesson | Community/PR inserts for build N are not valid for N+1 without sniff; profile must own numbers + tests |

## Non-goals

- Hot-swap profile without restart in M4 (restart-bound flip is acceptable first).  
- Multi-build concurrent support.  
- Claiming full automatic compatibility.
