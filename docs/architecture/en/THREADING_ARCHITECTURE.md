# Threading Architecture (M1)

**Status:** Proposed  
**Italian:** [`../it/THREADING_ARCHITECTURE.md`](../it/THREADING_ARCHITECTURE.md)

## Preserve (Implemented today)

- `MapUpdater`: producer-consumer queue + worker threads (`Maps/MapUpdater.h`).  
- **Do not** unsafely parallelize the same map’s tick.  
- `World::Update` orchestrates sessions + schedules map updates + `ProcessQueryCallbacks` (`World.cpp`).  
- DB work on `DatabaseWorkerPool` threads; results consumed via callbacks.

## Strategy

1. **Measure first** (M3) — map queue latency, tick times, DB callback backlog.  
2. Optimize only with before/after numbers.  
3. No thread-per-module model for Admin pilot (use existing loops + short critical sections).  
4. Custom allocators/pools: investigate only with allocation profiles (may worsen vs current/jemalloc).

## Risks

Incorrect synchronization around MapUpdater / session maps; module callbacks touching world objects off-thread.

## Tests (Planned)

Stress: many maps; TSAN/local where toolchain allows; regression of shutdown `deactivate()`.
