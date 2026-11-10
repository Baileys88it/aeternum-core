# Build Environment (M0)

Observed on maintainer’s Windows machine (Trinity Engine build tree `<SERVER_ROOT>\build`).

| Item | Value | Source |
|---|---|---|
| OS | Windows (DESKTOP-BGTAC5H) | machine |
| CMake | **4.4.4** (PATH); latest stable upstream at research time **4.4.5** | `cmake --version` / cmake.org |
| Generator | Visual Studio 17 2022 | CMakeCache |
| Boost | 1.84.0 (`C:/local/boost_1_84_0`) | CMakeCache |
| OpenSSL | Win64 install path in cache | CMakeCache |
| MySQL | 8.0.46 client/libs | mysql --version |
| Targets | worldserver, bnetserver, extractors | `(operator files, not included)` |
| Build logs | `<SERVER_ROOT>\_buildlogs` | exists |
| Warnings-as-errors | OFF | CMakeCache |

**Policy:** no GitHub Actions builds on Aeternum. Local reproducible scripts only. `.github/workflows` removed from this repo tree on M0 import.
