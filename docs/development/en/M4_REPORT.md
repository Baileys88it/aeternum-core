# M4 Report — Client profile & update isolation foundation

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** eternum/main  
**IT:** [../it/M4_REPORT.md](../it/M4_REPORT.md)

## Delivered

| Item | Location |
|---|---|
| ACTIVE profile pointer | orever/client_profiles/ACTIVE → 1.60.1.70790 |
| Pure ExtraInserts remap | src/server/game/Forever/ClassicOpcodeRemap.h (used by ClassicOpcodes + tests) |
| Profile helpers | Forever/ClientProfile.h/.cpp + seam log from World::LoadConfigSettings |
| Opcode/hotfix seams docs | orever/client_profiles/1.60.1.70790/seams/ |
| Must-not-break gate | Catch2 [classic][profile][active][gate] |
| Conf key | Classic.ActiveClientProfile in worldserver.conf.dist |
| Module SDK draft | src/modules/sdk/include/aeternum/sdk/ (+ internal/) — **not frozen** |
| Architecture docs | CLIENT_PROFILE_AND_UPDATE_ISOLATION.md, MODULE_SDK_SURFACE.md (EN+IT) |

## Explicitly deferred

- Auto-apply ExtraInserts from profile file at startup
- Hot-swap without restart / multi-profile runtime
- SDK 1.0 freeze (Admin pilot is the consumer)

## Verification

- Catch2 [classic][profile][active] + [classic][extrainsects][70790]
- End-of-milestone isolated Aeternum soak ≥20 min on eternum_* copies, then Trinity production restarted via scheduled tasks

## Live soak (isolated)

- Stopped Trinity production with 120s IT+EN countdown (stop-only).
- Ran Aeternum on eternum_* DB copies, ports 2119 / 18083 / 18085, Metric local on.
- Duration: **20.6 min** (Europe/Rome). No crash. DBErrors size unchanged (704475 bytes / 6650 lines).
- world_tick_ms sub-ms visible in report (avg ~0.075 ms).
- Logged Forever client profile active id: 1.60.1.70790.
- Stopped Aeternum; restarted production via Aeternum-Login + Aeternum-GameServer; ports 1119/8085 listening.

