# Aeternum M0 — AUDIT REPORT (Executive + Sections A–K)

**Date:** 2026-11-04 (Europe/Rome)  
**Base snapshot:** `113226264c`  
**Repo:** https://github.com/<owner>/aeternum-engine  
**Actions:** disabled (`enabled:false` verified via API)  
**Italian summary:** [`AUDIT_REPORT.it.md`](AUDIT_REPORT.it.md)

---

## Executive summary (12 lines)

1. Aeternum starts from maintainer’s production Forever line (TrinityCore Forever → advocaite → Aeternum-server → snapshot `113226264c`), not from a greenfield rewrite.  
2. The live **Trinity Engine** already runs client **70790** with bnet **1119** / world **8085** / MySQL **3308** (`auth/characters/world/hotfixes`).  
3. Character creation broke on 70790 because `Classic.ClientExtraInserts` used 70755 numbers (`0x63/0x64`), remapping CREATE to a pet-battle handler; fixed surgically with `0x71/0x72` — **canonical proof that client-version logic is too easy to get wrong**.  
4. Compatibility today is a mix of conf inserts, opcode tables, packet structs, SQL hotfixes, and NOTES — **not** an isolated profile layer.  
5. Core runtime (world loop, MapUpdater thread pool, DatabaseWorkerPool/async, ScriptMgr/SmartAI) is mature TrinityCore and should be **preserved**.  
6. Tests exist via **Catch2** under `tests/` but coverage is narrow relative to protocol/launcher risk.  
7. Metrics hook (Influx-style `Metric.*`) exists but is **disabled** (`Metric.Enable=0`); tick timings beyond “World initialized in 7s” are largely **UNMEASURED**.  
8. Process baseline (read-only): worldserver ~196 MB WS / ~2.7 GB PM, bnetserver ~5 MB WS (at sample time).  
9. Technology stance: keep C++/CMake/MySQL/Catch2; treat WASM, custom allocators, ClickHouse, dual test frameworks as **investigate / adopt only with demonstrated benefit**.  
10. Priority #1 for post-M0 work: **client update architecture + Module SDK** (stability of Forever updates and maintainability at larger scale).  
11. M0 delivers docs only; **no M1 ADRs/designs** beyond open questions and approval gates.  
12. **STOP:** await maintainer’s explicit approval before M1.

---

## A. Current state

- Codebase: full TrinityCore Forever tree (`src/common`, `src/server/{bnetserver,worldserver,game,database,scripts,shared,proto}`, `sql/`, `(operator files, not included)`, `tests/`, `dep/`).  
- Processes (Trinity Engine sample 2026-11-04 ~23:40 Rome): `worldserver` PID 24760 since 22:32; `bnetserver` PID 19944 since 20:30.  
- Client: Forever 1.60.1.**70790**; realm id 70 gamebuild 70790.  
- Build: VS 2022 generator, CMake **4.4.4** on PATH (cache from `<SERVER_ROOT>\build`), Boost **1.84**, OpenSSL Win64, MySQL **8.0.46**.  
- Parallel projects: Aeternum (`<ENGINE_ROOT>`) vs Trinity Engine (`<SERVER_ROOT>`).

## B. What works (preserve)

- Auth + world login path with Forever client / launcher contracts.  
- Character creation **after** ClientExtraInserts fix.  
- MapUpdater worker pool; World::Update session/DB callback structure.  
- DatabaseWorkerPool + prepared statements + DBUpdater.  
- ScriptMgr + SmartAI + C++ scripts tree.  
- `sql/custom` discipline (SQL + rollback + NOTES).  
- Catch2 unit tests already integrated.  
- Operational launcher scripts reading realmlist / setting website env.

## C. What needs improvement

- Client-version surface not isolated → high regression risk on each Forever build.  
- Scattered packet layout fixes without a compatibility test matrix.  
- DBErrors still thousands (many non-Forever leftovers already cleaned in pending SQL; remainder needs triage).  
- Observability: Metric disabled; no Tracy; limited tick/map latency baselines.  
- Guides referencing older builds.  
- Conf comment-on-same-line hazard for opcode insert parsing.  
- No module SDK / stable public API boundary yet (ScriptMgr hooks are internal-coupled).

## D. Forever compatibility

See **`FOREVER_COMPATIBILITY.md`** (deep dive + 70790 create-char case study).  
Supported build **70790** via `build_info` + inserts `0x440071,0x440072` / server `0x470125`. Upstream forks audited earlier today are **not** fully on 70790.

## E. Technology recommendations (brief; details in register)

| Tech | Verdict |
|---|---|
| C++ (current standard as in tree) / CMake | **Keep** — official docs; upgrade std only with compiler/dep proof |
| MySQL 8.0 | **Keep** as system of record |
| Catch2 | **Keep** (already in tree); do not add GoogleTest unless gap proven |
| clang-tidy / cppcheck / sanitizers (local) | **Adopt gradually** in M2 |
| Tracy / Google Benchmark | **Investigate** for M3 — adopt if overhead acceptable |
| OpenTelemetry C++ / Prometheus | **Investigate** after Metric needs clarified; Metric already Influx-oriented |
| Wasmtime / WASM modules | **Investigate only** — isolation interesting, not needed for M1–M3 |
| Custom allocators / pools | **Investigate only** — may be worse than current; require benchmarks |
| ClickHouse | **Reject** for gameplay persistence; revisit only for analytics |
| GitHub Actions (cloud) | **Reject / keep disabled** per maintainer |

## F. Proposed architecture (incremental — conceptual mapping only)

Do **not** force-move the tree in M0/M1 without ADR approval. Map concepts onto reality:

| Concept | Likely current anchors |
|---|---|
| engine/runtime & world | `World.cpp`, mains, MapUpdater |
| engine/networking | WorldSocket*, Opcodes*, classic remap |
| engine/database | DatabaseWorkerPool, DBUpdater |
| forever/protocol & client_profiles | ClassicOpcodes + conf inserts + Packets + build_info → **future isolation target** |
| framework/hooks | ScriptMgr |
| modules | `src/server/scripts`, future explicit modules |
| docs/engine | how-it-works EN/IT (this M0) |

## G. Development priorities (ranked)

| Rank | Item | Crit. | Benefit | Complex | Risk | Maint. |
|---|---|---|---|---|---|---|
| **1** | Client compatibility architecture + Module SDK | Max | Max | High | Med | Lowers long-term |
| 2 | Compatibility/protocol automated tests + launcher contract tests | High | High | Med | Low | Medium |
| 3 | Engineering foundations (warnings, tidy, reproducible local CI) | High | Med | Med | Low | Medium |
| 4 | Performance baselines + optional Tracy/Benchmark | Med | Med | Med | Med | Medium |
| 5 | DBError Forever triage continuation | Med | Med | Low–Med | Med | Ongoing |
| 6 | Admin/observability APIs | Low–Med | Med | High | Med | High |
| 7 | WASM sandbox modules | Low now | Speculative | High | High | High |

## H. Testing strategy

- Keep Catch2; expand unit tests around opcode remap & packet structs.  
- Add integration checks: auth build accept 70790, create-char wire mapping, who/party/arena layouts.  
- Manual Forever client validation remains gate for compatibility milestones.  
- Performance: establish baselines before optimizations (M3).  
- No claim of coverage completeness at M0.

## I. Migration strategy

- Aeternum evolves on its repo; Trinity Engine stays production.  
- Promote changes Trinity←Aeternum only with explicit maintainer approval, backups, and rollback SQL/binaries.  
- Conf/DB migrations need paired rollbacks (already practiced in `sql/custom`).  
- Launcher contracts frozen unless versioned together.

## J. Open questions (need human decisions)

1. Single Forever build support vs multi-build profiles in one process?  
2. Accept temporary dual-maintenance (Aeternum + Trinity) duration?  
3. Module priority after SDK: bots / events / AHBot / other?  
4. Observability backend preference (keep Influx Metric vs OTel/Prometheus)?  
5. License/branding rename timeline (executables still worldserver/bnetserver)?  
6. Whether pending Trinity DBError SQL should land before any Aeternum runtime experiments?

## K. Approval gates before M1

1. Approve M0 audit completeness (this report + audit set + engine EN/IT docs).  
2. Confirm priority #1 = client update architecture + Module SDK.  
3. Confirm **modern ≠ better** evidence rule.  
4. Confirm launcher contracts immutable without joint change.  
5. Confirm Actions remain disabled; local builds only.  
6. Confirm no Trinity Engine restarts/writes from Aeternum work without maintainer’s OK.  
7. Choose answers to open questions J1–J3 at least before architecture ADRs.

**M1 is not started.**
