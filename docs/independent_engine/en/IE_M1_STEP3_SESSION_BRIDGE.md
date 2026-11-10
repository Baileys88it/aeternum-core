# IE-M1 Step 3 — Protocol session bridge + isolated test instance

**Date:** 2026-11-05 (Europe/Rome)  
**Commit target:** `aeternum/main`  
**Soak:** untouched (bnet 24052 / world 20024 / monitor 11960; ports 2119 / 18083 / 18085 / admin 9100)

## What shipped

1. **Session bridge** (`Forever/ProtocolSessionBridge`): process-wide `IOpcodeRegistry` / `IClientProfile` seat.  
   `ClassicOpcodes::RemoveClientExtraInserts` routes through the bridge when `Classic.ProtocolApiAdapter.Enable=1` (default **ON** — Remap/adapter parity proven by Catch2).  
   Math remains `Forever::OpcodeRemap::RemoveClientExtraInserts` with the same Effective/profile insert lists → **zero wire change**.

2. **Isolated test instance** (not soak):  
   - Dir: `<ENGINE_ROOT>\_testinst_iem1` (conf + logs; outside soak `build\bin\RelWithDebInfo`)  
   - Binaries: `build_iem1\bin\RelWithDebInfo` via `-c`  
   - Ports (127.0.0.1 only): bnet **3119**, REST **28083**, world **28085**, admin **9200**  
   - DBs (MySQL 3308, **kept**): `aeternumtest_auth`, `aeternumtest_characters`, `aeternumtest_world`, `aeternumtest_hotfixes`  
   - Credentials: documented DPAPI root for CREATE/GRANT only; runtime uses soak trinity user on test schemas. Secrets never committed.

3. **Admin smoke:** `/health` keys identical soak↔test (`status,db,modules,scripts`). Test `/uptime`, `/sessions`, `/metrics` OK with test-only token. Soak `/metrics` requires auth (token only in soak process env at start — not available to agent); gauge **names** on test match `BuildPrometheus` contract.

## Verification numbers

| Check | Result |
|-------|--------|
| Catch2 `[protocol]` | 57 assertions / 12 cases PASS |
| Catch2 full | **1926 assertions / 126 cases PASS** |
| Test startup | admin 9200 listen; log: `ProtocolApiAdapter session bridge: enable=1 profile='1.60.1.70790' client_inserts=2` |
| Test Server.log ERROR/FATAL | **0** |
| Soak ERROR sample | **0** recent |
| Soak impact during test (5s) | world ΔCPU **0.109** s, bnet ΔCPU **0.031** s; soak WS stable ~428 MB |
| RAM | before ~14.7 GB free → during test ~10.1 GB free (test world ~2.5 GB) → after stop ~12.9 GB free |
| Live client login | **not possible** on loopback alternate ports — schedule separately |

## Flag

`Classic.ProtocolApiAdapter.Enable` — default `1`. Set `0` to fall back to direct `EffectiveClientExtraInserts` path (same Remap).

## Not done

- Real client login against test or soak ports  
- Sniff / 0x3D013D  
- Trinity production trees / DBs