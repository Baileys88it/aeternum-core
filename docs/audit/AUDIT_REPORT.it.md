# Aeternum M0 — Riepilogo esecutivo (IT)

**Data:** 4 novembre 2026 (Europe/Rome)  
**Snapshot:** `113226264c`  
**Repo:** https://github.com/<owner>/aeternum-engine (Actions **disabilitate**)  
**Rapporto completo (EN, sezioni A–K):** [`AUDIT_REPORT.md`](AUDIT_REPORT.md)

## Cosa è Aeternum

Progetto **parallelo** a **Trinity Engine** (`<SERVER_ROOT>`). Obiettivo: far evolvere il codice Forever verso un engine manutenibile e aggiornabile quando cresce, **senza** diventare un groviglio di dipendenze. Il launcher attuale deve continuare a funzionare (bnet **1119**, world **8085**, client **70790**, sito site).

## Findings principali

1. La base è il server Forever già funzionante (non una riscrittura).  
2. Il bug creazione personaggio 70790 è la prova che la compatibilità client è fragile: insert opcode `0x63/0x64` (da 70755) rimappavano CREATE su pet-battle; fix `0x71/0x72`.  
3. Oggi la logica “per build” è sparsa (conf, opcode, packet, SQL, NOTES): va isolata (priorità #1 insieme al Module SDK).  
4. Runtime Trinity (world loop, MapUpdater, DB async, ScriptMgr) va **preservato**.  
5. Test Catch2 presenti ma stretti rispetto al rischio protocollo.  
6. Prestazioni: baseline parziale (RAM/CPU processo, init 7s); tick/map in gran parte **NON MISURATI**.  
7. Tecnologie nuove (WASM, allocator custom, ClickHouse…): solo se dimostrano vantaggio.  
8. M0 = solo documentazione. **Stop in attesa di approvazione** prima di M1.

## Rischi top

- Nuovo client Forever senza profilo/test automatici.  
- Doppia manutenzione Aeternum/Trinity.  
- DBErrors residui e dati incompleti Forever.  
- Osservabilità spenta (`Metric.Enable=0`).

## Richieste di approvazione

Completezza M0; priorità client-update+SDK; regola modern≠better; contratti launcher; Actions off; nessun riavvio Trinity da questo filone senza ordine; risposte alle open question (un solo build vs multi-profilo, durata dual-maintenance, primo modulo dopo SDK).
