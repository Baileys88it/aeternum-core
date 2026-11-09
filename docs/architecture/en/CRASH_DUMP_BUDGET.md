# Crash dump budget (M9)

WheatyExceptionReport still writes `.dmp`/`.txt` under `<exe>/Crashes` (or `Crash.Directory`).
On startup Aeternum configures `Trinity::Crash::DumpStore` and enforces:
- `Crash.BudgetMB` (default 256)
- `Crash.RetentionDays` (default 30)
Oldest files deleted first. Catch2 uses `WriteTestMarker` (safe, no crash).
