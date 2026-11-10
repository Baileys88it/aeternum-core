# M2 Report — Engineering Foundations

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** eternum/main  
**IT:** [../it/M2_REPORT.md](../it/M2_REPORT.md)

## Decisions recorded (ADR-001..009 → Accepted)

1. Profiles: orever/client_profiles/1.60.1.70790/ (single active; C++ adapters later in src/server/game/Forever/).
2. Admin HTTP: **Boost.Beast**, localhost, token/RBAC/audit — **not in M2**.
3. Soak: **72h** clean + **14 days** players; cutover only with maintainer OK.
4. Metrics: existing Metric → local file/log first; full catalog + **~75 MB** disk budget + human summary + later Prometheus via admin; **no metrics code in M2** (M3 enable/sink/summary; M5–M8 dense catalog + /metrics).

## Delivered

| Item | Path |
|---|---|
| Profile data | orever/client_profiles/1.60.1.70790/ |
| ExtraInserts validator + unit tests | 	ools/config-validator/ |
| Catch2 remap skeleton (70790 regression) | 	ests/game/ClassicExtraInsertsRemap.cpp |
| Quality checklist EN+IT | docs/development/en|it/QUALITY_CHECKLIST.md |
| clang-tidy / cppcheck proposals | .clang-tidy, .cppcheck, .cppcheck-suppressions |
| Observability rewrite EN+IT | docs/architecture/en|it/OBSERVABILITY_AND_PERFORMANCE.md |

## C++ startup ExtraInserts check

**Deferred to M4.** M2 ships offline Python validator + Catch2 contract tests to avoid a second policy reader beside ClassicOpcodes.cpp static conf singletons / zero behavior change.

## Build verification (real)

| | |
|---|---|
| Build dir | <ENGINE_ROOT>\build (Trinity <SERVER_ROOT> **untouched**) |
| Generator | Visual Studio 17 2022 x64 |
| Config | RelWithDebInfo, SERVERS=1, TOOLS=0, BUILD_TESTING=1 |
| Deps reused | Boost 1.84, OpenSSL-Win64, MySQL 8.0.46 (same roots as Trinity) |
| worldserver.exe | Built uild/bin/RelWithDebInfo/worldserver.exe |
| 	ests.exe | Built uild/bin/RelWithDebInfo/tests.exe |
| Catch2 [classic][extrainsects] | **2 cases / 4 assertions — PASSED** |
| Full 	ests.exe | **74 cases / 1674 assertions — PASSED** |
| Python validator tests | **4/4 PASSED** |
| MSBuild note | Post-build catch_discover_tests failed (tests.exe DLL PATH during discover); binaries OK; tests run manually with MySQL/OpenSSL on PATH |

## STOP

M2 complete. **Do not start M3** without maintainer approval.
