# Report copertura opcode — profilo client 1.60.1.70790

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Metodo:** analisi sola lettura di `Opcodes.h` / `Opcodes.cpp`, `ClassicClientOpcodes.inc`, seam Classic, più il campione di opcode non gestiti dal test login Aeternum.  
**EN:** [`../en/OPCODE_COVERAGE_REPORT.md`](../en/OPCODE_COVERAGE_REPORT.md)  
**JSON evidenza (locale):** `_buildlogs/opcode_coverage_70790.json`

## Ambito

| Voce | Fonte |
|---|---|
| Profilo Forever attivo | `forever/client_profiles/1.60.1.70790/` (+ `ACTIVE`) |
| Aspettative wire critiche | `opcodes_table.md` (CREATE / CHECK name) |
| Seam di remap | `seams/opcode_seams.md`, `ClassicOpcodes.cpp`, `Forever/ClassicOpcodeRemap.h` |
| Tabella handler core | `Opcodes.cpp` |
| Wire Classic CMSG → core | `ClassicClientOpcodes.inc` (686 entry) |

Nessun tocco al DB `auth` di produzione né a Trinity.

## Tabella handler core (CMSG)

Righe `DEFINE_HANDLER(CMSG_…)` in `Opcodes.cpp` (**1031**; enum **1032** nomi `CMSG_`).

| Status | Conteggio | Significato per Aeternum |
|---|---:|---|
| `STATUS_LOGGEDIN` | 586 | Accettati in-world |
| `STATUS_AUTHED` | 39 | Pre-login / char select |
| `STATUS_TRANSFER` | 2 | Transfer mappa |
| `STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT` | 1 | Finestra logout |
| **Attivi / cablati** | **626** | Status sopra e handler ≠ `Handle_NULL` |
| `STATUS_UNHANDLED` | 379 | Non gestiti (`Handle_NULL`) — backlog retail |
| `STATUS_IGNORED` | 16 | Shop / token / VAS (ignorati) |
| `STATUS_NEVER` | 8 | Sessione early (`AUTH_*`, `PING`, …) fuori tabella normale |

**Server (SMSG):** **1410** righe — **750** `STATUS_NEVER`, **660** `STATUS_UNHANDLED`.

## Layer remap Classic 70790

| Metrica | Valore |
|---|---:|
| Entry in `ClassicClientOpcodes.inc` | 686 |
| Nomi remap senza riga handler | 0 |
| Gate profilo (`opcodes_table.md`) | `CMSG_CREATE_CHARACTER`, `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY` |

Gate Catch2: `[classic][profile][active][gate]` / `[classic][extrainsects]`.

## Campione osservato (test client)

Log dalla console world Aeternum durante la sessione Forever di maintainer (non uno sniff completo):

| Osservato | Nome risolto | Status tabella | Handler |
|---|---|---|---|
| SET_EXCLUDED_CHAT_CENSOR_SOURCES | `CMSG_SET_EXCLUDED_CHAT_CENSOR_SOURCES` | `STATUS_UNHANDLED` | `Handle_NULL` |
| SERVER_VALIDATION_SIGNATURE_REQUEST | `CMSG_SERVER_VALIDATION_SIGNATURE_REQUEST` | `STATUS_UNHANDLED` | `Handle_NULL` |
| EXPIRE_BLEEP_TOKENS | `CMSG_EXPIRE_BLEEP_TOKENS` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_ACCOUNT_CHARACTER_LIST | `CMSG_GET_ACCOUNT_CHARACTER_LIST` | `STATUS_UNHANDLED` | `Handle_NULL` |
| GET_REMAINING_GAME_TIME | `CMSG_GET_REMAINING_GAME_TIME` | `STATUS_UNHANDLED` | `Handle_NULL` |
| unknown `0x3D013D` | *(nessun hit enum/remap)* | **UNKNOWN_WIRE** | — |
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

### Note sul campione

- **15 / 17** sono `CMSG_*` già `STATUS_UNHANDLED` (feature retail non implementate).
- **`CMSG_CLASSIC_EMPTY_PROBE_70755`** è stub intenzionale Forever.
- **`0x3D013D`** assente da `Opcodes.h` / `ClassicClientOpcodes.inc` (vicini `0x3D013B` / `0x3D0140`) → candidato **gap ExtraInserts / remap**.
- Stessa sessione: **arena template senza map id** (dato, non opcode).

## Sintesi copertura (lab 70790)

| Bucket | N | Azione |
|---|---:|---|
| Handler CMSG attivi | 626 | Tenere i gate verdi |
| CMSG UNHANDLED espliciti | 379 | Backlog; priorità solo se serve UX Forever |
| UNHANDLED visti in login | 15 nominati | Bassa priorità se non bloccano UI |
| Wire sconosciuto in login | 1 (`0x3D013D`) | **Investigare ExtraInserts / remap** |
| Tabella remap Classic | 686 | Tutti i nomi hanno riga handler |

## Prossimi passi (nessun codice in questo commit)

1. Dump opcode breve su **solo** Aeternum intorno al gruppo `0x3Dxxxx`.
2. Remap + Catch2 se dopo ExtraInserts è un CMSG noto; altrimenti stub Classic documentato.
3. Non implementare in massa i 379 UNHANDLED retail.
4. Moduli gameplay restano proposal.

## Doc collegate

- [CLIENT_TEST_REPORT.md](CLIENT_TEST_REPORT.md)  
- [QUALITY_CHECKLIST.md](QUALITY_CHECKLIST.md)  
- Profilo: `forever/client_profiles/1.60.1.70790/`