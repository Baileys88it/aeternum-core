# Report test login client â€” Aeternum sulle porte Trinity (2026-11-05)

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**EN:** [`../en/CLIENT_TEST_REPORT.md`](../en/CLIENT_TEST_REPORT.md)  
**Snapshot conf locali (non in git):** `_buildlogs/clienttest_working_20261105/`

## Obiettivo

Login remoto col Forever launcher su Aeternum usando le stesse porte pubbliche di Trinity (bnet **1119**, LoginREST **8083**, world **8085**), con Trinity fermo e task pianificati disabilitati durante il test.

## Esito

- Dopo allineamento TLS LoginREST a Trinity, **Gioca** apre il client.
- Auth world OK dopo `Network.SkipBuildAuthKeyCheck = 1` come in produzione Forever.
- maintainer ha prodotto una lista di opcode non gestiti (sotto) per un futuro report di copertura.
- Fine test: Aeternum fermato; conf isolate ripristinate (**2119 / 18083 / 18085**) mantenendo i fix indipendenti dalla porta; Trinity riavviato.

## Fix da conservare (indipendenti dalla porta)

| Fix | Motivo |
|---|---|
| Cert/key LoginREST `Aeternum.chain.pem` / `Aeternum.key.pem` | Il launcher usa `forever_root_ca`; il cert stock `bnetserver.cert.pem` fallisce TLS â†’ Â«Server non raggiungibileÂ» |
| `Network.SkipBuildAuthKeyCheck = 1` | Build **70790** senza righe in `build_auth_key` (come `auth` Trinity); con `=0` compare `Missing client build auth keyâ€¦` |
| `Network.EnterEncryptedModeRegionGroup = 8` | Allineato a Trinity Forever |
| `Classic.Hardcore = 0` | Evita warning di chiave conf mancante |
| `Admin.AuditKeepRotated = 3`, `Admin.MetricsDenseDefault = 0` | Completezza schema admin |

**Non committare** i `.pem`.

## Flusso Gioca (sintesi)

1. `GET /api/game/client-build` sul sito.
2. TCP indirizzo baked su **1119**; LoginREST **8083** con CA forever.
3. `POST /bnetserver/launcher/login/` â†’ ticket; portal client `:1119`.
4. World **8085** `HandleAuthSession` (controllo build key se non skippato).

## Opcode non gestiti osservati (console world Aeternum)

Analisi completa: [OPCODE_COVERAGE_REPORT.md](OPCODE_COVERAGE_REPORT.md).

Input per un futuro report di copertura opcode (elenco come riportato da maintainer):

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

Nota aggiuntiva: **arena template senza map ids**.

## Ripristino

- Conf di lavoro (porte prod): `_buildlogs/clienttest_working_20261105/`
- Baseline isolata: `_buildlogs/bak_clienttest_prodports_20261105/`
- Dopo test: realmlist Aeternum su porta **18085**; task Trinity riabilitati e produzione avviata da `<SERVER_ROOT>\server`.

## Nome realm

Il nome in eternum_auth.realmlist deve restare **Aeternum** (non Trinity). I rollback dei test su porte prod possono ripristinare address/port ma devono mantenere 
ame='Aeternum' (vedi _buildlogs/bak_clienttest_prodports_20261105/aeternum_realmlist_rollback.sql).


