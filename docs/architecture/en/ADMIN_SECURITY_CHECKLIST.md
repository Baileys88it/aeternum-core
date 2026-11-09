# Admin & Monitoring — security review checklist (M5 v0)

**Italian:** [`../it/ADMIN_SECURITY_CHECKLIST.md`](../it/ADMIN_SECURITY_CHECKLIST.md)

Use before enabling `Admin.Enable=1` on any machine.

## Bind & network

- [ ] `Admin.BindIP` is `127.0.0.1` or `::1` (startup refuses anything else).
- [ ] `Admin.Port` is unused locally; not forwarded on the router.
- [ ] Router still only exposes 1119 / 8083 / 8085 — never the admin port.
- [ ] Confirmed process listens only on loopback (`netstat` / `Get-NetTCPConnection`).

## Authentication

- [ ] `Admin.Token` empty in git; token supplied via env `AETERNUM_ADMIN_TOKEN` (or local conf outside git).
- [ ] Token length ≥ 24 random bytes (or equivalent entropy).
- [ ] Grepped logs: token string does **not** appear in `Server.log` / audit log.
- [ ] Unauthenticated `GET /metrics` returns 401.
- [ ] Wrong token returns 401.

## Authorization (RBAC)

- [ ] v0 token is read-only (`metrics.read`, `admin.read` only).
- [ ] No write/mutating HTTP methods accepted (non-GET → 405).
- [ ] No SQL, shell, GM-command, or restart endpoints exist in v0.

## Audit

- [ ] `Admin.AuditLogFile` writable; entries show method/path/status/role only.
- [ ] Failed auth produces audit lines without secrets.

## Operational

- [ ] Default `Admin.Enable=0` / `Admin.Port=0` on production Trinity unless maintainer explicitly asks.
- [ ] Module `critical:false` — failure to start admin must not stop worldserver.
- [ ] Launcher / site / bnet ports unaffected after enable.

## Sign-off

| Check | Who | Date (Europe/Rome) |
|---|---|---|
| Pre-soak | | |
| Post-soak | | |

## Admin API token (update 2026-11-05)

- Precedence: environment variable (`Admin.TokenFromEnv`, default `AETERNUM_ADMIN_TOKEN`) **before** plaintext `Admin.Token` in conf.
- If `Admin.Token` / `Admin.OperatorToken` in conf is non-empty: WARN at startup **without** the value (fail-fast planned later).
- Default `Admin.TokenMinLength` = **32**.
- Operator token is **optional**.
- Wizard: `Manage-AeternumAdminToken.ps1` (Generate/Rotate/Recover). Never log the value.
- Port 9300 remains `127.0.0.1` only.
