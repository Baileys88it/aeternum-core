# World loop & MapUpdater threading

**Status:** Implemented

## World::Update

`World.cpp` `World::Update(uint32 diff)` drives sessions (`UpdateSessions`), map updates, and `ProcessQueryCallbacks()` for async DB completions.

## MapUpdater

`MapUpdater` (`Maps/MapUpdater.h/.cpp`):

- `activate(num_threads)` starts worker threads.  
- `schedule_update(Map&, diff)` enqueues `MapUpdateRequest`.  
- Workers pull from `ProducerConsumerQueue`.  
- `wait()` / `deactivate()` for barrier & shutdown.  
- **Invariant to preserve:** do not parallelize the **same** map tick unsafely.

**Sources:** `World.cpp`, `MapManager.*`, `MapUpdater.*`.
