# Auth / build keys — 1.60.1.70790

| Item | Expected |
|---|---|
| Launcher → bnet | TCP `1119` |
| World | TCP `8085` |
| `realmlist.gamebuild` | `70790` |
| `auth.build_info` | Row present for build 70790 (do not invent secrets here) |
| Portal / site | Unchanged launcher contract |

Secrets (CDN keys, authenticators) are **out of band** — never commit.

IT: `build_auth_keys.it.md`

## Operational note (client-test 2026-11-05)

Forever **70790** has no uild_auth_key rows (neither Trinity uth nor eternum_auth). Production Trinity sets Network.SkipBuildAuthKeyCheck = 1. Without it, world rejects with *Missing client build auth key for build 70790*. See docs/development/en/CLIENT_TEST_REPORT.md.
