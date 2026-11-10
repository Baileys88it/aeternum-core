# Superficie Module SDK — bozza (M4)

**Stato:** Bozza — **API non congelata** (`AETERNUM_SDK_API_DRAFT`)  
**EN:** [`../en/MODULE_SDK_SURFACE.md`](../en/MODULE_SDK_SURFACE.md)  
**Codice:** `src/modules/sdk/include/aeternum/sdk/`

## Pubblico vs interno

| Percorso | Destinatari | Promessa |
|---|---|---|
| `aeternum/sdk/version.h` | Tutti | Macro versione; bozza fino a 1.0.0 |
| `aeternum/sdk/module.hpp` | Autori moduli (futuro) | Identity / lifecycle / entry in bozza |
| `aeternum/sdk/internal/host_hooks.hpp` | Solo engine | Seam host instabili |

## Relazione col sistema moduli esistente

`src/modules/` già costruisce moduli statici opzionali con `trinity_add_module` + ScriptMgr.
La bozza SDK **non** sostituisce quel percorso; documenta la futura façade (ADR-004) senza congelare gli header.

## Criteri di freeze (dopo)

- Pilot Admin & Monitoring compila solo contro header pubblici
- Policy semver pubblicata; rimozione di `AETERNUM_SDK_API_DRAFT`
- Almeno un ciclo di deprecazione su un bump minor

See also: [MODULE_SDK_GUIDE.md](MODULE_SDK_GUIDE.md)
