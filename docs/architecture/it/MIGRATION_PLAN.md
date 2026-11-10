# Piano di migrazione (M1) — Dual maintenance e cutover

**Stato:** Accettato (M2)  
**EN:** [../en/MIGRATION_PLAN.md](../en/MIGRATION_PLAN.md)

## Dual maintenance

| Tree | Ruolo |
|---|---|
| Trinity Engine <SERVER_ROOT> | Produzione — niente write/restart da agenti Aeternum senza maintainer |
| Aeternum <ENGINE_ROOT>\source | Evoluzione / design / futuro runtime |

## Porting Trinity → Aeternum

Cherry-pick commit Aeternum; copiare fix SQL+rollback+NOTES; tweak conf → artefatti profilo (da M4); mai push ad advocaite; promote inverso solo con approvazione maintainer + backup.

## Criteri cutover “Aeternum stable”

1. Contratti launcher verdi (1119/8085/build)  
2. Create-char + login + who/party/arena OK  
3. Procedura flip profilo esercitata  
4. Metriche admin localhost fidate  
5. Soak concordato senza regressioni critiche vs Trinity  
6. Drill backup/restore documentato  
7. Go-ahead esplicito maintainer  

Sequenza M2–M9: foundations → measure → client profiles → SDK+admin → data/update → advanced → harden obs → release.


## Periodo di prova / soak (Accettato M2 — 2026-11-05)

Prima dell’idoneità al cutover:

1. **72 ore** di run continuo pulito (niente crash / regressioni critiche).  
2. Poi **14 giorni** di uso reale con player senza crash/regressioni.  
3. Cutover **solo** con approvazione **esplicita** di maintainer.
