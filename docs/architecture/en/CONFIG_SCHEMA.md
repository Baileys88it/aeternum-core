# Config schema & --check-config (M9)

## Behaviour
- Required keys (default: `RealmID`, `DataDir`, `LoginDatabaseInfo`, `WorldDatabaseInfo`, `CharacterDatabaseInfo`) → **FAIL** if missing/empty.
- Unknown keys under Aeternum prefixes (`Admin.`, `Metric.`, `Crash.`, `Config.Schema.`, …) → **WARN**.
- `Config.Schema.WarnUnknownAll=1` scans all keys (noisy on stock Trinity conf).
- `Config.Schema.FailFast=1` (default) aborts worldserver startup on required failures.

## Dry-run
```
worldserver.exe --check-config -c path/to/worldserver.conf
```
Exit `0` = OK, `1` = required failures. Does **not** open databases or write to them.
