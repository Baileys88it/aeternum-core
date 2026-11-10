# IE Coupling Backlog (generated)

**Generated:** 2026-11-05 13:57 (machine local)
**RepoRoot:** `<repo-root>`
**Public surface:** PASS (no forbidden types/includes in `include/aeternum/api` / `src/modules/sdk/include`).
**Allowed seat:** `src/modules/sdk/adapters/trinity` (TrinityCore adapter for SDK 1.0 ops).

## Known debt (host / admin)

Before IE-M1 debt-clear: **9** hits (ModuleHost 2, AdminHttpServer 6, AdminMonitoringScripts 1).

After routing through `aeternum/api/ops` facades + Trinity adapter:

_No debt markers matched — host/admin are clean. Trinity includes belong only under adapters/trinity._

### Clearance history

| Phase | DEBT host/admin | Notes |
|-------|-----------------|-------|
| Pre-adapter (IE-M1 lint) | 9 | Direct World/ScriptMgr/Metric/Database/GameTime |
| Post SDK 1.0 facades | **0** | Coupling confined to adapter (6 allowed includes) |

## Clearance criteria

- Host uses `aeternum/api` + ops ports only (no `World.h` / `ScriptMgr.h`).
- Admin HTTP talks to `IRuntimeStats` / `IMetricLocalQuery` / `IProcessControl` / `IDatabasePing` via `aeternum::ops::Services()`.
- Trinity coupling confined to `src/modules/sdk/adapters/trinity`.
- Debt count must trend down; new hits in host/admin trees are regressions.

## Behavior unchanged

- Admin endpoints, Prometheus/metrics output, and config keys (`Admin.*`, `Metric.Local.*`) unchanged.
- Forever session path / HandlerOf not touched in this step.