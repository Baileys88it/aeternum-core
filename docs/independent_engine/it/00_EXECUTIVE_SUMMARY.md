# Aeternum M0 — Independent Engine Assessment: Riepilogo esecutivo

**Data:** 2026-11-05 (Europe/Rome) — pass di approfondimento + incorporazione QA  
**Classificazione:** Architecture Discovery — **IE-M0 accepted by owner 2026-11-05**  
**Repo:** `<repo-root>` — `aeternum/main`  
**Delta Aeternum:** da baseline Mobius/Forever = layer additivo documentato nel discovery report  
**Output:** originariamente `docs/independent_engine/`; ora anche in-repo  
**Input QA:** una review QA interna (non pubblicata). Le affermazioni tecniche sono state allineate (opcode 1032/380/626).

**Regola di wording (QA + Tech Lead):** «Motore indipendente» è l’**obiettivo**, non lo stato attuale. Stato attuale = server Forever derivato da TrinityCore/Mobius con layer additivo Aeternum. L’indipendenza di licenza GPL **non** è implicata dagli adapter e dipende dalla **decisione owner #1** (non è parere legale).

## Stato traccia

- **IE-M0:** accettato dall’owner il **2026-11-05** (Europe/Rome). Vedi `OWNER_DECISIONS.md`.
- **IE-M1:** autorizzato (boundary Protocollo + `aeternum/api` 1.0 + harness goldens).
- Decisioni owner #1–#10: registrate; dove delegato, Tech Lead ha applicato vincoli senior (niente rewrite big-bang; niente API speculative).

---

## Baseline opcode verificata (reconteggio 2026-11-05)

Reconteggio indipendente di `Opcodes.cpp` `DEFINE_HANDLER(CMSG_…)` (CONFERMATO, allineato al QA):

| Metrica | Valore verificato | Doc/JSON precedenti |
|---|---:|---|
| Righe `DEFINE_HANDLER(CMSG_)` | **1032** | 1031 (off-by-one) |
| `STATUS_UNHANDLED` | **380** | 379 (off-by-one) |
| Righe status attivi (LOGGEDIN/AUTHED/TRANSFER/RECENTLY) | **628** | — |
| Di cui `Handle_NULL` | **2** | 2 |
| **Active/wired** (status attivi − NULL) | **626** | 626 (corretto) |
| `STATUS_IGNORED` / `STATUS_NEVER` | 16 / 8 | uguale |
| Nomi `CMSG_` unici in `ClassicClientOpcodes.inc` | **686** | 686 |
| Path JSON evidenza | **`<buildlogs>/opcode_coverage_70790.json`** | trovato; numeri interni stale |

**`0x3D013D`:** assente da `Opcodes.h` / `ClassicClientOpcodes.inc` = CONFERMATO; sighting live solo via console owner / coverage docs = **PARZIALMENTE CONFERMATO** (non ri-sniffato in questa fase).

**Catch2:** i gate **esistono** (CONFERMATO). Il verde corrente è stato **ri-verificato** in IE-M1 su `build_iem1` (vedi report di commit). Ultima evidenza archiviata pre-IE-M1: `<archive>/m8_catch2_full.log` — «All tests passed (**1841** assertions in **110** test cases)».

---

## 1. Maturità architetturale attuale

Aeternum **non** è ancora un motore indipendente. È uno stack Forever capace, derivato da TrinityCore, con engineering additivo reale:

| Layer | Stato |
|---|---|
| Game/world/combat | Eredità Trinity; login+combat client verificati in ops |
| Forever protocol seams | Profilo ExtraInserts auto-apply + helper di remap funzionanti |
| Metric local sink | Implementato v0 |
| Admin HTTP + Prometheus | Modulo implementato; default OFF + loopback — README corretto in IE-M1 |
| Module SDK | Draft v0.2; host/admin includono ancora header Trinity |
| Contratti pubblici engine | `include/aeternum/api` 1.0 introdotto in IE-M1 |
| Percorso GPL-free / clean-room | Assente come big-bang — decisione #1: strangler sotto GPL |

La roadmap operativa M0–M9 è **completa**. Questa traccia IE è separata.

## 2. Principali dipendenze TrinityCore

CRITICAL: Player/Unit/Creature/WorldObject/Map, WorldSession/Opcodes, World/MapUpdater, DatabaseWorkerPool, ScriptMgr.  
HIGH: Spells/Combat/Handlers/ObjectMgr/DB2.  
MEDIUM: ClientProfile, Metric, Admin, SDK host.  
LOW: file profilo, ClassicOpcodeRemap.h, config-validator, bat contrib.  
GPL-2.0 (`COPYING`): adapter in-process ⇏ indipendenza di licenza.

## 3. Candidati di estrazione più preziosi

1. Forever Protocol (profili, ExtraInserts, remap, poi codec)  
2. Operations (porte Metric + Admin)  
3. Module SDK superficie pubblica Trinity-free  
4. Façade Kernel lifecycle  
5. Repository di persistenza (più tardi)

## 4. Architettura target raccomandata

Modular monolith; 9 domini; Trinity come `adapters/trinitycore`; API pubbliche senza i tipi puntatore elencati; Forever-only profilo attivo singolo; niente microservizi di default; conservare tutto il lavoro Aeternum.

## 5. Rischi tecnici principali

R1 messaging GPL; R2 breakage Forever; R3 drift dual-tree; R4 SDK draft; R5 gap opcode (380 UNHANDLED + wire sconosciuto); R6 onestà README (chiuso in IE-M1); R7 soak incompleto; più gap QA (inventario licenze dep, rischio cert/LoginREST, corpus golden — avviato in IE-M1).

## 6. Scope IE-M1 raccomandato (in corso)

Contratti + esiti IE-ADR + fix README + policy forbidden-include; nienterefresh gameplay; niente cutover. Vedi piano M1.

## 7. Decisioni owner

**Risolte il 2026-11-05** — vedi [`OWNER_DECISIONS.md`](OWNER_DECISIONS.md).

### Prompt storico (risposto)

1. Licenza / obiettivo indipendenza  
2. Approvare IE-M1..IE-M9 come distinta da M0–M9 operativi  
3. Accettare / rifiutare / emendare IE-ADR-001..012  
4. SDK v0.2 draft vs freeze 1.0 ridotta  
5. Ordine Protocol → Operations → Module → Kernel → World/Gameplay  
6. Contratti in-repo vs solo docs  
7. Trinity produzione fino a soak + 14g + ok esplicito  
8. Moduli gameplay solo proposta vs sviluppare dopo World  
9. Autorizzare sniff/remap `0x3D013D` solo su Aeternum  
10. Fix docs-only README Admin/Prometheus ora  

---

## Disaccordi QA e risoluzione

| # | Finding QA | Risoluzione Tech Lead |
|---|---|---|
| Q1 | UNHANDLED 380 vs 379; DEFINE_HANDLER 1032 vs 1031 | **Accettato QA.** Doc a **1032 / 380 / active-wired 626**. |
| Q2 | JSON coverage “mancante” | **Trovato** in `<buildlogs>`; numeri interni stale. |
| Q3 | `0x3D013D` live non ri-verificato | **PARZIALMENTE CONFERMATO**. |
| Q4 | Catch2 verde non verificato in M0 read-only | Ora ri-verificato in build_iem1 (IE-M1). |
| Q5 | Phase J senza pass/fail misurabili | **Accettato** — criteri aggiunti. |
| Q6 | README honesty troppo deferita | **Accettato** — fix in IE-M1. |
| Q7 | Over-promise “independent engine” | **Accettato** — wording obiettivo. |
| Q8 | IE-M0 incompleto finché decisioni aperte | **Chiuso** con OWNER_DECISIONS. |
| Q9 | Rollback M1 sottile | Espanso nel piano M1. |
| Q10 | Gap rischi (licenze, cert, goldens) | R21–R23 + harness goldens IE-M1. |

## Incompleto / non ispezionato (onesto)

Grafo include clang completo; mappa scripts/; dump serviceHash bnet; review sicurezza token launcher; sniff live `0x3D013D` (autorizzato ma non eseguito finché non schedulato); completezza hotfixes DB2.
