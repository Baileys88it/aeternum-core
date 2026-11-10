# Report M4 — Foundation profilo client e isolamento aggiornamenti

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** eternum/main  
**EN:** [../en/M4_REPORT.md](../en/M4_REPORT.md)

## Consegnato

| Voce | Dove |
|---|---|
| Puntatore ACTIVE | orever/client_profiles/ACTIVE → 1.60.1.70790 |
| Remap ExtraInserts puro | Forever/ClassicOpcodeRemap.h |
| Helper profilo | Forever/ClientProfile.* + log seam da World::LoadConfigSettings |
| Doc seam opcode/hotfix | .../seams/ |
| Gate must-not-break | Catch2 [classic][profile][active][gate] |
| Chiave conf | Classic.ActiveClientProfile |
| Bozza Module SDK | src/modules/sdk/include/aeternum/sdk/ — **non congelata** |
| Doc architettura | EN+IT come sopra |

## Rimandato

- Auto-apply ExtraInserts dal file profilo all’avvio
- Hot-swap / multi-profilo
- Freeze SDK 1.0

## Verifica

- Catch2 gate + remap 70790
- Soak Aeternum isolato ≥20 min, poi ripristino Trinity produzione

## Soak live (isolato)

- Fermata Trinity produzione con countdown 120s IT+EN (solo stop).
- Aeternum su copie eternum_*, porte 2119 / 18083 / 18085, Metric local.
- Durata: **20,6 min** (Europe/Rome). Nessun crash. DBErrors invariato (704475 byte / 6650 righe).
- world_tick_ms sub-ms nel report (avg ~0,075 ms).
- Log: Forever client profile active id: 1.60.1.70790.
- Stop Aeternum; ripartenza produzione via task Aeternum-Login + Aeternum-GameServer; porte 1119/8085 ok.

