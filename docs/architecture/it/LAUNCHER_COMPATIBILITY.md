# Contratto compatibilità launcher (M1)

**Stato:** Proposta / vincolante operativamente  
**EN:** [../en/LAUNCHER_COMPATIBILITY.md](../en/LAUNCHER_COMPATIBILITY.md)

## Contratti congelati (non cambiare senza rilascio congiunto launcher+server)

| Contratto | Valore | Evidence |
|---|---|---|
| Battlenet | **1119** | BattlenetPort; setup_elevated.ps1 |
| World | **8085** | script firewall/setup |
| LoginREST | ticket/login come da conf | bnetserver REST |
| Client build | build attivo (70790 oggi) in uild_info + gamebuild | audit SQL M0 |
| Realmlist | launcher legge address da uth.realmlist | uild_launcher_everforge.ps1 |
| Portal/hosts | redirect of the client login host | setup elevated |
| Sito | https://<your-site> | FOREVER_WEBSITE |
| Launcher | Project ItaPatch / Everforge / third-party | <LAUNCHER_ROOT> |

Internamente si possono cambiare remap/SQL/moduli **se** i contratti esterni reggono. Gate: cold launch con launcher attuale quando Aeternum sarà eseguibile.

