# Engine documentation (English) — How it works TODAY

**Status labels:** Implemented · Partially implemented · Planned  
**Scope:** Describes the Forever Trinity-based server as of snapshot `113226264c` / live Trinity Engine behavior.  
**Italian:** `../it/`

| Doc | Topic |
|---|---|
| [01_overview.md](01_overview.md) | Process topology |
| [02_startup_shutdown.md](02_startup_shutdown.md) | Startup & shutdown |
| [03_networking_sessions.md](03_networking_sessions.md) | Login → realm → world → opcodes / ExtraInserts |
| [04_world_loop_mapupdater.md](04_world_loop_mapupdater.md) | World loop & MapUpdater |
| [05_maps_grids.md](05_maps_grids.md) | Maps & grids |
| [06_entities.md](06_entities.md) | Player / Creature / GO lifecycle |
| [07_movement.md](07_movement.md) | Movement |
| [08_combat_spells.md](08_combat_spells.md) | Combat & spells |
| [09_quests.md](09_quests.md) | Quests |
| [10_ai.md](10_ai.md) | AI (CreatureAI / SmartAI / scripts) |
| [11_database.md](11_database.md) | DB pools, prepared, async, DBUpdater, sql/custom |
| [12_scripting_hooks.md](12_scripting_hooks.md) | ScriptMgr |
| [13_configuration_logging.md](13_configuration_logging.md) | Configuration & logging |
| [14_client_update_today.md](14_client_update_today.md) | Client update procedure **as done today** |

## Related (Aeternum)

- M2 foundations report: [../../development/en/M2_REPORT.md](../../development/en/M2_REPORT.md)
- Client profiles: orever/client_profiles/
- Observability strategy: [../../architecture/en/OBSERVABILITY_AND_PERFORMANCE.md](../../architecture/en/OBSERVABILITY_AND_PERFORMANCE.md)

- M3 measurement: [../../development/en/M3_REPORT.md](../../development/en/M3_REPORT.md) / [../../architecture/en/METRIC_CATALOG.md](../../architecture/en/METRIC_CATALOG.md)
