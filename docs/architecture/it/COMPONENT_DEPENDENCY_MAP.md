# Mappa dipendenze componenti (M1)

**Stato:** Proposta  
**EN (con diagrammi mermaid):** [../en/COMPONENT_DEPENDENCY_MAP.md](../en/COMPONENT_DEPENDENCY_MAP.md)

## Flusso attuale (evidenza)

netserver e worldserver usano i DB uth / characters / world / hotfixes.  
Nel game: WorldSocket → WorldSession → Opcodes / ClassicOpcodes → Packets/*.  
MapManager/MapUpdater eseguono i tick mappa.  
ScriptMgr riceve la registrazione da src/server/scripts.

File chiave: WorldSocket.cpp, WorldSession.*, ClassicOpcodes.*, Opcodes.*, MapUpdater.*, DatabaseWorkerPool.*, DBUpdater.*, ScriptMgr.*, src/common/Metric/*.

## Regole target

1. I moduli dipendono da SDK/ScriptMgr, non dagli interni di WorldSocket.  
2. Il profilo forever può conoscere opcode/packet; l'engine non deve spargere check di build.  
3. Il modulo admin non possiede i thread MapUpdater.  
4. Vietare cicli moduli ⇄ forever privati.

## Hotspot da ridurre (M4–M5)

ClassicOpcodes + ExtraInserts; edit ad-hoc in Packets/; Metric/comandi GM sparsi.

