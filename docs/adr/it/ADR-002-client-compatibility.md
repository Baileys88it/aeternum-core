# ADR-002: Single active Forever client profile

**Stato:** Accettato  
**Data:** 2026-11-04  
**Milestone:** M1  
**EN:** [`../ADR-002-client-compatibility.md`](../ADR-002-client-compatibility.md)


## Problema
ExtraInserts/global packet edit possono rimappare opcode critici (caso 70790).

## Soluzione
**Un solo profilo client attivo**; pacchetto con manifest, inserts, expectation, test. Cambio build = nuovo profilo.

## Perché
Decisione maintainer; “isolate what changes.”

## Alternative
Multi-profilo runtime (no); solo conf tribale (no).

## Fonti
`ClassicOpcodes.cpp`, NOTES 70790; conclusione: profilo + gate create-char.

## Complessità / Perf / Manutenzione
Media-alta in M4; perf ok; manutenzione migliora.

## Test
Remap unitari; validazione conf; smoke creazione personaggio.

## Rischi & rollback
Ripristino profilo precedente + SQL rollback.

## Decisione (Accettato M2 — 2026-11-05)

**Percorso:** `forever/client_profiles/1.60.1.70790/`. Un solo profilo attivo. Manifest + ExtraInserts + tabella opcode + note auth/pacchetti + test. Loader runtime in **M4**; M2 = solo dati + validator + scheletro Catch2.
