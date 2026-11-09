# Validatore configurazione (ExtraInserts + profilo client)

Tool Python 3 standalone (solo stdlib). Verifica formato ExtraInserts, assenza di commenti inline sulla riga valore, e allineamento al manifest del profilo attivo.

```bash
python tools/config-validator/validate_extra_inserts.py \
  --conf path/to/worldserver.conf \
  --profile forever/client_profiles/1.60.1.70790
```

Controllo C++ a startup: **rimandato a M4** (log-only). In M2 solo tool offline + test per non toccare il comportamento del core.
