# Architettura database (M1)

**Stato:** Proposta  
**EN:** [../en/DATABASE_ARCHITECTURE.md](../en/DATABASE_ARCHITECTURE.md)

## Decisione

**MySQL** resta autorevole per la persistenza di gioco (uth, characters, world, hotfixes). ClickHouse rifiutato per il gameplay.

## Pilastri attuali (Implementati)

| Pezzo | Dove |
|---|---|
| Pool async | DatabaseWorkerPool |
| Update | DBUpdater + sql/updates |
| Fix Forever | sql/custom + rollback |
| Riferimento prod | MySQL 8.0.46 (Trinity Engine :3308) |

## Migrazioni

SQL additivi con rollback; backup prima di apply rischiosi; profilo client può richiedere righe uild_info/hotfix coordinate. Non inventare un secondo runner: estendere DBUpdater. Modulo admin: read-mostly, niente SQL remoto arbitrario.

