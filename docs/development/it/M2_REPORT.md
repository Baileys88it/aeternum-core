# Report M2 — Engineering Foundations

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** eternum/main  
**EN:** [../en/M2_REPORT.md](../en/M2_REPORT.md)

## Decisioni (ADR-001..009 → Accettato)

1. Profili in orever/client_profiles/1.60.1.70790/ (uno attivo; adapter C++ in src/server/game/Forever/ più avanti).
2. Admin HTTP: **Boost.Beast**, localhost, token/RBAC/audit — **non in M2**.
3. Soak: **72h** pulito + **14 giorni** con player; cutover solo con OK maintainer.
4. Metriche: Metric esistente → file/log locale; catalogo completo + budget disco **~75 MB** + report umano; Prometheus via admin dopo; **niente codice metriche in M2** (M3 / M5–M8).

## Consegne

Profilo 70790, 	ools/config-validator/, Catch2 ClassicExtraInsertsRemap.cpp, checklist qualità EN+IT, proposte clang-tidy/cppcheck, rewrite osservabilità EN+IT.

Check C++ startup ExtraInserts: **rimandato a M4**.

## Verifica build (reale)

- Dir: <ENGINE_ROOT>\build (Trinity non toccato)
- worldserver.exe + 	ests.exe RelWithDebInfo OK
- Catch2 extrainsects: **2/2 PASSED**; suite: **74 cases / 1674 assertions PASSED**
- Validator Python: **4/4 PASSED**
- Nota: fallisce solo post-build catch_discover_tests (PATH DLL); esecuzione manuale OK

## STOP

M2 completato. **Non iniziare M3** senza approvazione maintainer.
