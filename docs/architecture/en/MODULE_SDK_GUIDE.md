# Module SDK guide (static v0.2, M7)

**Status:** Implemented draft (`AETERNUM_SDK_API_DRAFT`) — **not frozen**
**Italian:** [`../it/MODULE_SDK_GUIDE.md`](../it/MODULE_SDK_GUIDE.md)
**Headers:** `src/modules/sdk/include/aeternum/sdk/` (public) + `internal/` (engine only)

## Lifecycle

Host-driven for every registered static `IModule`:

`DISCOVER → LOAD → INIT → START → (running) → STOP → SHUTDOWN`

Dependency order: dependencies start first; stop order is the reverse. Cycles abort startup.

## `critical` flag

| critical | Failure on Load/Init/Start or plan (profile/deps) |
|---|---|
| `true` | `World::StopNow(ERROR_EXIT_CODE)` — clean abort |
| `false` | Module disabled, WARN log, world continues |

## Profile compatibility

`ModuleDescriptor::supportedProfiles` — nullptr-terminated list.
`nullptr` / empty = all profiles. Otherwise the active `Classic.ActiveClientProfile` must match or the module is refused (critical → abort).

## Public vs internal

| Include | Who |
|---|---|
| `aeternum/sdk/version.h`, `module.hpp`, `registry.hpp` | Module authors |
| `aeternum/sdk/internal/*` | Engine / Catch2 planner tests only |

## Example module (sketch)

```cpp
#include "aeternum/sdk/module.hpp"
#include "aeternum/sdk/registry.hpp"

class HelloModule final : public aeternum::sdk::IModule {
public:
  HelloModule() {
    _d.identity = { "example.hello", "0.1.0", ">=0.2.0 <1.0.0" };
    _d.critical = false;
    _d.supportedProfiles = nullptr; // all
  }
  aeternum::sdk::ModuleDescriptor const& GetDescriptor() const override { return _d; }
  aeternum::sdk::ModuleResult OnStart() override { /* ... */ return aeternum::sdk::ModuleResult::Ok; }
private:
  aeternum::sdk::ModuleDescriptor _d{};
};

static HelloModule g_hello;

void AddSC_example_hello() {
  aeternum::sdk::RegisterModule(&g_hello);
}
```

Register `AddSC_example_hello` via `trinity_add_module`. See `admin_monitoring` for a real consumer.

## Out of v0

Dynamic/native/WASM loading, hot unload, frozen 1.0 ABI.

## Logging

Default Logger.root is ERROR-only. Enable lifecycle lines in Server.log:

`
Logger.module.sdk=4,Console Server
Logger.module.admin=4,Console Server
`
