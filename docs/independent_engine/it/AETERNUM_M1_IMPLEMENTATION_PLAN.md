# Piano di implementazione AETERNUM IE-M1

> Versione italiana. EN: [`../en/AETERNUM_M1_IMPLEMENTATION_PLAN.md`](../en/AETERNUM_M1_IMPLEMENTATION_PLAN.md)

**Stato:** AUTHORIZED dall’owner il 2026-11-05 — IE-M1 in corso (Protocol-first, `aeternum/api` 1.0 ridotta)  
**Data:** 2026-11-05 (Europe/Rome)  
**Milestone:** IE-M1 Kernel & Contracts  
**Vincoli:** read/write solo dopo approvazione; non toccare mai produzione Trinity; coordinare con CPU del soak.

## Obiettivi
1. Baseline contratti Independent Engine  
2. Esiti IE-ADR accettati/rifiutati/emendati registrati  
3. Onestà README (Admin/Prometheus)  
4. Policy include vietati (+ lint opzionale)  
5. Zero cambio intenzionale di comportamento gameplay

## Non-obiettivi
Refactor gameplay; fix di massa sugli opcode; moduli dinamici; cutover; rename CMake salvo richiesta owner.

## Work item concreti

### WP0 — Decision gate (bloccante)
- **Task:** raccogliere risposte alle decisioni #1–#10 del riepilogo esecutivo  
- **File:** nessuno in repo finché non esistono risposte; registrare in `OWNER_DECISIONS.md`  
- **Test:** n/a  
- **Accept:** tutte e 10 risposte o defer esplicito con data — **FATTO 2026-11-05**

### WP1 — Onestà README / roadmap
- **File:** `README.md`, `README.it.md`  
- **Cambio:** Admin/Prometheus → Implementato (modulo, default off, loopback); distinguere M0–M9 operativi vs IE-M0–M9  
- **Accept:** claim allineati al tree — **FATTO**

### WP2 — Bozze contratti
- **In-repo:** `include/aeternum/api/{protocol,ops,config,sdk}/*.hpp` solo con consumatori reali  
- **Test:** Catch2 compile TU + gate tipi vietati  
- **Accept:** sketch presenti; tipi vietati assenti — **FATTO (superficie 1.0)**

### WP3 — Registrazione IE-ADR
- **File:** `docs/adr/IE-ADR-STATUS.md` + testo in `docs/independent_engine/`  
- **Accept:** IE-ADR-001..012 con esito — **FATTO (tutti ACCEPTED)**

### WP4 — Policy include vietati
- **File:** `docs/architecture/en/FORBIDDEN_INCLUDES.md` + `contrib/aeternum/ci/check_forbidden_includes.ps1`  
- **Accept:** script documenta debito host/admin senza far fallire il debito noto — in corso di raffinamento IE-M1

### WP5 — Snapshot backlog indipendenza
- **File:** `docs/development/en|it/IE_COUPLING_BACKLOG.md`  
- **Accept:** backlog con path evidenza — obiettivo di questo passo

### WP6 — Verifica
- Catch2 su `build_iem1`; soak non killato; Trinity invariato

## Criteri di accettazione (M1 fatto)
- [x] Decisioni loggate  
- [x] Esiti IE-ADR  
- [x] README Admin/Prometheus corretto  
- [x] Contratti senza tipi vietati  
- [ ] Policy + lint con debito host/admin riportato  
- [ ] Backlog coupling  
- [x] Catch2 non in regressione quando eseguito  
- [x] Nessun cambio Trinity produzione; soak non killato da M1  

## Rollback
Revert commit M1; cancellare cartelle additive; nessuna migrazione DB in M1.

## Nota reconteggio opcode (2026-11-05)
`DEFINE_HANDLER(CMSG_)` **1032**, `STATUS_UNHANDLED` **380**, active-wired **626**.

## Sicurezza soak (QA)
Prima di commit: confermare soak desiderato; snapshot conf senza stampare secret; `git status` intenzionale; zero write sotto produzione Trinity. Durante M1: niente Handlers/Spells/Entities/Maps/tabelle Opcodes; niente cert; niente migrazioni DB.

## Stato 2026-11-05

ProtocolSessionBridge consegnato; vedi IE_M1_STEP3_SESSION_BRIDGE.md. Flag Classic.ProtocolApiAdapter.Enable default ON.
