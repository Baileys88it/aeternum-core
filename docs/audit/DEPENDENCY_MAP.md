# Dependency Map (M0)

## Third-party under `dep/` (present in tree)

argon2, boost (headers path also external Boost 1.84), CascLib, efsw, fmt, g3dlite, gsoap, jemalloc, mysql, openssl (+ openssl_ed25519), protobuf, rapidjson, readline, recastnavigation, SFMT, short_alloc, threads, utf8cpp, valgrind, zlib.

## External (build machine — Trinity Engine cache)

| Dep | Evidence |
|---|---|
| CMake 4.4.4 | `cmake --version` |
| Visual Studio 17 2022 | `CMAKE_GENERATOR` in CMakeCache |
| Boost 1.84.0 | `C:/local/boost_1_84_0` |
| OpenSSL-Win64 | `C:/Program Files/OpenSSL-Win64` |
| MySQL 8.0.46 | mysql.exe version; libs in cache |

## Test framework

**Catch2** fetched/used via CMake (`Catch2::Catch2WithMain` in tests). Prefer extending Catch2 over adding GoogleTest.

## Runtime coupling (conceptual)

```text
worldserver → game → (common, database, shared)
bnetserver → (common, database, proto/services)
scripts → game (ScriptMgr)
tools/extractors → common/collision/CascLib
```

No WebAssembly runtime in tree. Metric module talks Influx line protocol when enabled.
