# Profilo client e isolamento aggiornamenti (foundation M4)

**Stato:** Foundation (M4) + auto-apply ExtraInserts (M6)  
**EN:** [`../en/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md`](../en/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md)  
**Policy:** Esattamente un profilo Forever attivo per deployment (ADR-002).

## Cosa formalizza M4

1. **Pacchetto profilo versionato** in `forever/client_profiles/<product>.<build>/` con puntatore `forever/client_profiles/ACTIVE`.
2. **Foundation C++** in `src/server/game/Forever/`:
   - `ClassicOpcodeRemap.h` — remap ExtraInserts puro
   - `ClientProfile.h/.cpp` — id attivo da conf, load ExtraInserts, log seam
3. **Seam documentati** (opcode + hotfix) in `seams/` di ogni profilo.
4. **Gate must-not-break:** Catch2 `[classic][profile][active][gate]`.
5. Chiave conf `Classic.ActiveClientProfile`.

## Fuori da M4

(M6 ha aggiunto l'auto-apply ExtraInserts all'avvio; resta fuori lo hot-swap senza restart.)

## Fuori da M4 / residuali

- Hot-swap senza restart
- Multi-profilo a runtime
- Sync automatico completo conf←profilo a ogni avvio
- Module SDK congelato (solo bozza — vedi MODULE_SDK_SURFACE.md)

## Workflow di flip (con restart)

1. Aggiungere `forever/client_profiles/<nuovo>/`.
2. Estendere Catch2 / tenere verde il gate ACTIVE.
3. Smoke creazione personaggio su client reale (obbligatorio).
4. Allineare `ACTIVE` + `Classic.ActiveClientProfile` + ExtraInserts + auth.
5. Restart worldserver (120s IT+EN su Trinity di produzione quando applicabile).
