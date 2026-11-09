# IE Coupling Backlog (generato)

**Generato:** 2026-11-05 13:55 (ora macchina, Europe/Rome)
**RepoRoot:** `<repo-root>`
**Superficie pubblica:** PASS (nessun tipo/include proibito in `include/aeternum/api` / `src/modules/sdk/include`).
**Seat consentito:** `src/modules/sdk/adapters/trinity` (adapter TrinityCore per le ops SDK 1.0).

## Debito noto (host / admin)

Prima di IE-M1 debt-clear: **9** hit (`ModuleHost` 2, `AdminHttpServer` 6, `AdminMonitoringScripts` 1).

Dopo il routing attraverso le facciate `aeternum/api/ops` + adapter Trinity:

_Nessun marker di debito corrisponde — host/admin sono puliti. Gli include Trinity appartengono solo sotto `adapters/trinity`._

### Storico clearance

| Fase | DEBT host/admin | Note |
|------|-----------------|------|
| Pre-adapter (IE-M1 lint) | 9 | World/ScriptMgr/Metric/Database/GameTime diretti |
| Post SDK 1.0 facades | **0** | Accoppiamento confinato nell'adapter (6 include consentiti) |

## Criteri di clearance

- L'host usa solo `aeternum/api` + porte ops (niente `World.h` / `ScriptMgr.h`).
- Admin HTTP parla con `IRuntimeStats` / `IMetricLocalQuery` / `IProcessControl` / `IDatabasePing` via `aeternum::ops::Services()`.
- Accoppiamento Trinity confinato in `src/modules/sdk/adapters/trinity`.
- Il conteggio DEBT deve scendere; nuovi hit in host/admin sono regressioni.

## Comportamento invariato

- Endpoint admin, output Prometheus/metriche e chiavi di config (`Admin.*`, `Metric.Local.*`) invariati.
- `HandlerOf` / path di sessione Forever **non** toccati in questo step.