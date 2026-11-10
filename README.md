<p align="center"><img src="docs/assets/aeternum-logo.png" alt="Aeternum" width="220"></p>

# Aeternum Engine

**Italiano:** [README.it.md](README.it.md)

**Launcher:** [Aeternum Launcher](../../../aeternum-launcher) is the companion client launcher for this engine.

Aeternum is a World of Warcraft Forever server: a Battle.net login process (`bnetserver`) and a world process (`worldserver`) that simulate a Forever Classic realm. The game loop, map updater, script system, and MySQL content path come from TrinityCore (GPL-2.0). On top of that tree this repository adds a Forever client profile for build **1.60.1.70862**, a boot-time opcode adapter on the world socket, a static module host, a small frozen C++ API, optional local metrics, and an optional loopback admin HTTP server.

This page describes the source tree as it is. A row marked **Partial** or **Default off** is code you can read and, where a switch exists, turn on. It is not a claim that the feature is the production path. Nothing below states a player count, a benchmark, or a delivery date.

## What the engine can do

| Area | Status | What that means in this tree |
|---|---|---|
| Forever realm processes | **Implemented** | `bnetserver` (Battle.net port + Login REST) and `worldserver` (game port). Shared libraries for network, logging, database, and utilities. |
| World simulation | **Implemented** | TrinityCore-derived sessions, maps, instances, creatures, gameobjects, spells, combat, quests, loot, mail, guilds, and `ScriptMgr`. Content comes from MySQL. |
| Forever client protocol | **Implemented**, profile required | Inbound and outbound world opcodes go through the Protocol Adapter dense tables. The shipped `worldserver.conf.dist` leaves the profile empty, which fail-closes packet dispatch. |
| One active client profile | **Implemented** | `forever/client_profiles/1.60.1.70862` is the active profile and `forever/client_profiles/ACTIVE` points at that id. The previous `1.60.1.70790` profile stays in the tree for reference. Hot-swap of a second profile is not implemented. |
| SQL world content | **Implemented** | Auth, characters, world, and hotfixes databases. Base SQL is under `sql/base/`, updates under `sql/updates/`. |
| Static in-process modules | **Implemented** | Folders under `src/modules/` with a `CMakeLists.txt` link into `worldserver`. Shipped modules: the module host and Admin & Monitoring. |
| Frozen public C++ API 1.0 | **Implemented**, small surface | [`include/aeternum/api`](include/aeternum/api/README.md). Admin, metrics, config snapshot, module lifecycle, protocol types. |
| Module kernel (manifest, FSM, SHA-256) | **Implemented** for static modules | Fail-closed `module.manifest.json`, lifecycle states, admin `GET /modules/lifecycle`. |
| C ABI for native modules | **Implemented** as a header and loader | [`include/aeternum/abi/AeternumABI.h`](include/aeternum/abi/README.md). The loader is compiled in by default and stays idle until `Aeternum.Modules.NativeRuntime = 1`. |
| Local metrics | **Implemented**, **default off** | 10s → 1m → 1h → 1d files, about a 75 MB disk budget, human hotspot report, daily rollup. |
| Admin HTTP + Prometheus text | **Implemented**, **default off**, loopback only | Read-only routes. Operator drain exists and is **default off**. |
| Config schema | **Implemented** on worldserver | `worldserver --check-config`. Unknown `Aeternum.*` keys warn. Missing required keys abort startup. |
| Native `.dll` / `.so` modules | **Partial**, **default off** | Loader, fault fence, packet-hook table, and sample sources exist. Samples are not installed into the build output. |
| WASM guest (wasm3) | **Partial** | The module host creates one fuel-limited VM at boot. Live upload exists only on the telemetry HTTP server, which is **default off**. |
| ECS map kernel | **Partial**, **default off** | Opt-in for one map id and one instance id. That map then skips the legacy `Map::Update` body. |
| In-process TCP edge / mesh | **Partial**, not started | Library is linked. `Aeternum.Edge.Enable` is ignored. Worldserver does not open the edge listener. |
| `aeternum_gateway` | **Partial**, Windows only | Separate IOCP process. It is not wired to `worldserver.conf` and does not return backend bytes to the client. |
| Telemetry HTTP | **Implemented listener**, **default off**, loopback only | `GET /api/telemetry`, operator `POST /api/wasm/deploy`, operator `POST /api/wasm/kill`. Keys are absent from `worldserver.conf.dist`. |
| GitHub-hosted CI | **Absent** | No engine workflow under `.github/workflows/`. The Catch2 suite was removed and `BUILD_TESTING` is forced off. |
| `loginconfig` / `serverconfig` | **Absent** | Login settings are `bnetserver.conf`. World settings are `worldserver.conf` plus `worldserver.conf.d/*.conf`. |

## Processes and the game loop

Two server binaries are built when `SERVERS` is on (the CMake default):

- **`bnetserver`** accepts the Battle.net connection and Login REST. Defaults in [`bnetserver.conf.dist`](src/server/bnetserver/bnetserver.conf.dist): `BattlenetPort = 1119`, `LoginREST.Port = 8081`, `BindIP = "0.0.0.0"`. Certificate and key paths default to `./bnetserver.cert.pem` and `./bnetserver.key.pem`. The build copies the in-tree sample PEM files next to the binary. Replace them for any realm you run, and do not commit private keys.
- **`worldserver`** accepts game connections and runs the world. Defaults in [`worldserver.conf.dist`](src/server/worldserver/worldserver.conf.dist): `WorldServerPort = 8085`, `BindIP = "0.0.0.0"`. There is no separate `authserver` target.

`World::Update` drives sessions, maps, and scripts. Map work can run on the `MapUpdater` worker pool (`src/server/game/Maps/MapUpdater.cpp`). With metrics enabled, world-tick duration and database queue depth are recorded (`world_tick_ms`, `db_queue_login`, `db_queue_character`, `db_queue_world`, `db_queue_hotfixes`). The overrun threshold ships as `Metric.Threshold.world_tick_ms = 100`.

There is no multi-realm orchestrator in this tree. One world process serves the realm id in its config.

## Protocol and client profiles

Forever clients move opcode numbers between builds. Aeternum isolates that in one profile directory and a dense lookup table loaded once at boot.

The active profile id in the tree is **`1.60.1.70862`** (`forever/client_profiles/ACTIVE`). The package contains:

- `manifest.json` (profile id, client build, integrity list)
- `extra_inserts.conf` (client and server opcode inserts)
- `opcodes/opcodes.json` and `opcodes/unknown_policy.json` (integrity-covered)
- seam notes under `seams/`

At world load, `Aeternum::ProtocolAdapterShadow::LoadAtStartup` (`src/server/game/Server/Protocol/AdapterShadow.cpp`) reads `Classic.ActiveClientProfile`, resolves `forever/client_profiles/` (see below), checks SHA-256, and fills the inbound and outbound tables. `WorldSocket` uses those tables for client opcodes and for server opcodes. `ignored_known` opcodes are dropped with a counter and no log line. Unknown opcodes are dropped, counted, and reported with a rate limit from `unknown_policy.json`.

If the profile id is empty, the profile directory cannot be found, or the load fails, the function logs a fatal line and leaves the tables unloaded. Later packets are dropped and outbound packets are not sent. That function does not itself stop the process. The shipped dist file has `Classic.ActiveClientProfile = ""`, so a world copied from the dist file will boot and then refuse game traffic until you set the profile.

Set all of these together when you point a realm at 70862:

| Key | Role | Dist default |
|---|---|---|
| `Classic.ActiveClientProfile` | Profile directory name | empty |
| `Classic.ProfileDataRoot` | Directory that contains `forever/client_profiles/`. Empty searches the process working directory and up to six parents. | empty |
| `Classic.ProfileExtraInserts.AutoApply` | `1` loads inserts from the profile, `0` uses the conf lists, `-1` turns on when the profile id is set | `-1` |
| `Classic.ClientExtraInserts` / `Classic.ServerExtraInserts` | Fallback lists. Ignored when the profile file loads. | empty |

`forever/client_profiles/ACTIVE` is checked against the conf id and a mismatch is logged. The pointer file does not load the adapter by itself. Auth `build_info` / realm `gamebuild` still have to match the client; the profile manifest records build **70862** as the build this package describes. Restart the world after a profile change. Runtime hot-swap of the opcode tables is not implemented.

Offline check for ExtraInserts text:

```bash
python tools/config-validator/validate_extra_inserts.py \
  --conf path/to/worldserver.conf \
  --profile forever/client_profiles/1.60.1.70862
```

`include/aeternum/api/protocol/adapter.hpp` is the older public skeleton. The running adapter is `src/aeternum/protocol_adapter/`.

## Data and content

World content (creatures, spawns, quests, items, hotfixes, and the rest of the world schema) is loaded from MySQL or a compatible server. Four connection strings are required by the world dist file:

- `LoginDatabaseInfo`
- `WorldDatabaseInfo`
- `CharacterDatabaseInfo`
- `HotfixDatabaseInfo`

The dist file ships local placeholder values on `127.0.0.1` port `3306` with the upstream template user and schema names (`auth`, `world`, `characters`, `hotfixes`). Replace them before you start the processes. Do not commit filled `.conf` files.

Apply SQL from `sql/base/` and `sql/updates/` (`auth`, `characters`, `world`, `hotfixes`) in the usual order, then `sql/custom/` if you use it. `DataDir` (dist default `"."`) must point at extracted client data: dbc, maps, vmaps, and mmaps. CMake `TOOLS` defaults on and builds the map, vmap, and mmap extractors.

Prefer SQL for content changes. A higher-level content authoring SDK is not in this tree. Module SQL is not applied by the core database updater; `DBUpdater` scans `sql/updates` and `sql/custom` only.

## Configuration

### Files that ship

| File | When it is read |
|---|---|
| `worldserver.conf` | Copied by you from `worldserver.conf.dist`. Main world config. On Windows the dist file is copied next to `worldserver` when `COPY_CONF` is on (the default). On Unix the same option copies it to the build `etc` directory and installs it into `CONF_DIR`. |
| `bnetserver.conf` | Copied by you from `bnetserver.conf.dist`. Same copy rule for `bnetserver`. |
| `worldserver.conf.d/*.conf` | Every `*.conf` file in that directory, loaded after the main file. `*.conf.dist` is ignored. |
| `modules/<id>/module.manifest.json` | Read at world boot from `Aeternum.Modules.ManifestDir`, or from `<directory of worldserver.conf>/modules` when that key is empty. |

The world CMake build copies `src/modules/admin_monitoring/conf/admin_monitoring.conf.dist` into `worldserver.conf.d/` and copies that module's manifest to `modules/admin.monitoring/module.manifest.json`. Rename the admin dist file to `admin_monitoring.conf` before those `Admin.*` keys apply. Until you do, the code defaults keep admin HTTP off (`Admin.Enable` false, `Admin.Port` 0).

This tree has no `loginconfig` or `serverconfig` files, and no install rule that would emit them. The repo-root [`modules/`](modules/) tree (native samples and `modules/wasm/mod_test`) is source for standalone module builds. It is not copied into the worldserver output. The runtime `modules/` directory next to the config starts with the admin manifest only.

`worldserver --check-config` loads the conf files, runs `Trinity::ConfigSchema`, prints issues, and exits without opening databases. `bnetserver` has no `--check-config` flag. Required keys, unless `Config.Schema.Required` overrides them, are `RealmID`, `DataDir`, `LoginDatabaseInfo`, `WorldDatabaseInfo`, and `CharacterDatabaseInfo`. `Config.Schema.FailFast` defaults to true in code, so a missing required key aborts startup. Unknown keys under `Admin.`, `Metric.`, `Crash.`, `Config.Schema.`, `Forever.`, `Aeternum.`, and `Module.` are warnings when `Config.Schema.WarnUnknown` is true (the code default). Warnings do not block startup and do not stop the code from reading the key.

### Switches and where they live

Leave the off defaults in place unless you mean to enable the feature.

**Admin & Monitoring** — keys live in `admin_monitoring.conf.dist`. They apply only after you rename that file to `.conf`.

```ini
Admin.Enable = 0
Admin.BindIP = "127.0.0.1"
Admin.Port = 0
Admin.Token = ""
Admin.TokenFromEnv = "AETERNUM_ADMIN_TOKEN"
Admin.TokenMinLength = 32
Admin.OperatorTokenFromEnv = "AETERNUM_ADMIN_OPERATOR_TOKEN"
Admin.Drain.Enable = 0
Admin.Drain.MinDelaySec = 120
```

The bind must be loopback (`127.0.0.1`, `::1`, or `localhost`). `0.0.0.0` and `::` are rejected. A non-loopback peer is rejected even after accept. Prefer the environment variable named by `Admin.TokenFromEnv`. A non-empty `Admin.Token` in the file logs a warning; the value is not written to the log. Minimum length defaults to 32. Failed auth is rate-limited (`Admin.AuthFailMax`, `Admin.AuthFailWindowSec`).

**Metrics** — keys are already in `worldserver.conf.dist`. `Metric.Enable = 0` is the default. `Metric.Sink` defaults to `"local"`.

```ini
Metric.Enable = 0
Metric.Sink = "local"
Metric.Local.Directory = "Logs/metrics"
Metric.Local.BudgetMB = 75
Metric.Local.RetentionDays = 14
Metric.Local.FlushSeconds = 10
Metric.Local.MaxSeries = 256
```

`Metric.Sink` also accepts `influx` and `both`. `Metric.ConnectionInfo` is the InfluxDB line and is unused while the sink stays `local`.

**Module host** — read by the host. They are not written in `worldserver.conf.dist`. Code defaults:

| Key | Default | Effect |
|---|---|---|
| `Aeternum.Modules.ManifestDir` | empty → `<conf directory>/modules` | Manifest root. |
| `Aeternum.Modules.RequireManifest` | `0` | `1` rejects a static module with no manifest. `0` loads it and logs a warning. |
| `Aeternum.Modules.NativeRuntime` | `0` | `1` scans the manifest root for `"runtime": "native"`. Read only when the binary was built with `AETERNUM_NATIVE_MODULE_RUNTIME` (CMake default **ON**). |
| `Aeternum.Modules.HotReload` | `0` | Allows a native module whose manifest says `hot-reloadable` to reload at the world-thread sync point. |

`ManifestDir`, `RequireManifest`, and `NativeRuntime` are on the schema allowlist. `HotReload` is not, so setting it warns under `Aeternum.*`.

**Edge keys in the world dist file** — present, and ignored:

```ini
Aeternum.Edge.Enable = 0
Aeternum.Edge.BindIP = "127.0.0.1"
Aeternum.Edge.Port = 0
Aeternum.Edge.MaxSessions = 128
Aeternum.Edge.ConnectTimeoutMs = 2000
Aeternum.Edge.IdleTimeoutMs = 30000
Aeternum.Edge.BackendAddresses = ""
Aeternum.Edge.BackendPorts = ""
```

These names are not on the schema allowlist, so a stock conf copied from the dist file warns once per key. If `Aeternum.Edge.Enable` is true, world startup logs that the key is ignored because the transport edge is `aeternum_gateway`, and then it does not bind. See [Edge, gateway, telemetry, WASM, and ECS](#edge-gateway-telemetry-wasm-and-ecs).

**Telemetry HTTP and ECS** — also absent from the dist file. Adding them warns (they are under `Aeternum.` and not allowlisted). Defaults used by the code:

```ini
Aeternum.TelemetryHttp.Enable = 0
Aeternum.TelemetryHttp.BindIP = "127.0.0.1"
Aeternum.TelemetryHttp.Port = 0

Aeternum.ECS.Enabled = 0
Aeternum.ECS.OwnedMapId = -1
Aeternum.ECS.OwnedInstanceId = -1
Aeternum.ECS.Capacity = 1024
Aeternum.ECS.Workers = 16
```

Environment overrides follow the existing config rule: a `TC_` variable in upper snake case replaces the file value (`TC_METRIC_ENABLE`, and the same pattern for other keys).

## Modules and the public SDK

### What a module is today

A static module is a directory in `src/modules/` with its own `CMakeLists.txt`. `trinity_add_module` links it into `worldserver` and calls its script loader from `AddModuleScripts()` after core scripts. Optional `CONFIGS` are copied to `worldserver.conf.d/`. Optional `MANIFEST` is copied to `modules/<id>/module.manifest.json`. Details and the loader example are in [`src/modules/README.md`](src/modules/README.md).

Two modules are wired:

| Module | CMake option | Default | Manifest installed |
|---|---|---|---|
| Module host `aeternum-sdk` | `BUILD_AETERNUM_MODULE_SDK` | ON | no (legacy load + warning unless you require manifests) |
| Admin & Monitoring | `BUILD_ADMIN_MONITORING` | ON | `admin.monitoring` version `0.3.0`, `critical: false` |

The host (`src/modules/sdk`) starts on world startup. It validates manifests, walks the static lifecycle (Discover → Validated → Loaded → Active, or Error), and on shutdown stops modules in reverse order. A critical module failure aborts startup through the process-control port. A non-critical failure disables that module and continues. The host headers are still draft: the Trinity adapter under `src/modules/sdk/adapters/trinity` includes game headers. The frozen surface is `include/aeternum/api`, not those draft headers.

Frozen 1.0 headers and their consumers:

| Header | Used by |
|---|---|
| `status.hpp` | the other public headers |
| `ops/admin.hpp`, `ops/metrics.hpp`, `ops/runtime.hpp`, `ops/services.hpp` | admin module and the Trinity ops adapter |
| `config/snapshot.hpp` | config schema / `--check-config` |
| `sdk/lifecycle.hpp` | module host |
| `protocol/opcode.hpp`, `protocol/profile.hpp` | profile and opcode types |
| `protocol/adapter.hpp` | public skeleton; the socket uses `src/aeternum/protocol_adapter` |
| `data_loaded.hpp`, `ecs_map.hpp` | boot hook and the optional ECS map driver |

The C ABI in `include/aeternum/abi/AeternumABI.h` is the boundary for a native library: plain C types, `aeternum_module_entry`, host function table checked with `struct_size`. A CMake build step runs `contrib/aeternum/ci/check_abi_no_stl.ps1` so that ABI headers do not include the C++ standard library. The script is PowerShell; a host without PowerShell fails that custom target.

### Native runtime (default off)

`AETERNUM_NATIVE_MODULE_RUNTIME` defaults **ON**, so the OS loader (`LoadLibraryExW` on Windows, `dlopen` on POSIX) is linked. At runtime nothing is loaded until `Aeternum.Modules.NativeRuntime = 1`.

When that is set, the host scans each subdirectory of the manifest root. A manifest with `"runtime": "native"` is hashed (SHA-256 of the listed files), checked for ABI version, and loaded. The directory must not be a symlink or reparse point. A critical native failure aborts boot and rolls back. A contained fault marks the process tainted and asks the world thread for a clean shutdown after 60 seconds (`World::ShutdownServ`). That path is best-effort: a wild write before the fault is outside the fence, and an infinite loop is not preempted. The existing `MaxCoreStuckTime` freeze detector remains the world-thread watchdog.

Packet hooks are a dense table, at most four callbacks per opcode, subscribed from `on_init`, then frozen. Movement opcodes can be subscribed. Client opcodes that `WorldSocket` handles at socket level stay denied. Host allocations are cookie-checked (module, generation, size); an orphan free or a double free is rejected.

To run a native module you build it yourself, place `module.manifest.json` and the library in `<manifest dir>/<id>/`, and set `Aeternum.Modules.NativeRuntime = 1`. The sources under `modules/native/` (`mod_chatguard`, `mod_combat_override`, `mod_anticheat`, `mod_data_mutator`, `mod_holographic_ui`) are standalone CMake projects for that ABI. Their own READMEs describe local hook experiments. They are not installed by the worldserver build, and they are not a supported gameplay feature set.

`Aeternum.Modules.HotReload = 1` only affects a native module that declares `loadPolicy` `hot-reloadable`. The reload runs at the existing world-thread sync point.

## Metrics and admin HTTP

### Local metrics

With `Metric.Enable = 1` and `Metric.Sink` including `local`:

1. Samples aggregate in windows **10s → 1m → 1h → 1d** into `.mlines` files under `Metric.Local.Directory` (default `Logs/metrics`).
2. Cardinality stops at `Metric.Local.MaxSeries` (default 256). Labels that would explode the series count are not kept.
3. Disk use is capped by `Metric.Local.BudgetMB` (default 75) and `Metric.Local.RetentionDays` (default 14). The oldest files go first. Gzip of the coarse windows is `Metric.Local.Compress` (default 0).
4. `latest_report.txt` lists the top series and a short hint from the metric catalog (`src/common/Metric/MetricHumanReport.cpp`).
5. About once per 24 hours of flushes, `daily_YYYYMMDD.txt` records avg / p95 / max per series plus warn, error, and fatal counts. That file counts toward the same budget.

The catalog and the threshold hints are in [docs/architecture/en/METRIC_CATALOG.md](docs/architecture/en/METRIC_CATALOG.md).

### Admin HTTP

The module is linked by default. The listener starts only when `Admin.Enable` is true and `Admin.Port` is non-zero, which requires the renamed `worldserver.conf.d` file. Routes:

| Method and path | Auth | Body |
|---|---|---|
| `GET /health` | none | JSON liveness |
| `GET /uptime` | read token | uptime seconds |
| `GET /sessions` | read token | session counts |
| `GET /modules/lifecycle` | read token | module id, critical flag, FSM state, reason |
| `GET /metrics` and `GET /metrics/dense` | read token | Prometheus text (`?dense=1` or the `/dense` path) |
| `GET /metrics/store` | read token | local store summary |
| `GET /report/latest` and `GET /report/daily` | read token | the metric report files |
| `POST /admin/drain?delay_sec=` | operator token | schedules `World` shutdown, only if `Admin.Drain.Enable = 1` |

Send the token as `Authorization: Bearer` or `X-Admin-Token`. `/health` is the only unauthenticated route. Anything other than GET, except the drain POST, is rejected. Drain delay is at least `Admin.Drain.MinDelaySec` (default 120). The audit log defaults to `Logs/admin_audit.log` with a 2 MB rotation and three kept files. Tokens are not written to that log.

Crash dumps use `Crash.Directory` (empty means the code default directory), `Crash.BudgetMB` (default 256), and `Crash.RetentionDays` (default 30). Those keys are applied at world startup even when admin HTTP is off, as long as they are visible to config. They are documented in the admin dist file; if that file is still named `.conf.dist`, the code defaults apply.

## Edge, gateway, telemetry, WASM, and ECS

These pieces are in the tree from the later milestones. Each one below is the runtime behavior, not the milestone tag.

### In-process edge (M13 library)

`aeternum-meshing` (`src/modules/sdk/meshing/AsyncEdge.cpp`) is a bounded opaque TCP relay: fixed session slots (code cap 1024, dist example 128), IPv4 backends, optional handoff frame on the backend link. It is linked into the module host. World startup does not call `Start`. `Aeternum.Edge.*` does not fill that config. Map changes call `NotifyMapChange`, which returns disabled unless some other code has attached a running edge. Nothing in `src/server` attaches one. Treat the dist keys as a commented example that the schema does not yet allowlist.

### Windows gateway (M15)

On Windows, CMake builds `aeternum_gateway` from `src/modules/sdk/gateway/Main.cpp`. It is not linked into `worldserver`, and it does not read `worldserver.conf`.

```text
aeternum_gateway --bind 127.0.0.1 --port 3725 --backend 127.0.0.1:8085
```

`--backend` is required (the process exits 2 without one). Repeat it up to 16 times. Addresses are IPv4. The CLI default bind is `127.0.0.1`, default port is `3725`, worker count is 4, and the session table cap is 50000 (`kMaxCapacity`). That cap is a table size, not a measured connection count.

What the process actually does: accept TCP, read at most 128 bytes per completion, wrap those bytes in an internal 16-byte header, and send the frame to a backend chosen by slot modulo backend count. The thread that reads the backend socket discards those bytes. It does not write them back to the accepted client. This is not a transparent proxy in front of `worldserver`, and it is not started by the world process.

### Telemetry HTTP (default off)

`Aeternum.TelemetryHttp.Enable` defaults false. A port outside 1–65535 is refused. The listener refuses a non-loopback bind and refuses a non-loopback peer. It reuses the admin token resolvers (`Admin.Token` / `Admin.TokenFromEnv` for read, `Admin.OperatorToken` / `Admin.OperatorTokenFromEnv` for operator). A token must be 32–256 visible ASCII characters. Browser `Origin` and `Cookie` headers are rejected.

| Method and path | Role | Result |
|---|---|---|
| `GET /api/telemetry` | read | JSON snapshot from the in-process profiler (256 identity cap) plus WASM module generation state |
| `POST /api/wasm/deploy` | operator | accepts one `application/wasm` or `application/octet-stream` body, or a multipart body, up to 256 KiB, and loads it into the hot WASM pipeline |
| `POST /api/wasm/kill` | operator | JSON `{"module","generation"}`. Sets a cooperative abort flag for that generation. The HTTP 200 means the flag was set, not that the guest has stopped. |

### WASM

On every module-host start the process creates one wasm3 environment (embedded `dep/wasm3`), default fuel budget 1,000,000 (allowed range 1..10,000,000), 64 KiB VM stack, and at most 1 MiB of linear memory. There is no WASI, filesystem, or network import. If `<manifest dir>/wasm/mod_test.wasm` exists, that file is loaded and run; the build does not install a sample. Init failure is an error log, not a boot abort.

`modules/wasm/mod_test` is a separate build (`build.py` plus a WASI SDK clang). The hot deploy path (`/api/wasm/deploy`) loads a single export pipeline (`aeternum_boss_tick` under the identity `hot/Aeternum_BossAI`) into that VM. Fuel limits compiled steps; a host callback is not preempted. This is an embedded interpreter with a byte cap, not an OS sandbox for untrusted code.

### ECS kernel (default off)

`Aeternum.ECS.Enabled` defaults false. When it is true, the factory is installed only for the single pair `Aeternum.ECS.OwnedMapId` and `Aeternum.ECS.OwnedInstanceId` (both must be ≥ 0 and must match that map). `Capacity` must be 1..4096 and `Workers` must be 1..16, or the factory returns no driver. The registry holds at most 16 map/instance kernels.

`Map::Update` then calls `CustomUpdate` and returns. The legacy map update body does not run for that instance. Other maps stay on the legacy updater. Turning this on is an ownership switch for one instance, not an extra system beside the normal map tick.

## Build

Typical dependencies: CMake 3.24 or newer, a C++ toolchain, Boost, OpenSSL, a MySQL or MariaDB client, and zlib. The root `CMakeLists.txt` forces `BUILD_TESTING` off. The comment there records that the Catch2 milestone suite was removed, so there is no `tests` target to build.

```bash
cmake -S . -B build
cmake --build build --config RelWithDebInfo --target worldserver --target bnetserver
```

Useful cache options (defaults in `cmake/options.cmake` and `src/modules/sdk/CMakeLists.txt`):

| Option | Default | Effect |
|---|---|---|
| `SERVERS` | ON | `worldserver` and `bnetserver` |
| `TOOLS` | ON | map / vmap / mmap tools |
| `SCRIPTS` | `static` | scripts linked into the world binary |
| `COPY_CONF` | ON | copy `*.conf.dist`, admin dist, and the admin manifest into the output |
| `BUILD_AETERNUM_MODULE_SDK` | ON | module host |
| `BUILD_ADMIN_MONITORING` | ON | admin module |
| `AETERNUM_NATIVE_MODULE_RUNTIME` | ON | compile the native loader; runtime switch stays off |
| `WITH_DYNAMIC_LINKING` | OFF | Trinity script hot-swap. Independent of the native module loader. |

On Windows, `aeternum_gateway` is built with the module SDK. Binaries go to `build/bin/<Config>/` for multi-config generators. Unix install layout uses `bin/` and the configured `CONF_DIR`.

`contrib/aeternum/ci/local_ci.ps1` still configures a build and then requires `tests.exe`. That target is gone, so the script stops there. It does not enable GitHub Actions. The only GitHub Actions workflow is `.github/workflows/neutrality.yml`, which checks that no server-specific references are committed; it does not build `worldserver`.

## Run

1. Create four empty schemas and apply `sql/base` plus `sql/updates` for auth, characters, world, and hotfixes. Point `DataDir` at extracted client data.
2. Copy `bnetserver.conf.dist` to `bnetserver.conf` and `worldserver.conf.dist` to `worldserver.conf`. Set the four database strings, realm id, ports, and `BindIP` to the interfaces you intend to open. The dist defaults listen on `0.0.0.0`.
3. Set `Classic.ActiveClientProfile = "1.60.1.70862"`. If the process working directory is not inside this repository (or within six parents of it), set `Classic.ProfileDataRoot` to the source root.
4. Put Login REST and bnet PEM paths on files that belong to this host. The sample PEMs copied by the build are not a deployment certificate.
5. Optionally: `worldserver --check-config` before the first real start.
6. Start `bnetserver`, then `worldserver`.
7. Leave `Metric.Enable`, admin HTTP, native runtime, telemetry HTTP, and ECS at their off defaults until you have a reason to turn one on. Renaming `admin_monitoring.conf.dist` to `.conf` is what makes the `Admin.*` block visible; `Admin.Enable` in that file ships as `0`.

Do not point this build at a database or port that another realm is already using unless that migration is deliberate.

## Documentation

Operator-facing docs that match the layout above:

- Engine tour: [docs/engine/en/00_INDEX.md](docs/engine/en/00_INDEX.md)
- Client launcher: [Aeternum Launcher](../../../aeternum-launcher)
- Client profile direction: [docs/architecture/en/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md](docs/architecture/en/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md) and [forever/client_profiles/README.md](forever/client_profiles/README.md)
- Metric catalog: [docs/architecture/en/METRIC_CATALOG.md](docs/architecture/en/METRIC_CATALOG.md)
- Admin module notes: [docs/architecture/en/ADMIN_MONITORING_MODULE.md](docs/architecture/en/ADMIN_MONITORING_MODULE.md)
- Static module layout: [src/modules/README.md](src/modules/README.md)
- Public API: [include/aeternum/api/README.md](include/aeternum/api/README.md)
- C ABI: [include/aeternum/abi/README.md](include/aeternum/abi/README.md)

Milestone write-ups under `docs/m2/`, `docs/m3/`, `docs/development/`, and `docs/independent_engine/` record how individual blocks landed. Several of them still say "local only" or describe an earlier default. Where they disagree with this page, this page follows the source: `WorldSocket`, `worldserver.conf.dist`, `ConfigSchema.cpp`, and `src/modules/sdk/CMakeLists.txt`.

## Contributing and license

How to contribute (same model as TrinityCore):

1. Fork the repository.
2. Create a branch in your fork for one fix (`git checkout -b fix/short-description`).
3. Commit, push the branch to your fork, and open a pull request against `main`.

`main` is protected: nobody pushes to it directly, every change needs a pull request with an approving review, and force pushes and deletion are blocked. Keep changes neutral: no realm names, addresses, passwords, keys, local paths or custom rates, and new options off by default. Details, SQL file naming and the bug report format are in [CONTRIBUTING.md](CONTRIBUTING.md). Security problems go through [SECURITY.md](SECURITY.md), never a public issue.

License: **GPL-2.0**. The terms are in [COPYING](COPYING). Copyright history is in [AUTHORS](AUTHORS). This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. Derivative work of TrinityCore, also GPL-2.0. Existing copyright headers stay authoritative.
