# M1 Kernel — Block 5 (Multi-file integrity + ABI STL lint)

**Branch:** `feat/m1-kernel` (local commits; push / production deploy only when maintainer authorizes)  
**Scope:** Extend D10 integrity to multiple allowlisted files; lock ABI headers against STL includes; freeze STOP_TESTINST timeouts in a CMake template. No hot-reload. `WITH_DYNAMIC_LINKING=OFF`. Do **not** restart `_public_iem1` as part of this block.

## Deliverables

| Item | Path | Notes |
|---|---|---|
| STOP_TESTINST template | `contrib/aeternum/scripts/STOP_TESTINST.bat.in` | Graceful **120s** / hard **180s**; CMake `configure_file` → `aeternum_runtime_gen/STOP_TESTINST.bat` |
| Multi-file integrity | `manifest.hpp` + `manifest_gate.hpp` | `integrity.files: [{path, sha256}, ...]` XOR legacy `file`+`sha256` |
| ABI STL lint | `contrib/aeternum/ci/check_abi_no_stl.ps1` | CMake target `aeternum-abi-stl-lint` (ALL); fails build on STL includes in ABI headers |
| Catch2 | `ManifestGate.cpp`, `ManifestParser.cpp` | multi-file good/bad/missing/traversal/duplicate/legacy |
| This doc | `M1_KERNEL_BLOCK5.md` | |

## STOP_TESTINST timeouts

`STOP_SERVER.bat.in` already used `-ShutdownDelay 120 -ExitTimeoutSec 180`. Instance bat `_testinst_iem1\STOP_TESTINST.bat` was hand-edited in Blocco4; it had **no** `.bat.in` source, so CMake could not regenerate it. Blocco5 adds the template + `configure_file` with cache paths:

- `AETERNUM_TOOLS_DIR` (default `<ENGINE_ROOT>/_tools`)
- `AETERNUM_TESTINST_BIN_DIR` (default `<ENGINE_ROOT>/_testinst_iem1/bin`)

Path-scoped stop rules unchanged (only that runtime dir; never Trinity / other instances).

## Multi-file integrity (D10 extension)

### Schema

```json
"integrity": {
  "files": [
    { "path": "module.bin", "sha256": "<64 hex>" },
    { "path": "assets/cfg.json", "sha256": "<64 hex>" }
  ]
}
```

Legacy (still supported):

```json
"integrity": { "file": "module.bin", "sha256": "<64 hex>" }
```

### Precedence (safe)

| Manifest content | Behaviour |
|---|---|
| Only `files` array | Validate every entry (order preserved) |
| Only `file` + `sha256` | Legacy single-file path |
| Both forms present | **Parse reject** `integrity_ambiguous_form` (no silent skip) |
| Neither | No hash check (unchanged M1 policy) |

### Path / hash rules

- Paths relative under `<manifestDir>/<id>/`; `IsSafeIntegrityRelativePath` (no `..`, no absolute, no `\`, safe charset).
- Reject duplicate paths, empty `files` array, >64 entries, bad hex length.
- Boot-only hashing (`Sha256FileHex`, max 256 MiB per file). Never on World tick / packet path.
- Fail reasons **name the offending path**:
  - `integrity_sha256_mismatch:<path>`
  - `integrity_file_missing:<path>`
  - `integrity_file_unsafe:<path>`
- Rejected → FSM **Error** for that module; non-critical → world keeps running (same as Blocco4).

`GET /modules/lifecycle` already surfaces `reason` from the gate/FSM snapshot.

## ABI STL lint

**Scanned (must stay C11 / no STL):**

1. `include/aeternum/abi/**`
2. `src/modules/**/abi/**` (future per-module ABI trees)
3. `src/modules/**/*ABI.h(pp)` name matches

**Not scanned (STL allowed):** `src/modules/sdk/include/**` (host SDK), `*.cpp`, `admin_monitoring` implementation, `adapters/trinity`.

Banned includes (non-exhaustive list in script): `<string>`, `<vector>`, `<map>`, `<unordered_map>`, `<set>`, `<list>`, `<deque>`, `<memory>`, `<functional>`, `<optional>`, `<variant>`, `<string_view>`, `<array>`, `<iostream>`, `<sstream>`, …  
Comments and block comments are ignored. Error format: `path:line: #include <...>`.

CMake: `add_custom_target(aeternum-abi-stl-lint ALL ...)` with `add_dependencies` on `aeternum-sdk` / `modules`.

## Out of scope

Push to `origin`, deploy to `_public_iem1`, hot-reload, Authenticode, Trinity-type forbidden-includes expansion over all of `src/modules/**` (separate from STL ABI lint; host SDK still uses STL intentionally).

## Verify

```text
ctest -C RelWithDebInfo -R "\[m1\]" --output-on-failure
powershell -File contrib/aeternum/ci/check_abi_no_stl.ps1 -RepoRoot <source>
```
