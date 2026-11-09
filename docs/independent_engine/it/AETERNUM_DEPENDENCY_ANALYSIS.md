# Analisi dipendenze Aeternum

> Versione italiana. EN: [`../en/AETERNUM_DEPENDENCY_ANALYSIS.md`](../en/AETERNUM_DEPENDENCY_ANALYSIS.md)

**Data:** 2026-11-05 (Europe/Rome)  
**Metodo:** campionamento include + lettura moduli Aeternum (non clang full graph).

## Criticità Trinity (CRITICAL)
Player, Unit, Creature, WorldObject, Map, WorldSession, Opcodes, World, MapUpdater, DatabaseWorkerPool, ScriptMgr.

## Alta (HIGH)
Spells/Combat/Handlers, ObjectMgr, DB2/hotfixes.

## Media (MEDIUM) — layer Aeternum ancora accoppiato
- `src/modules/sdk/src/ModuleHost.cpp` → `World.h`, `ScriptMgr.h`, `Forever/ClientProfile.h`  
- `src/modules/admin_monitoring/AdminHttpServer.cpp` → `World.h`, `DatabaseEnv.h`, `ScriptMgr.h`, `Metric.h`, `GameTime.h`

## Bassa (LOW)
File profilo, `ClassicOpcodeRemap.h` (puro), config-validator, bat runtime.

## Superficie pubblica (IE-M1)
`include/aeternum/api/**` e `src/modules/sdk/include/**` devono restare senza tipi/include vietati. Il debito sta in host/admin (vedi `IE_COUPLING_BACKLOG`).

## Licenze dipendenze (R21)
Inventario completo `dep/*` ancora aperto; GPL-2.0 del core resta vincolante per in-process adapter.

## Implicazioni estrazione
Protocol first riduce il rischio Forever; Operations sblocca honesty ops; Module clearance elimina `World.h` dall’host; World/Gameplay per ultimi.
