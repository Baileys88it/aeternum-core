# Aeternum M0 — Independent Engine Assessment: Executive Summary

**Date:** 2026-11-05 (Europe/Rome) — deepened + QA-incorporation pass  
**Classification:** Architecture Discovery — **IE-M0 accepted by owner 2026-11-05**  
**Repo:** ``<repo-root>`` — `aeternum/main` @ `9894b2e1c275eafa56547dc6b6a40e22389fb5b2`  
**Aeternum delta:** `b3c55465c8..HEAD` = 20 commits, 194 files  
**Outputs:** ``docs/independent_engine/`\` only (repo untouched)  
**QA input:** an internal QA review (not published). Outcome: **FAIL as evidence-grade complete** until gaps below closed; technical claims largely substantiated.

**Wording rule (QA + Tech Lead):** “Independent engine” is the **target**, not the current state. Current state = TrinityCore/Mobius-derived Forever server with an Aeternum additive layer. GPL license independence is **not** implied by adapters and depends on **owner decision #1** (not legal advice).

---

## Verified opcode baseline (recount 2026-11-05)

Independent recount of `Opcodes.cpp` `DEFINE_HANDLER(CMSG_…)` (CONFIRMED, matches QA):

| Metric | Verified value | Prior docs/JSON |
|---

## Track status

- **IE-M0:** accepted by owner **2026-11-05** (Europe/Rome). See `OWNER_DECISIONS.md`.
- **IE-M1:** authorized (Protocol boundary + `aeternum/api` 1.0 + goldens harness).
- Owner decisions #1–#10: recorded; Tech Lead applied delegated choices under senior constraints (no big-bang rewrite; no speculative APIs).
|---:|---|
| `DEFINE_HANDLER(CMSG_)` rows | **1032** | 1031 (off-by-one) |
| `STATUS_UNHANDLED` | **380** | 379 (off-by-one) |
| Active-status rows (LOGGEDIN/AUTHED/TRANSFER/RECENTLY) | **628** | — |
| Of which `Handle_NULL` | **2** | 2 |
| **Active/wired** (active-status − NULL) | **626** | 626 (correct) |
| `STATUS_IGNORED` / `STATUS_NEVER` | 16 / 8 | same |
| `ClassicClientOpcodes.inc` unique `CMSG_` names | **686** | 686 |
| Evidence JSON path | **``<buildlogs>``** | cited as `_buildlogs/…` relative; QA initially missed it — **found**; file itself still contains stale 1031/379 and must not be trusted over recount |

**`0x3D013D`:** absent from `Opcodes.h` / `ClassicClientOpcodes.inc` = CONFIRMED; live wire sighting only via owner console / coverage docs = **PARTIALLY CONFIRMED** (not re-sniffed this phase).

**Catch2:** gate **files exist** (CONFIRMED). Current green **NOT re-verified** this read-only phase (no test run). Last recorded green evidence: ``<archive>`` — “All tests passed (**1841** assertions in **110** test cases)”. Treat as historical, not current certification.

---

## 1. Current architectural maturity

Aeternum is **not** yet an independent engine. It is a Forever-capable TrinityCore-derived stack with real additive engineering:

| Layer | State |
|---|---|
| Game/world/combat | Trinity inheritance; client login+combat verified earlier same day (ops) |
| Forever protocol seams | Working profile ExtraInserts auto-apply + remap helpers |
| Metric local sink | Implemented v0 |
| Admin HTTP + Prometheus | Implemented module; default OFF + loopback — **README still wrong** |
| Module SDK | Draft v0.2; host/admin still include Trinity headers |
| Public engine contracts | Absent |
| GPL-free / clean-room path | Absent — depends on decision #1 |

Operational roadmap M0–M9 (metrics/profiles/admin/SDK draft/local CI) is **complete**. This IE-M0 track is separate.

## 2. Main TrinityCore dependencies

CRITICAL: Player/Unit/Creature/WorldObject/Map, WorldSession/Opcodes, World/MapUpdater, DatabaseWorkerPool, ScriptMgr.  
HIGH: Spells/Combat/Handlers/ObjectMgr/DB2.  
MEDIUM: ClientProfile, Metric, Admin, SDK host.  
LOW: profile data files, ClassicOpcodeRemap.h, config-validator, contrib bats.  
GPL-2.0 (`COPYING`): adapter-in-process ≠ license independence.

## 3. Most valuable extraction candidates

1. Forever Protocol (profiles, ExtraInserts, remap, then codecs)  
2. Operations (Metric + Admin ports)  
3. Module SDK Trinity-free public surface  
4. Kernel lifecycle façade  
5. Persistence repositories (later)

## 4. Recommended target architecture

Modular monolith; 9 domains; Trinity as `adapters/trinitycore`; public APIs forbid listed Trinity pointer types; Forever-only single active profile; no microservices by default; preserve all Aeternum work.

## 5. Main technical risks

R1 GPL messaging; R2 Forever breakage; R3 dual-tree drift; R4 draft SDK; R5 opcode gaps (380 UNHANDLED + unknown wire); R6 README honesty; R7 soak incomplete; plus QA-noted gaps (dep license inventory, cert/LoginREST risk ID, no protocol golden corpus).

## 6. Recommended IE-M1 scope

Contracts + IE-ADR outcomes + **docs-only README honesty quick fix (now, after owner OK)** + forbidden-include policy; no gameplay refactors; no cutover. See M1 plan.

## 7. Critical decisions requiring owner approval

See **DECISIONS REQUIRED** below — not decided by this assessment.

---

## QA disagreements and resolution

| # | QA finding | Tech Lead resolution |
|---|---|---|
| Q1 | UNHANDLED 380 vs docs 379; DEFINE_HANDLER 1032 vs 1031 | **Accept QA.** M0 docs updated to **1032 / 380 / active-wired 626**. Prior JSON analyzer off-by-one; JSON path located but numbers inside superseded by recount. |
| Q2 | `_buildlogs/opcode_coverage_70790.json` missing | **Located** at ``<buildlogs>`` (also analyzer `.py` beside it). Cite absolute path; treat contents as stale vs recount. |
| Q3 | `0x3D013D` live not re-verified | Relabel **PARTIALLY CONFIRMED** (docs/log + absent enum/inc only). |
| Q4 | Catch2 green not verified this phase | Explicit **NOT VERIFIED now**; cite archive `m8_catch2_full.log` 1841/110 as last green evidence only. |
| Q5 | Phase J lacks measurable pass/fail | **Accepted** — add pass/fail criteria per scenario (Dependency/Architecture/Risk updates). |
| Q6 | README honesty deferred too far | **Accepted** — elevate to owner decision: **docs-only quick fix now** (not wait for IE-M2). |
| Q7 | “Independent engine” over-promise | **Accepted** — target wording; current = Trinity-derived + Aeternum layer; GPL independence ↔ decision #1. |
| Q8 | IE-M0 cannot be formally complete while owner decisions open | **Agreed** — discovery package can be evidence-updated; **track acceptance remains process-blocked**. |
| Q9 | M1 rollback checklist thin | Expand M1 plan with soak freeze, CHECK_CONFIG, launcher non-regression, go/no-go build-during-soak. |
| Q10 | Risk gaps (dep licenses, cert/LoginREST, golden corpus) | Add R21–R23 in risk register. |

**Disagreement closed without conflict:** Protocol-first vs admin-first extraction — both Tech Lead and QA accept Protocol-first with admin honesty fix in parallel (docs-only).

---

## DECISIONS REQUIRED FROM PROJECT OWNER

**Resolved 2026-11-05** — see [`OWNER_DECISIONS.md`](OWNER_DECISIONS.md). Historical wording retained below for audit.

### Historical prompt (answered)

1. **License / independence goal:** (a) architectural independence under GPL-2.0, (b) long-term clean-room for different licensing, or (c) dual-track? *(Not legal advice.)*  
2. Approve **IE-M1..IE-M9** as distinct from completed operational M0–M9?  
3. Accept / reject / amend **IE-ADR-001..012**?  
4. Keep SDK **v0.2 draft** until Trinity-free 1.0, or freeze a tiny 1.0 now?  
5. Confirm extraction order **Protocol → Operations → Module → Kernel → World/Gameplay**?  
6. IE-M1 contracts **in-repo** (`engine/contracts`) or **docs-only** until M2?  
7. Reaffirm Trinity production until **72h soak + 14d players + explicit approval**?  
8. Keep gameplay modules proposal-only until IE-M3/M4?  
9. Authorize Forever sniff/remap for **`0x3D013D`** on Aeternum only?  
10. **Docs-only quick fix NOW:** authorize README.md / README.it.md correction so Admin HTTP + Prometheus are described as implemented (module, default off, loopback) — **do not defer to IE-M2**.

---

## Incomplete / uninspected (honest)

Full clang include-graph; scripts/ tree map; bnet serviceHash numeric dump; `updates` table row-level vs files (log-only PARTIAL); deep Aeternum audit; `dep/*` license inventory; Handler semantics; soak metric file bodies; Admin pentest; Mobius upstream delta; **Catch2 current exit code**; **live re-sniff of 0x3D013D**; hotfixes DB2 completeness; launcher token security deep review.
