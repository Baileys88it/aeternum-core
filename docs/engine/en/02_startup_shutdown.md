# Startup & shutdown

**Status:** Implemented

## worldserver (typical)

1. Load `worldserver.conf` (UTF-8 **without BOM**; BOM makes line 1 fail with `'=' character not found`).  
2. Open DB pools (Login/World/Character/Hotfix).  
3. DBUpdater may apply SQL if enabled.  
4. Load DB2/hotfixes, spawns, scripts, SmartAI, …  
5. Log line `World initialized in X minutes Y seconds` (sample: **7 seconds**).  
6. Accept sockets; MapUpdater workers activate as configured.

## bnetserver

1. Load `bnetserver.conf` (`BattlenetPort=1119`, LoginREST settings).  
2. Auth DB access; REST login service.  

## Shutdown

Graceful: stop accepting, drain sessions, deactivate MapUpdater (`MapUpdater::deactivate`), flush DB. Trinity Engine restarts are **operator-controlled** (maintainer); Aeternum docs must not assume auto-restart.

**Sources:** `worldserver/Main.cpp`, `bnetserver/Main.cpp`, `World.cpp`, `MapUpdater.*`, Server.log.

## Windows runtime helpers (build output)

On Windows, CMake POST_BUILD copies helper scripts into the per-config runtime directory (for example `build\bin\RelWithDebInfo`):

| File | Role |
|---|---|
| `START_SERVER.bat` | TCP-check MySQL 3308, start bnet then world, wait for ports from conf |
| `STOP_SERVER.bat` | Graceful stop (120s IT+EN countdown) of processes whose path is under that folder only |
| `STATUS.bat` | Ports / processes; optional health if `AETERNUM_HEALTH_TOKEN` is set |
| `CHECK_CONFIG.bat` | `worldserver.exe --check-config` |
| `README.txt` | English install + config + troubleshooting guide for that folder |

Templates live in `contrib/aeternum/scripts/*.bat.in` (+ `README.txt.in`). There is **no** separate `server\` folder — helpers belong in the build runtime dir. Prefer `LogsDir = "logs"` under that same folder.
