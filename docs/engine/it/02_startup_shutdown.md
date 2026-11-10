# Avvio e spegnimento

**Stato:** Implementato

## worldserver

1. Carica `worldserver.conf` (UTF-8 **senza BOM**).  
2. Apre i pool DB.  
3. DBUpdater può applicare SQL.  
4. Carica dati/script.  
5. Log `World initialized in …` (es. 7 secondi).  
6. Accetta socket; attiva worker MapUpdater.

## bnetserver

Porta **1119**, LoginREST, DB auth.

## Spegnimento

Drena sessioni, `MapUpdater::deactivate`, flush DB. I riavvii di Trinity Engine li decide **maintainer**.

**Sorgenti:** Main.cpp, `World.cpp`, `MapUpdater.*`, Server.log.

## Helper Windows (cartella di build)

Su Windows, CMake in POST_BUILD copia gli script di avvio nella directory runtime per-config (es. `build\bin\RelWithDebInfo`):

| File | Ruolo |
|---|---|
| `START_SERVER.bat` | Check TCP MySQL 3308, avvia bnet poi world, attende le porte dal conf |
| `STOP_SERVER.bat` | Stop graduale (countdown 120s IT+EN) solo dei processi il cui path e sotto quella cartella |
| `STATUS.bat` | Porte / processi; health opzionale se e impostato `AETERNUM_HEALTH_TOKEN` |
| `CHECK_CONFIG.bat` | `worldserver.exe --check-config` |
| `README.txt` | Guida EN a installazione, config e troubleshooting in quella cartella |

I template sono in `contrib/aeternum/scripts/*.bat.in` (+ `README.txt.in`). **Nessuna** cartella `server\` separata: gli helper stanno nella runtime di build. Preferire `LogsDir = "logs"` sotto la stessa cartella.
