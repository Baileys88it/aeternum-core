# Opcode coverage report — client profile 1.60.1.70790

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Method:** read-only analysis of `Opcodes.h` / `Opcodes.cpp`, `ClassicClientOpcodes.inc`, Classic remap seams, plus the unhandled sample logged during the Aeternum client login test.  
**IT:** [`../it/OPCODE_COVERAGE_REPORT.md`](../it/OPCODE_COVERAGE_REPORT.md)  
**Evidence JSON (local):** `_buildlogs/opcode_coverage_70790.json`

## Scope

| Item | Source of truth |
|---|---|
| Active Forever profile | `forever/client_profiles/1.60.1.70790/` (+ `ACTIVE`) |
| Critical wire expectations | `opcodes_table.md` (CREATE / CHECK name) |
| Remap seams | `seams/opcode_seams.md`, `ClassicOpcodes.cpp`, `Forever/ClassicOpcodeRemap.h` |
| Core handler table | `src/server/game/Server/Protocol/Opcodes.cpp` |
| Classic CMSG wire → core | `ClassicClientOpcodes.inc` (686 mapped entries) |

No production `auth` DB and no Trinity install were touched for this report.

## Core handler table (CMSG)

Parsed `DEFINE_HANDLER(CMSG_…)` rows in `Opcodes.cpp` (**1031** handlers; enum has **1032** `CMSG_` names).

| Status | Count | Meaning for Aeternum |
|---|---:|---|
| `STATUS_LOGGEDIN` | 586 | Accepted in-world (most gameplay) |
| `STATUS_AUTHED` | 39 | Pre-login / char select path |
| `STATUS_TRANSFER` | 2 | Map transfer |
| `STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT` | 1 | Edge logout window |
| **Active / wired** | **626** | Status above and handler ≠ `Handle_NULL` |
| `STATUS_UNHANDLED` | 379 | Explicitly unhandled (`Handle_NULL`) — expected retail backlog |
| `STATUS_IGNORED` | 16 | Shop / token / VAS noise (silently ignored) |
| `STATUS_NEVER` | 8 | Early session opcodes (`AUTH_*`, `PING`, …) processed outside the normal table |

**Server (SMSG) table:** **1410** `DEFINE_SERVER_OPCODE_HANDLER` rows — **750** `STATUS_NEVER`, **660** `STATUS_UNHANDLED` (server opcodes are send-side; “unhandled” here means no connection-type / status wiring beyond the stub defaults).

## Classic 70790 remap layer

| Metric | Value |
|---|---:|
| Entries in `ClassicClientOpcodes.inc` | 686 |
| Remap target names missing from handler table | 0 |
| Profile gate opcodes (`opcodes_table.md`) | `CMSG_CREATE_CHARACTER`, `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY` |

Must-not-break: Catch2 `[classic][profile][active][gate]` / `[classic][extrainsects]` (see quality checklist).

## Observed unhandled sample (client test)

Logged on the Aeternum world console during the maintainer’s Forever launcher session (not a full sniff). Mapped against the handler table:

| Observed | Resolved name | Table status | Handler |
|---|---|---|---|
| SET_EXCLUDED_CHAT_CENSOR_SOURCES | `CMSG_SET_EXCLUDED_CHAT_CENSOR_SOURCES` | `STATUS_UNHANDLED` | `Handle_NULL` |
| SERVER_VALIDATION_SIGNATURE_REQUEST | `CMSG_SERVER_VALIDATION_SIGNATURE_REQUEST` | `STATUS_UNHANDLED` | `Handle_NULL` |
| EXPIRE_BLEEP_TOKENS | `CMSG_EXPIRE_BLEEP_TOKENS` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_ACCOUNT_CHARACTER_LIST | `CMSG_GET_ACCOUNT_CHARACTER_LIST` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_REMAINING_GAME_TIME | `CMSG_GET_REMAINING_GAME_TIME` | `STATUS_UNHANDLED` | `Handle_NULL` |
| unknown `0x3D013D` | *(no enum / remap hit)* | **UNKNOWN_WIRE** | — |
| REQUEST_MYTHIC_PLUS_SEASON_DATA | `CMSG_REQUEST_MYTHIC_PLUS_SEASON_DATA` | `STATUS_UNHANDLED` | `Handle_NULL` |
| OPEN_TRADESKILL_NPC | `CMSG_OPEN_TRADESKILL_NPC` | `STATUS_UNHANDLED` | `Handle_NULL` |
| QUEST_SESSION_REQUEST_START | `CMSG_QUEST_SESSION_REQUEST_START` | `STATUS_UNHANDLED` | `Handle_NULL` |
| REQUEST_WEEKLY_REWARDS | `CMSG_REQUEST_WEEKLY_REWARDS` | `STATUS_UNHANDLED` | `Handle_NULL` |
| REQUEST_SCHEDULED_PVP_INFO | `CMSG_REQUEST_SCHEDULED_PVP_INFO` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_CHARACTER_CURRENCY_TRANSFER_LOG | `CMSG_GET_CHARACTER_CURRENCY_TRANSFER_LOG` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_LANDING_PAGE_SHIPMENTS | `CMSG_GET_LANDING_PAGE_SHIPMENTS` | `STATUS_UNHANDLED` | `Handle_NULL` |
| CLASSIC_EMPTY_PROBE_70755 | `CMSG_CLASSIC_EMPTY_PROBE_70755` | `STATUS_LOGGEDIN` | `Handle_NULL` (stub) |
| MOVE_SET_TURN_RATE_CHEAT | `CMSG_MOVE_SET_TURN_RATE_CHEAT` | `STATUS_UNHANDLED` | `Handle_NULL` |
| REQUEST_INSTANCE_ENCOUNTER_EVENT_SYNC | `CMSG_REQUEST_INSTANCE_ENCOUNTER_EVENT_SYNC` | `STATUS_UNHANDLED` | `Handle_NULL` |
| UPGRADE_RUNEFORGE_LEGENDARY | `CMSG_UPGRADE_RUNEFORGE_LEGENDARY` | `STATUS_UNHANDLED` | `Handle_NULL` |

### Notes on the sample

- **15 / 17** resolve to known `CMSG_*` entries already marked `STATUS_UNHANDLED` (retail features not implemented in this core). They are noise for Classic Forever play unless a specific UX needs them.
- **`CMSG_CLASSIC_EMPTY_PROBE_70755`** is intentionally stubbed (`STATUS_LOGGEDIN` + `Handle_NULL`) for Forever Classic probe traffic.
- **`0x3D013D`** does **not** appear in `Opcodes.h` or `ClassicClientOpcodes.inc`. Nearest retail CMSG values sit at `0x3D013B` / `0x3D0140`. Treat as a **Classic wire / ExtraInserts gap** candidate — needs a sniff + remap entry before a handler can be attached.
- Separate data issue from the same session: **arena template has no map ids** (not an opcode gap).

## Coverage summary (70790 lab)

| Bucket | Count | Action |
|---|---:|---|
| Active CMSG handlers | 626 | Keep regression gates green |
| Explicit UNHANDLED CMSG | 379 | Backlog; prioritize only if Forever UX needs them |
| Observed UNHANDLED in live login | 15 named | Low priority unless blocking UI |
| Unknown wire in live login | 1 (`0x3D013D`) | **Investigate ExtraInserts / remap** |
| Classic remap table | 686 | All names have a handler row |

## Recommended next steps (no code in this commit)

1. Capture a short opcode dump around char-list → world enter with `Logger.network.opcode` elevated on **Aeternum only**, focused on group `0x3Dxxxx`, to identify `0x3D013D`.
2. If it remaps to a known retail CMSG after ExtraInserts, add/adjust `ClassicClientOpcodes.inc` + Catch2 gate; if it is Forever-only, document a Classic stub.
3. Do **not** mass-implement the 379 `STATUS_UNHANDLED` retail opcodes — triage against Forever Classic feature needs.
4. Gameplay module work stays at proposal stage (per roadmap).

## Related docs

- [CLIENT_TEST_REPORT.md](CLIENT_TEST_REPORT.md) — original observed list  
- [QUALITY_CHECKLIST.md](QUALITY_CHECKLIST.md) — Catch2 opcode gates  
- Profile: `forever/client_profiles/1.60.1.70790/`
