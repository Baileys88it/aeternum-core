# Overview & process topology

**Status:** Implemented

```text
[Forever Client 70790] + [Launcher / Project ItaPatch / Everforge]
        │
        ├─(LoginREST / bnet)──► bnetserver :1119
        │                         │
        │                         ▼
        │                      MySQL auth (build_info, accounts, realmlist)
        │
        └─(game TCP)──────────► worldserver :8085
                                  │
                                  ├─ auth / characters / world / hotfixes (MySQL 3308 on Trinity Engine)
                                  └─ optional site https://<your-site>
```

**Sources:** `src/server/bnetserver/Main.cpp`, `src/server/worldserver/Main.cpp`, `(operator files, not included)`, `build_launcher_everforge.ps1`.

Aeternum must keep this external topology unless launcher+server change together.
