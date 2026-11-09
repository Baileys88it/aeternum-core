# Architettura target Independent Engine

> Versione italiana. EN: [`../en/AETERNUM_INDEPENDENT_ENGINE_ARCHITECTURE.md`](../en/AETERNUM_INDEPENDENT_ENGINE_ARCHITECTURE.md)

**Stato:** target accettato via IE-ADR-001..012  
**Forma:** modular monolith a nove domini; Trinity come adapter; API pubbliche senza tipi puntatore gameplay Trinity.

## Domini (vista)
1. Kernel (lifecycle)  
2. Protocol (profili, opcode, codec)  
3. Operations (admin, metrics)  
4. Module SDK  
5. World (view/commands)  
6. Gameplay services  
7. Persistence  
8. Content/data packaging  
9. Tooling/CI locale  

## Dipendenze permesse
- I domini pubblici dipendono solo da `aeternum/api` e tipi POD/ID.  
- Gli adapter possono includere Trinity.  
- I moduli gameplay futuri consumano solo API pubbliche.

## Forever-only
Un profilo client attivo per deployment. Gli update client sono lavoro esplicito di compatibilità, non «gratis».

## Non-goal
Microservizi di default; rewrite clean-room globale; cutover produzione senza soak + 14 giorni + ok owner.
