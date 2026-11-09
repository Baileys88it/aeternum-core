# Admin & Monitoring Module (M9)

Read-only admin HTTP on loopback (Boost.Beast), Module SDK consumer.

## Endpoints
| Method | Path | Auth | Notes |
|--------|------|------|-------|
| GET | `/health` | anon | Extended: DB ping, script count, module list |
| GET | `/modules/lifecycle` | readonly token | Module Manager FSM snapshot (`ListModuleLifecycle`): id/critical/state/reason |
| GET | `/uptime` `/sessions` `/metrics` `/metrics/dense` `/metrics/store` `/report/*` | readonly token | Unchanged from M8 |
| POST | `/admin/drain?delay=N` | **operator** token | **Disabled by default** (`Admin.Drain.Enable=0`). Min delay 120s. IT+EN announce + `ShutdownServ`. No world mutations. |

## Tokens
- `Admin.Token` / `Admin.TokenFromEnv` → ReadOnly
- `Admin.OperatorToken` / `Admin.OperatorTokenFromEnv` → Operator (drain). Must be distinct.
- Constant-time compare, min length, 401 rate limit, conf-reload rotation (M8).

## Metric store
Persists under `Metric.Local.Directory` (see METRIC_CATALOG). `/metrics/store` reports absolute path + file_count.

## Related
- Crash dump budget: `Crash.Directory` / `Crash.BudgetMB` / `Crash.RetentionDays` (test hook only in Catch2; Wheaty still writes real dumps).
- Config schema: `worldserver --check-config` (exit 0/1, no DB). Startup FAIL-fast on missing required keys.

> Token management (2026-11-05): env-first via `Admin.TokenFromEnv` (fallback `AETERNUM_ADMIN_TOKEN`), min length 32, conf-token WARN, wizard `Manage-AeternumAdminToken.ps1`. See ADMIN_SECURITY_CHECKLIST.
