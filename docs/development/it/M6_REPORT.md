# Report M6 — Gestione dati e aggiornamenti (auto-apply ExtraInserts profilo)

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**EN:** [`../en/M6_REPORT.md`](../en/M6_REPORT.md)

## Mapping

MASTER/MIGRATION **M6 = Data & update management**. Nella nostra sequenza M4 = profilo client, M5 = Admin (anticipato). Questo M6 completa l’auto-apply ExtraInserts dal pacchetto profilo (priorità client-update).

## Consegnato

Auto-apply + `ProfileDataRoot` / `AutoApply`, check ACTIVE/manifest, ClassicOpcodes su insert effettivi, Catch2 `[m6][data]`, docs EN+IT, drill backup/restore su copia.

## Soak / produzione

Countdown 120s IT+EN evidenziato; soak ≥20 min; prod **bnet 20192 / world 14980**.
