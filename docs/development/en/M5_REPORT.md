# M5 Report — Admin & Monitoring module v0

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**IT:** [`../it/M5_REPORT.md`](../it/M5_REPORT.md)

## Delivered

| Item | Location |
|---|---|
| Static module | `src/modules/admin_monitoring/` (Boost.Beast, no new deps) |
| Bind policy | Loopback only (`AdminBind.h`); refuse wildcard/LAN |
| Auth + RBAC | Bearer / `X-Admin-Token`; role `readonly` (`metrics.read`, `admin.read`) |
| Audit log | `Admin.AuditLogFile` — no tokens |
| Endpoints (GET) | `/health`, `/uptime`, `/sessions`, `/metrics` (Prometheus text), `/report/latest`, `/report/daily` |
| Default | `Admin.Enable=0`, `Admin.Port=0` |
| Security checklist | `docs/architecture/en/ADMIN_SECURITY_CHECKLIST.md` |
| Catch2 | `[admin][auth]`, `[admin][bind]`, `[admin][prometheus]` — 32 assertions / 4 cases PASS |

## Explicitly out of v0

Write/mutating admin actions, remote bind, multi-role tokens beyond readonly, SDK freeze.

## Build & tests

- CMake RelWithDebInfo: `worldserver`, `bnetserver`, `tests` — exit 0
- `tests.exe "[admin]"` — All tests passed (32 assertions in 4 test cases)

## Live soak (Aeternum alt ports)

| Check | Result |
|---|---|
| Prod stop | PID 11788 (world) / 24072 (bnet); 120s IT+EN countdown then graceful exit |
| Countdown broadcast evidence | `<ENGINE_ROOT>\_buildlogs\m5_countdown_evidence.log` + `stop_prod_m5.log`; `Server.log` ends with `Halting process...` after ~120s; announce IT+EN + `server shutdown 120` injected via `restart-worldserver.ps1 -SendConsolePid` (same path that delivered the working shutdown) |
| Aeternum start | world 25072 / bnet 5424; ports 18085 / 2119 / 18083; admin `127.0.0.1:9100` |
| `/health` no auth | 200 `{"status":"ok"}` |
| `/metrics` no/wrong token | 401 |
| `/metrics` + Bearer | 200 Prometheus (uptime, sessions, `world_tick_ms`, local store bytes) |
| `/uptime` `/sessions` `/report/latest` `/report/daily` | 200 (daily list empty until midnight roll) |
| POST `/health` | 405 |
| Token leak in logs | none |
| Soak duration | ≥20 min (see soak log) |
| Crash | none |

## Production restart

See Italian final report block (PIDs after scheduled tasks).

## REGOLE_PROGETTO.md

Restored/confirmed at `<SERVER_ROOT>\REGOLE_PROGETTO.md` (only copy): §4 120s IT+EN + maintainer go (milestone exception 2026-11-05) + close orphans; §7 site changelog; §9 import rule.

## Production restart (final)

| Item | Value |
|---|---|
| Soak | 21.7 min (uptime_seconds=1301 at stop); no crash |
| Admin bind | 127.0.0.1:9100 only |
| Auth reject | 401 without/wrong token; 200 with Bearer/X-Admin-Token |
| Countdown broadcast | YES — evidence _buildlogs/m5_countdown_evidence.log + stop_prod_m5.log + Server.log Halting process... after 120s |
| Prod bnet PID | 23232 (port 1119) |
| Prod world PID | 24200 (port 8085) |
| Actions | disabled (enabled:false) |
| GitHub Actions | left disabled; push to eternum/main only |

