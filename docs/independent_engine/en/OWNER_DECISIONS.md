# OWNER_DECISIONS — Independent Engine track (IE-M0 → IE-M1)

**Date:** 2026-11-05 (Europe/Rome)  
**Owner:** project owner answers recorded; Tech Lead choices applied where owner delegated.  
**Stance:** stability > correctness > compatibility > performance > simplicity. Small, reversible, measured steps. No big-bang rewrite.

---

## English

### Decision record

| # | Topic | Owner answer | Tech Lead applied choice | Status |
|---|---|---|---|---|
| 1 | License / independence | "choose best" | **Incremental strangler extraction behind interfaces under GPL-2.0.** No big-bang rewrite. Clean-room replacement only per module where a **measured** gain exists, with a golden packet corpus + replay proving behavior parity. *(Not legal advice.)* | ACCEPTED |
| 2 | IE-M1..IE-M9 track | Yes | Approve as **separate** from completed operational M0–M9. | ACCEPTED |
| 3 | IE-ADR-001..012 | "choose best" | **Accept all twelve** as written. Amendments only as written change records during implementation. | ACCEPTED |
| 4 | SDK | "most efficient" | **Freeze a small SDK 1.0** surface in `aeternum/api/` covering admin, metrics, config, module lifecycle (+ protocol boundary). Existing `src/modules/sdk` draft remains draft until host is Trinity-free. | ACCEPTED |
| 5 | Extraction order | "best for our project" | **Protocol → Operations → Module → Kernel → World/Gameplay.** Lowest risk / highest Forever leverage first. | ACCEPTED |
| 6 | Contracts location | "choose" | **Only contracts with a real current consumer** enter the repo under `include/aeternum/api/`. No TrinityCore types (`Player*`, `WorldSession*`, `Map*`, `Unit*`, `Creature*`, `WorldObject*`). Compile tests required. **No speculative APIs.** | ACCEPTED |
| 7 | Trinity production | Yes (details later OK) | Trinity stays production until **soak end (2026-11-08 10:54 Europe/Rome) + 14 days with players + explicit owner approval**. | ACCEPTED |
| 8 | Gameplay modules | Must develop; choose best | Develop **after World extraction**, on the new APIs — not before. | ACCEPTED |
| 9 | Opcode `0x3D013D` | "choose best" | **Sniff + remap allowed on Aeternum only** (test client). Never on Trinity production. | ACCEPTED |
| 10 | README Admin/Prometheus | Yes | **Docs-only fix now** (EN+IT). | ACCEPTED |

### Senior constraints applied (2026-11-05)

1. **No rewrite now.** Extract behind interfaces; replace a piece only when parity is proven.
2. **No speculative public API.** Headers land only when a real in-tree consumer already exists (admin_monitoring, metric local sink, config schema, module host, Forever opcode remap / profile).
3. **Soak is sacred.** Do not stop/restart/overwrite RelWithDebInfo binaries used by the running soak; build into a separate tree.
4. **Measure.** Protocol work ships with a golden corpus + replay harness design from day one.

### Track status

- **IE-M0:** accepted by owner 2026-11-05 (Europe/Rome).
- **IE-M1:** authorized to start (Protocol boundary + small `aeternum/api` + README honesty + goldens harness skeleton).

---

## Italiano

### Registro decisioni

| # | Tema | Risposta owner | Scelta Tech Lead | Stato |
|---|---|---|---|---|
| 1 | Licenza / indipendenza | "scegli tu" | **Estrazione strangler incrementale dietro interfacce sotto GPL-2.0.** Niente riscrittura big-bang. Clean-room solo per modulo con guadagno misurato + corpus pacchetti golden + replay. *(Non è parere legale.)* | ACCETTATA |
| 2 | Traccia IE-M1..IE-M9 | Sì | Approvata, **separata** da M0–M9 operativi già chiusi. | ACCETTATA |
| 3 | IE-ADR-001..012 | "scegli tu" | **Accettate tutte e 12.** Modifiche solo con record scritto. | ACCETTATA |
| 4 | SDK | "più efficiente" | **Congela SDK 1.0 ridotta** in `aeternum/api/` (admin, metriche, config, lifecycle moduli + protocollo). Il draft in `src/modules/sdk` resta draft finché l'host non è Trinity-free. | ACCETTATA |
| 5 | Ordine estrazione | "migliore per il progetto" | **Protocol → Operations → Module → Kernel → World/Gameplay.** | ACCETTATA |
| 6 | Contratti | "scegli tu" | **Solo contratti con consumatore reale** in `include/aeternum/api/`. Niente tipi TrinityCore. Test di compilazione. **Niente API speculative.** | ACCETTATA |
| 7 | Trinity produzione | Sì | Trinity resta produzione fino a **fine soak (2026-11-08 10:54 Europe/Rome) + 14 giorni con giocatori + approvazione esplicita**. | ACCETTATA |
| 8 | Moduli gameplay | Da sviluppare; scegli | Sviluppo **dopo** estrazione World, sulle nuove API. | ACCETTATA |
| 9 | Opcode `0x3D013D` | "scegli tu" | **Sniff + remap solo su Aeternum** (client di test). Mai su Trinity produzione. | ACCETTATA |
| 10 | README Admin/Prometheus | Sì | **Fix solo docs ora** (EN+IT). | ACCETTATA |

### Vincoli senior (2026-11-05)

1. **Niente rewrite ora.** Estrarre dietro interfacce; sostituire solo con parità dimostrata.
2. **Niente API pubbliche speculative.** Header solo con consumatore reale in-tree.
3. **Soak intoccabile.** Non fermare/sovrascrivere i binari RelWithDebInfo del soak; build in albero separato.
4. **Misurare.** Il protocollo nasce con corpus golden + harness di replay.

### Stato traccia

- **IE-M0:** accettato dall'owner il 2026-11-05 (Europe/Rome).
- **IE-M1:** autorizzato (boundary protocollo + `aeternum/api` ridotta + README + scheletro goldens).
