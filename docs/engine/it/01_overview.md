# Panoramica e topologia dei processi

**Stato:** Implementato

```text
[Client Forever 70790] + [Launcher / Project ItaPatch / Everforge]
        │
        ├─(LoginREST / bnet)──► bnetserver :1119
        │                         ▼
        │                      MySQL auth (build_info, account, realmlist)
        │
        └─(TCP gioco)─────────► worldserver :8085
                                  ├─ auth / characters / world / hotfixes (MySQL 3308 su Trinity Engine)
                                  └─ sito https://<your-site>
```

**Sorgenti:** `bnetserver/Main.cpp`, `worldserver/Main.cpp`, script installers Aeternum.  
Aeternum deve mantenere questi contratti esterni finché launcher e server non cambiano insieme.
