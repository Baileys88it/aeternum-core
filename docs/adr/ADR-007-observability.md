# ADR-007: Measure with existing Metric first

**Status:** Accepted  
**Date:** 2026-11-04  
**Milestone:** M1  
**IT:** [`it/ADR-007-observability.md`](it/ADR-007-observability.md)


## Problem
Ticks/maps largely UNMEASURED; enabling heavy stacks without budget risks overhead.

## Solution
1) Define metric set (see observability architecture).  
2) Prefer enabling/fixing in-tree **Metric** (Influx line protocol) before adopting Tracy/OTel/Prometheus in production.  
3) Tracy/OTel allowed as **dev/investigate** with measured overhead.

## Why
modern ≠ better; lowest integration risk.

## Alternatives
OTel-first — deferred.  
Tracy always-on prod — rejected until overhead known.

## Sources
- In-tree `src/common/Metric`  
- https://opentelemetry.io/docs/  
- https://github.com/wolfpld/tracy  
- https://prometheus.io/docs/

## Complexity / Perf / Maintenance
Low–med; overhead must be budgeted; one primary path preferred.

## Tests
Enable Metric on staging; compare CPU/RAM before/after; never claim wins without numbers.

## Risks & rollback
Conf flag `Metric.Enable`; disable instantly.

## Decision (Accepted M2 — 2026-11-05)

1. **Start:** existing TrinityCore `Metric` → **local** file/log sink (no external services until justified).  
2. **Capability:** core must be able to register the full server metric set (ticks, maps, sessions/latency, opcode top-N count/time, pkt in/out, DB pool latency/queue, map/grid/creature counts, script/AI/SmartAI, spell/aura, memory, threads, crashes) — see `docs/architecture/en/OBSERVABILITY_AND_PERFORMANCE.md` catalog (name, unit, meaning, healthy threshold, what to do if bad).  
3. **Readable:** periodic human summary with top-N hotspots + “where to improve” hints; later Prometheus text via admin localhost.  
4. **Tiny disk footprint:** aggregates only (10s/1m → 1h/1d roll-ups); no per-event metric logging by default; compact format (+ optional compression); size-based rotation + retention; **hard budget default ~75 MB** (50–100 MB range); cardinality limits (top-N opcodes/maps, **no per-player labels**); detailed tracing on-demand with time limit.  
5. **Code:** **not in M2** (docs only). Implement enable+sink+summary in **M3**; denser catalog + admin `/metrics` in **M5–M8**.
