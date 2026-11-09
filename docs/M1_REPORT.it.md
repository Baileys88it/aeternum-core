# Report M1 Aeternum — Architettura e ADR

**Data:** 4 novembre 2026 (Europe/Rome)  
**Stato:** Design completato — **STOP in attesa di approvazione** (M2 non avviato)  
**EN:** [`M1_REPORT.md`](M1_REPORT.md)

## Sintesi

M1 consegna l’architettura obiettivo mappata sull’albero TrinityCore Forever reale, il design a **singolo profilo client attivo** (caso studio 70790 + workflow 12 passi), moduli static-first + SDK con pilota **Administration & Monitoring**, strategie DB/threading/osservabilità, contratto launcher, dual-maintenance/cutover, e **ADR-001…009** (stato **Proposta**) in EN+IT.

Nessuna modifica a sorgenti/CMake/DB/dipendenze.

## Decisioni chiave

1. Un solo build/profilo client; switch = sostituzione profilo  
2. Trinity resta produzione fino a “Aeternum stable”  
3. Priorità #1 = update client + Module SDK  
4. Primo modulo SDK = Admin & Monitoring (localhost, RBAC)  
5. MySQL SoR; MapUpdater preservato; misurare prima  
6. Metric in-tree prima di Tracy/OTel in prod  
7. WASM/allocator/dynamic nativi rimandati con prove  
8. modern ≠ better; docs EN+IT obbligatorie  

## Approvazioni richieste

Accettare o emendare gli ADR-001…009 (in particolare 002, 004, 008, 009) e la checklist di cutover.

## Proposta M2 (solo scope — non iniziata)

Fondamenta: validazione conf ExtraInserts; skeleton test remap Catch2; checklist qualità locale; cartella profilo `70790` **solo documentazione**; aggiornare tabella milestone.  
**Fuori da M2:** loader runtime, codice SDK, server HTTP admin, Tracy/OTel, touch Trinity, refactor CMake.

## Domande aperte

Path profili; libreria HTTP admin; durata soak; destinazione Metric.

## Gate per M2

OK su M1/ADR; conferma scope M2; Actions off; niente restart Trinity; contratti launcher fermi.
