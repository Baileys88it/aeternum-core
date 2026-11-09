# Client login test report â€” Aeternum on Trinity ports (2026-11-05)

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**IT:** [`../it/CLIENT_TEST_REPORT.md`](../it/CLIENT_TEST_REPORT.md)  
**Working conf snapshot (local only, not committed):** `_buildlogs/clienttest_working_20261105/`

## Goal

Remote Forever launcher login against Aeternum using the same public ports as Trinity production (bnet **1119**, LoginREST **8083**, world **8085**), with Trinity stopped and scheduled tasks disabled for the test window.

## Outcome

- Launcher **Gioca** opened the client after LoginREST TLS was aligned with Trinity.
- World auth then succeeded after matching Trinityâ€™s `Network.SkipBuildAuthKeyCheck`.
- maintainer entered far enough to produce an unhandled-opcode sample list (below) for a future coverage report.
- After the test: Aeternum stopped; isolated alt-port confs restored (**2119 / 18083 / 18085**) while keeping port-independent fixes; Trinity production restarted.

## Port-independent fixes to keep (do not lose on soak restores)

| Fix | Why |
|---|---|
| LoginREST `CertificatesFile` / `PrivateKeyFile` = `./Aeternum.chain.pem` / `./Aeternum.key.pem` | Launcher trusts `forever_root_ca`; stock `bnetserver.cert.pem` fails TLS â†’ â€œServer non raggiungibileâ€ |
| `Network.SkipBuildAuthKeyCheck = 1` | Forever **70790** has **no** `auth.build_auth_key` rows (same as Trinity `auth`); with `=0` world logs `Missing client build auth key for build 70790 variant Win/x64/WoW` |
| `Network.EnterEncryptedModeRegionGroup = 8` | Match Trinity Forever |
| `Classic.Hardcore = 0` | Clears missing-key startup warning |
| `Admin.AuditKeepRotated = 3`, `Admin.MetricsDenseDefault = 0` | Admin schema completeness |

**Do not commit** `.pem` files. Cert material stays on the machine under the Aeternum bin (copied from Trinity server dir for the test).

## Launcher Gioca path (summary)

1. Website `GET /api/game/client-build` (accepted builds).
2. TCP reachability on baked server address for **1119** / status, LoginREST **8083** with forever CA.
3. `POST /bnetserver/launcher/login/` â†’ ticket; client portal `<login-host>:1119`.
4. World **8085** `HandleAuthSession` (build key check unless skipped).

## Unhandled opcodes observed (Aeternum world console) â€” input for future coverage report

Logged by maintainer during the client session (not a complete sniff; names as reported):

- `CMSG_SET_EXCLUDED_CHAT_CENSOR_SOURCES`
- `SERVER_VALIDATION_SIGNATURE_REQUEST`
- `EXPIRE_BLEEP_TOKENS`
- `GET_ACCOUNT_CHARACTER_LIST`
- `GET_REMAINING_GAME_TIME`
- unknown `0x3D013D`
- `REQUEST_MYTHIC_PLUS_SEASON_DATA`
- `OPEN_TRADESKILL_NPC`
- `QUEST_SESSION_REQUEST_START`
- `REQUEST_WEEKLY_REWARDS`
- `REQUEST_SCHEDULED_PVP_INFO`
- `GET_CHARACTER_CURRENCY_TRANSFER_LOG`
- `GET_LANDING_PAGE_SHIPMENTS`
- `CLASSIC_EMPTY_PROBE_70755`
- `MOVE_SET_TURN_RATE_CHEAT`
- `REQUEST_INSTANCE_ENCOUNTER_EVENT_SYNC`
- `UPGRADE_RUNEFORGE_LEGENDARY`

Also noted: **arena template has no map ids** (separate data issue).

Full coverage analysis: [OPCODE_COVERAGE_REPORT.md](OPCODE_COVERAGE_REPORT.md).

These are backlog for an opcode-coverage / Forever-compat report; no code change in this docs commit.

## Restore notes

- Working prod-port confs saved under `_buildlogs/clienttest_working_20261105/` (local).
- Isolated baseline bak: `_buildlogs/bak_clienttest_prodports_20261105/`.
- After test: realmlist rolled back to alt port **18085**; Trinity tasks re-enabled and production started from `<SERVER_ROOT>\server`.

## Realm display name

Isolated eternum_auth.realmlist name must remain **Aeternum** (not Trinity). Prod-port test rollbacks may restore address/port but must keep 
ame='Aeternum' (see _buildlogs/bak_clienttest_prodports_20261105/aeternum_realmlist_rollback.sql).


