# M10 local WASM sandbox (not hostile-code production ready)
Runtime: wasm3 v0.5.0, pinned 6b8bcb1e07bf26ebef09a7211b0a37a446eafd52, MIT.
Reproduce with WASI SDK 24.0 x86_64 Windows, clang 18.1.2:
`python build.py <wasi-sdk>/bin/clang.exe <build-output-dir>`.
CMake BUILD_TESTING requires `-DAETERNUM_WASM_CLANG=<absolute clang path>`.
All fixtures are actual C compiled with -nostdlib, not a handmade WASM emulator.
The build records full commands, compiler version, source/output SHA256.

SDK HostDiscoverAndStart allocates a dedicated environment and 64KiB VM stack;
M9 data hook calls it before MapManager worker creation. It optionally loads
`<manifestDir>/wasm/mod_test.wasm` (no implicit module scan). Shutdown resets VM.
This change never installs a sample in any live instance. Boot integration is
source/build verified, not live boot tested. No public ABI layout change.

One module per VM; max input256KiB, linear memory16 pages/1MiB (initial and grow),
aggregate interpreter allocation cap16MiB (boot-thread-only lifecycle).
Guest log has exactly one allowlisted function `aeternum.aeternum_host_log: v(i)`;
o WASI/libc/network/filesystem bindings, native addresses, imported globals or
memory. Start sections rejected; on_init must be void(void). Host validates
nonzero offset, memory bounds and NUL within256 bytes; log text is formatted as
data, never as a host format string. Guest pointer0 is intentionally rejected.
Unknown imports rejected even if unreachable. Callback exceptions become traps.

Limits: no CPU fuel/timeout, do NOT run hostile/unreviewed infinite loops;
negative tests are terminating. Parser and compiler use upstream wasm3, not a
security proof. VM stack and linear memory are bounded; this is not an OS
process sandbox. The old pinned upstream version needs independent security
review before untrusted modules. Allocator accounting covers every allocation
through built interpreter's m3_Malloc/m3_Realloc/m3_Free, not all C++/CRT/process
memory; repeated/failure tests do not establish universal leak-freedom.
Fixtures and source are local-only; no deployment or live verification.
