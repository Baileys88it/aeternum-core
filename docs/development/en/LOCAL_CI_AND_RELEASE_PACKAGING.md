# Local CI and release packaging (proposal)

**Date:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Status:** Local tooling + packaging **proposal**. GitHub Actions stay **disabled**.  
**IT:** [`../it/LOCAL_CI_AND_RELEASE_PACKAGING.md`](../it/LOCAL_CI_AND_RELEASE_PACKAGING.md)

## 1) Local CI (implemented script)

Script: [`contrib/aeternum/ci/local_ci.ps1`](../../../contrib/aeternum/ci/local_ci.ps1)

```powershell
# From anywhere:
powershell -NoProfile -ExecutionPolicy Bypass -File <ENGINE_ROOT>\source\contrib\aeternum\ci\local_ci.ps1

# Tests only (already built):
powershell -NoProfile -File ...\local_ci.ps1 -SkipBuild

# Classic opcode gates only:
powershell -NoProfile -File ...\local_ci.ps1 -Filter "[classic]"
```

What it does:

1. Optional `cmake -S/-B` (`-Reconfigure`).
2. Incremental `RelWithDebInfo` build of `worldserver`, `bnetserver`, `tests`.
3. Runs `build\bin\RelWithDebInfo\tests.exe` (Catch2), writes JUnit under `_buildlogs/`.
4. Checks runtime helpers exist beside the exes (`START_SERVER.bat` … `legacy.dll`).

What it never does:

- Enable or run GitHub Actions  
- Touch `<SERVER_ROOT>` / production `auth`  
- Package or upload artifacts to a public channel  

Align with [QUALITY_CHECKLIST.md](QUALITY_CHECKLIST.md) before pushing.

## 2) Release packaging proposal (not automated yet)

Goal: a **lab/operator zip** of one RelWithDebInfo runtime folder, not an installer.

### Include

| Path (relative to `build\bin\RelWithDebInfo`) | Why |
|---|---|
| `worldserver.exe`, `bnetserver.exe`, `*.dll` (incl. `legacy.dll`, `openssl_ed25519.dll`, MySQL/OpenSSL deps already beside exes) | Binaries |
| `START_SERVER.bat`, `STOP_SERVER.bat`, `STATUS.bat`, `CHECK_CONFIG.bat` | Operator scripts |
| `README.txt`, `create_aeternum_dbs.sql`, `set_realm_name_aeternum.sql` | Setup |
| `*.conf.dist` | Config templates (**never** ship filled `.conf` with secrets) |
| `logs\` (empty) | Created at run |

### Exclude

- `.conf` with DB passwords / cert private keys / PEM chains  
- Full `DataDir` client data (document external path)  
- PDB optional (separate “debug symbols” zip if needed)  
- `_run\`, build tree, `.git`

### Suggested layout name

`aeternum-engine-<gitsha>-win64-relwithdebinfo.zip`

Contents root = the runtime folder (so extracted bats sit next to exes).

### Version metadata (proposal)

Add a generated `BUILD_INFO.txt` in a future CMake POST_BUILD with: git SHA, branch, build config, Boost/OpenSSL/MySQL versions from the README template. Not required for M9 close-out.

### Signing / distribution

Out of scope for now. Private GitHub release asset or internal share only. No Actions workflow.

## 3) Gameplay modules

Remain **proposals only** — no packaging of experimental gameplay modules in the lab zip until an ADR says otherwise.

## 4) Next implementation steps (optional)

1. Add `-Package` switch to `local_ci.ps1` that zips the runtime dir with the include/exclude rules above.  
2. Generate `BUILD_INFO.txt` from CMake.  
3. Keep Actions disabled; operators run local CI on the build machine before tagging.