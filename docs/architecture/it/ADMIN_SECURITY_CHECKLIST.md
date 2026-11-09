# Admin & Monitoring — checklist di revisione sicurezza (M5 v0)

**EN:** [`../en/ADMIN_SECURITY_CHECKLIST.md`](../en/ADMIN_SECURITY_CHECKLIST.md)

Usare prima di impostare `Admin.Enable=1` su qualsiasi macchina.

## Bind e rete

- [ ] `Admin.BindIP` è `127.0.0.1` o `::1` (l’avvio rifiuta altro).
- [ ] `Admin.Port` libero in locale; non inoltrato sul router.
- [ ] Router espone solo 1119 / 8083 / 8085 — mai la porta admin.
- [ ] Il processo ascolta solo su loopback.

## Autenticazione

- [ ] `Admin.Token` vuoto in git; token da env `AETERNUM_ADMIN_TOKEN` (o conf locale fuori git).
- [ ] Entropia token adeguata (≥ 24 byte random o equivalente).
- [ ] Nei log il token **non** compare.
- [ ] `GET /metrics` senza token → 401.
- [ ] Token errato → 401.

## Autorizzazione (RBAC)

- [ ] Token v0 solo lettura (`metrics.read`, `admin.read`).
- [ ] Niente metodi mutanti (non-GET → 405).
- [ ] Nessun endpoint SQL/shell/GM/restart in v0.

## Audit

- [ ] Audit log scrivibile; solo method/path/status/role.
- [ ] Auth fallita senza segreti nel log.

## Operativo

- [ ] Default `Admin.Enable=0` / `Admin.Port=0` su Trinity produzione salvo richiesta esplicita di maintainer.
- [ ] Modulo non critico: fallimento admin non ferma il worldserver.
- [ ] Launcher / sito / porte bnet invariati.

## Firma

| Check | Chi | Data (Europe/Rome) |
|---|---|---|
| Pre-soak | | |
| Post-soak | | |

## Token Admin API (aggiornamento 2026-11-05)

- Precedenza: variabile d'ambiente (`Admin.TokenFromEnv`, default `AETERNUM_ADMIN_TOKEN`) **prima** di `Admin.Token` in conf.
- Se `Admin.Token` / `Admin.OperatorToken` in conf non sono vuoti: WARN a startup **senza** stampare il valore (fail-fast previsto in versione successiva).
- `Admin.TokenMinLength` default **32**.
- Token operatore **opzionale**.
- Wizard: `Manage-AeternumAdminToken.ps1` (Generate/Rotate/Recover). Mai loggare il valore.
- Porta 9300 solo `127.0.0.1`.
