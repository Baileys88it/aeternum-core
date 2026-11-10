# Entities lifecycle (Player / Creature / GameObject)

**Status:** Implemented

- **Player:** created via character creation handlers + `playercreateinfo*`; loaded from `characters` DB; enters Map.  
- **Creature / GO:** spawned from `world` tables; updated on map ticks; AI attached.  
- Object updates / fields: `Entities/Object` (+ UpdateFields).  

**Sources:** `Entities/`, character create handlers, world spawn tables.
