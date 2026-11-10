# Configuration & logging

**Status:** Implemented

- Conf files: `worldserver.conf`, `bnetserver.conf`.  
- Loggers: `Logger.*` keys; DBErrors logger routes SQL problems.  
- Opcode logger used during 70790 incident diagnosis.  
- **Write rule:** UTF-8 **no BOM**; keep comments off the ExtraInserts value line.

**Sources:** `common/Configuration`, `common/Logging`, conf samples on Trinity Engine (secrets redacted in audits).
