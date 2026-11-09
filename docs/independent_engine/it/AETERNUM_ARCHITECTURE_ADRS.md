# AETERNUM_ARCHITECTURE_ADRS (traccia Independent Engine)

> Versione italiana. EN: [`../en/AETERNUM_ARCHITECTURE_ADRS.md`](../en/AETERNUM_ARCHITECTURE_ADRS.md)

**Stato:** IE-ADR-001..012 **ACCEPTED** dall’owner il 2026-11-05 (vedi `OWNER_DECISIONS.md` / `docs/adr/IE-ADR-STATUS.md`).  
**Data:** 2026-11-05 (Europe/Rome)  
**Regola di collisione:** gli ADR operativi Accepted `docs/adr/ADR-001..009` restano storici. I nuovi usano **`IE-ADR-NNN`**.

## Mappa delle relazioni

| ADR Accepted esistente | Tema | Relazione |
|---|---|---|
| ADR-001 engine boundaries | Layering | Complementato da IE-ADR-001/002/004 |
| ADR-002 client compatibility | Singolo profilo Forever | Preservato; IE-ADR-005 specializza protocollo |
| ADR-003 module architecture | Static first | Preservato; IE-ADR-006/012 raffinano |
| ADR-004 module SDK | Direzione draft | Intent superseduto da IE-ADR-006 (emendato: freeze API 1.0 in `aeternum/api`) |
| ADR-005 threading | Ownership mappe | Preservato |
| ADR-006 database | MySQL SoR | Preservato; IE-ADR-007 aggiunge porte sopra MySQL |
| ADR-007 observability | Metric sink | Preservato |
| ADR-008 admin monitoring pilot | Modulo admin | Preservato; honesty README = decisione #10 |
| ADR-009 dual maintenance | Trinity prod | Preservato |

---

### IE-ADR-001 — Direzione Independent Engine
- **Contesto:** CMake `project(TrinityCore)`; obiettivo prodotto = engine Forever Aeternum.  
- **Problema:** Identità e forma ancora Trinity; linguaggio «indipendente» rischia over-promise.  
- **Decisione:** Perseguire la traccia Independent Engine; TrinityCore è fondazione → futuro adapter; **stato attuale non indipendente**.  
- **Alternative:** Soft-fork eterno; rewrite immediato.  
- **Conseguenze:** Milestone IE; wording cauto; GPL resta finché #1 non dice altrimenti.  
- **Esito:** **ACCEPTED**.

### IE-ADR-002 — Modular Monolith
- **Decisione:** Modular monolith a nove domini; niente microservizi di default.  
- **Esito:** **ACCEPTED**.

### IE-ADR-003 — TrinityCore Compatibility Boundary
- **Decisione:** `adapters/trinitycore` implementa i contratti Aeternum; il legacy può restare fino a sostituzione.  
- **Esito:** **ACCEPTED**.

### IE-ADR-004 — Public Engine Contracts
- **Decisione:** Vietare i tipi puntatore gameplay Trinity nei contratti/SDK pubblici; usare ID/POD/handle.  
- **Esito:** **ACCEPTED**.

### IE-ADR-005 — Protocol Independence
- **Decisione:** Protocol Engine possiede profili, opcode map, codec; il gameplay consuma messaggi decodificati.  
- **Esito:** **ACCEPTED**.

### IE-ADR-006 — Module SDK and Versioning
- **Decisione (emendata da decisione #4):** congelare una piccola API 1.0 in `aeternum/api` (admin, metrics, config, lifecycle, protocol); `src/modules/sdk` resta draft finché l’host non è Trinity-free; niente dynamic load.  
- **Esito:** **ACCEPTED** (con emendamento).

### IE-ADR-007 — Persistence Abstraction
- **Decisione:** Porte repository graduali; MySQL resta SoR (ADR-006).  
- **Esito:** **ACCEPTED**.

### IE-ADR-008 — Engine Kernel and Lifecycle
- **Decisione:** Façade `IEngineHost`; i bat restano UX.  
- **Esito:** **ACCEPTED**.

### IE-ADR-009 — Data Ownership and Content Versioning
- **Decisione:** Pacchetti contenuti versionati referenziati dai profili; validator nei tool.  
- **Esito:** **ACCEPTED**.

### IE-ADR-010 — Architecture Dependency Enforcement
- **Decisione:** Check local_ci su `include/aeternum/api` + `modules/sdk/include`; debito host/admin fino a M3.  
- **Esito:** **ACCEPTED**.

### IE-ADR-011 — Testing and Regression Strategy
- **Decisione:** Piramide unit + protocol goldens + checklist launcher periodica + metriche soak; Actions restano off.  
- **Esito:** **ACCEPTED**.

### IE-ADR-012 — Runtime Module Strategy
- **Decisione:** Solo static almeno fino a IE-M3; dynamic solo con piano security+ABI.  
- **Esito:** **ACCEPTED**.

## Accettazione degli ADR
Owner decision #3 del 2026-11-05: **tutti accettati**. Emendamenti futuri solo con nota datata.
