# Schema config e --check-config (M9)

## Comportamento
- Chiavi required (default: `RealmID`, `DataDir`, `LoginDatabaseInfo`, `WorldDatabaseInfo`, `CharacterDatabaseInfo`) → **FAIL** se assenti/vuote.
- Chiavi sconosciute sotto prefissi Aeternum (`Admin.`, `Metric.`, `Crash.`, `Config.Schema.`, …) → **WARN**.
- `Config.Schema.WarnUnknownAll=1` scansiona tutte le chiavi (rumoroso su conf Trinity stock).
- `Config.Schema.FailFast=1` (default) interrompe l’avvio di worldserver se falliscono le required.

## Dry-run
```
worldserver.exe --check-config -c path/to/worldserver.conf
```
Exit `0` = OK, `1` = fallimenti required. **Non** apre i database né scrive su di essi.
