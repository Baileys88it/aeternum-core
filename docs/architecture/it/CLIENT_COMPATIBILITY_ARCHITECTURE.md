# Architettura compatibilità client (M1)

**Stato:** Accettato (M2) — Priorità #1 insieme al Module SDK  
**Politica:** **un solo client profile attivo** per deployment (maintainer).  
**EN:** [`../en/CLIENT_COMPATIBILITY_ARCHITECTURE.md`](../en/CLIENT_COMPATIBILITY_ARCHITECTURE.md)

## Problema

I build Forever inseriscono opcode a metà gruppo. Oggi il remap è sparso in:

- `ClassicOpcodes.cpp` + conf `Classic.ClientExtraInserts` / `ServerExtraInserts`
- tabella `Opcodes.h/.cpp` e stub empty-probe
- struct in `Server/Packets/*`
- `auth.build_info` + `realmlist.gamebuild`
- hotfixes / aspettative DB2
- NOTES in `sql/custom`

Insert sbagliati rimappano handler non correlati in modo **silenzioso**.

## Soluzione proposta — Client Profile

Path logico candidato: `forever/client_profiles/<build>/` (es. `70790/`):

- `manifest.yaml` — build, note, git rev  
- `extra_inserts.conf` — solo valori (commenti vietati sulla riga valore)  
- expectation opcode critici (wire → handler)  
- note/adapter packet  
- expectation auth + hotfix  
- `tests/` — Catch2 + checklist manuale  
- case study (es. create-char 70790)

Runtime: una sola conf `Classic.ActiveClientProfile` (Planned); due profili attivi = fail all’avvio.

## Workflow update client (passi 1–12)

1. Scoprire il nuovo build — Parziale  
2. Diff di protocollo vs profilo attivo — Parziale (tool sniff/log Planned)  
3. Confrontare opcode — **Sì** (parse log)  
4. Confrontare struct packet — Parziale  
5. Confrontare dati/DB2 — Parziale  
6. Individuare handler colpiti — Parziale  
7. Report compatibilità — **Sì**  
8. Implementare adapter **nel nuovo profilo** — Manuale + review  
9. Test protocollo automatici — **Sì**  
10. Test integrazione — Parziale  
11. Validazione client reale (login, **creazione personaggio**, who/party/arena) — Gate manuale **obbligatorio**  
12. Approvare, pubblicare profilo, flip, ritirare il vecchio — Approvazione umana  

### Gate di validazione

Conf OK (no BOM, no token spuri); unit remap CREATE/CHECK/WHO/party/arena; **smoke create-char**; launcher ancora su 1119/8085; `build_info` + `gamebuild` allineati.

## Caso studio 70755 → 70790 (2026-11-04)

- Insert rotti: `0x440063,0x440064` (da 70755/PR#271)  
- Sintomo: wire `0x440070` → `CMSG_PET_BATTLE_REPLACE_FRONT_PET`; Finish inutile  
- Fix: `0x440071,0x440072` (tra CREATE `0x440070` e CHECK `0x440073`)  
- Lezione: non copiare insert da PR senza sniff; il profilo deve possedere numeri + test  

Evidence: NOTES in `sql/custom`.
