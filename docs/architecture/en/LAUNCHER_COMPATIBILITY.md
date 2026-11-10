# Launcher Compatibility Contract (M1)

**Status:** Proposed / Binding operational contract  
**Italian:** [`../it/LAUNCHER_COMPATIBILITY.md`](../it/LAUNCHER_COMPATIBILITY.md)

## Frozen external contracts (must never change without joint launcher+server release)

| Contract | Value / behavior | Evidence |
|---|---|---|
| Battlenet port | **1119** | `bnetserver.conf` `BattlenetPort`; `setup_elevated.ps1` |
| World port | **8085** | firewall/setup scripts; worldserver listen |
| LoginREST | Ticket/login HTTP(S) as configured (`LoginREST.*`) | bnetserver REST |
| Client build | Active Forever build (now **70790**) in `auth.build_info` + realm `gamebuild` | SQL M0 audit |
| Realmlist | Launcher reads `auth.realmlist` address fields | `build_launcher_everforge.ps1` |
| Portal / hosts | Pattern using `<login-host>` local redirect in setup | `setup_elevated.ps1` |
| Website | `https://<your-site>` | `FOREVER_WEBSITE` in launcher build script |
| Products | Project ItaPatch / Everforge / third-party launcher under `<LAUNCHER_ROOT>` | memory + scripts |

## What Aeternum may change internally

Opcode remap, packet structs, SQL content, module internals — **as long as** external contracts above hold.

## What requires coordinated change

Renaming executables, changing ports, auth protocol breaks, build advertisement format, REST paths the launcher calls.

## Test gate

Any milestone claiming compatibility: cold launch via **current launcher** against Aeternum build (when runnable) or documented dry-run checklist until runtime exists.
