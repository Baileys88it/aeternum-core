# Budget crash dump (M9)

WheatyExceptionReport continua a scrivere `.dmp`/`.txt` sotto `<exe>/Crashes` (o `Crash.Directory`).
All’avvio Aeternum configura `Trinity::Crash::DumpStore` e applica:
- `Crash.BudgetMB` (default 256)
- `Crash.RetentionDays` (default 30)
Si cancellano prima i file più vecchi. Catch2 usa `WriteTestMarker` (sicuro, senza crash).
