# Auth / chiavi build — 1.60.1.70790

| Voce | Atteso |
|---|---|
| Launcher → bnet | TCP `1119` |
| World | TCP `8085` |
| `realmlist.gamebuild` | `70790` |
| `auth.build_info` | Riga per 70790 (niente secret in repo) |

I secret restano fuori banda — mai in commit.

## Nota operativa (client-test 2026-11-05)

Per Forever **70790** non ci sono righe in uild_auth_key (né su uth Trinity né su eternum_auth). La produzione Trinity usa Network.SkipBuildAuthKeyCheck = 1. Senza questo, world rifiuta la sessione con *Missing client build auth key for build 70790*. Vedi docs/development/it/CLIENT_TEST_REPORT.md.
