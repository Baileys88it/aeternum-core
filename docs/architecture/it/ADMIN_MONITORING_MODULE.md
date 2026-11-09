# Modulo Admin & Monitoring (M9)

Admin HTTP in sola lettura su loopback (Boost.Beast), consumer Module SDK.

## Endpoint
| Metodo | Path | Auth | Note |
|--------|------|------|------|
| GET | `/health` | anon | Esteso: ping DB, conteggio script, lista moduli |
| GET | `/modules/lifecycle` | token readonly | Snapshot FSM Module Manager (`ListModuleLifecycle`): id/critical/state/reason |
| GET | `/uptime` `/sessions` `/metrics` `/metrics/dense` `/metrics/store` `/report/*` | token readonly | Come M8 |
| POST | `/admin/drain?delay=N` | token **operator** | **Disabilitato di default** (`Admin.Drain.Enable=0`). Ritardo min 120s. Announce IT+EN + `ShutdownServ`. Nessuna mutazione di mondo. |

## Token
- `Admin.Token` / `Admin.TokenFromEnv` → ReadOnly
- `Admin.OperatorToken` / `Admin.OperatorTokenFromEnv` → Operator (drain). Deve essere distinto.
- Compare constant-time, lunghezza minima, rate-limit 401, rotazione via conf reload (M8).

## Metric store
Persiste in `Metric.Local.Directory` (vedi METRIC_CATALOG). `/metrics/store` riporta path assoluto + file_count.

## Correlati
- Budget crash dump: `Crash.Directory` / `Crash.BudgetMB` / `Crash.RetentionDays` (hook di test Catch2; Wheaty scrive i dump reali).
- Schema config: `worldserver --check-config` (exit 0/1, niente DB). FAIL-fast all’avvio se mancano chiavi required.

> Token management (2026-11-05): env-first via `Admin.TokenFromEnv` (fallback `AETERNUM_ADMIN_TOKEN`), min length 32, conf-token WARN, wizard `Manage-AeternumAdminToken.ps1`. See ADMIN_SECURITY_CHECKLIST.
