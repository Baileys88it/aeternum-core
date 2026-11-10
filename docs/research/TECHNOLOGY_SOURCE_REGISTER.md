# Technology Source Register (M0)

Research date: **2026-11-04**. Distinguishes **Official** / **Upstream impl** / **Community** / **Our conclusion**.

| Technology | Official sources | Latest stable verified | License (typical) | Maintenance | Our conclusion |
|---|---|---|---|---|---|
| **C++** | https://isocpp.org/ · https://en.cppreference.com/ · https://github.com/isocpp/CppCoreGuidelines | Language evolving; use standard already selected by tree/compiler | N/A (language) | Active | **Keep**. Upgrade ISO standard only with MSVC/Boost/dep proof. |
| **CMake** | https://cmake.org/documentation/ · https://cmake.org/download/ | **4.4.5** (2026-10-27); machine has **4.4.4** | BSD-3-Clause | Active (Kitware) | **Keep**. Optional bump 4.4.4→4.4.5 local later. |
| **MySQL** | https://dev.mysql.com/doc/ | **8.0.46** on machine | GPL (Community) | Active | **Keep** as gameplay SoR. |
| **MariaDB** | https://mariadb.com/kb/en/documentation/ | (not deployed here) | GPL | Active | Compatible alternative theoretically; **no migration planned**. |
| **ClickHouse** | https://github.com/ClickHouse/ClickHouse | (not evaluated in depth) | Apache-2.0 | Active | **Reject** for gameplay persistence; only revisit for analytics. |
| **Wasmtime** | https://wasmtime.dev/ · https://docs.wasmtime.dev/ · https://github.com/bytecodealliance/wasmtime | **v50.0.1** (2026-10-29) | Apache-2.0 WITH LLVM-exception | Active | **Investigate only** — isolation interesting; adopt only with demonstrated benefit vs native modules. |
| **WebAssembly** | https://webassembly.org/ | Spec living standard | — | Active | Same as Wasmtime: not “modern = better”. |
| **Prometheus** | https://prometheus.io/docs/ | (ecosystem) | Apache-2.0 | Active | Investigate if Metric/Influx insufficient. |
| **OpenTelemetry C++** | https://opentelemetry.io/docs/ · https://github.com/open-telemetry/opentelemetry-cpp | **v1.30.0** (2026-10-22) | Apache-2.0 | Active | Investigate for unified traces/metrics; overhead must be measured. |
| **Tracy** | https://github.com/wolfpld/tracy | **v0.14.1** (2026-08-22) | BSD-3-Clause | Active | Strong candidate for **dev** profiling in M3; not production-default without budget. |
| **Google Benchmark** | https://github.com/google/benchmark | (use latest stable when adopting) | Apache-2.0 | Active | Use for microbenches after baselines exist. |
| **GoogleTest** | https://google.github.io/googletest/ · releases **v1.18.0** (2026-08-10), needs C++17 | v1.18.0 | BSD-3-Clause | Active | **Do not add** unless Catch2 proven insufficient (avoid dual frameworks). |
| **Catch2** | https://github.com/catchorg/Catch2 · https://github.com/catchorg/Catch2/releases | **v3.16.1** (2026-10-08) | BSL-1.0 | Active | **Keep** — already wired in Trinity tests. |
| **ASan/TSan/UBSan** | https://clang.llvm.org/docs/AddressSanitizer.html (and Thread/UB) | Clang toolchain | LLVM | Active | Adopt in **local** M2 quality gates (MSVC limitations noted). |
| **clang-tidy** | https://clang.llvm.org/extra/clang-tidy/ | with LLVM | LLVM | Active | Gradual M2. |
| **Clang Static Analyzer** | https://clang.llvm.org/docs/ClangStaticAnalyzer.html | with LLVM | LLVM | Active | Optional M2. |
| **cppcheck** | https://cppcheck.sourceforge.io/ | (check site on adopt) | GPL | Active | Optional complementary. |
| **GitHub Actions** | https://docs.github.com/en/actions | — | — | Active | **Keep disabled** on this repo (maintainer policy). Prefer local/self-hosted later if needed. |
| **GitLab CI** | https://docs.gitlab.com/ee/ci/ | — | — | Active | Only if self-host justifies; not required for M0. |
| **Custom allocators / pools** | CppCoreGuidelines · allocator awareneness literature | N/A | N/A | N/A | **Investigate only with benchmarks**; may worsen vs CRT/jemalloc already in dep. |
| **jemalloc** | in `dep/jemalloc` | tree-vendored | BSD-like | Vendored | Already available; don’t layer more allocators blindly. |

### Legend for conclusions
- **Keep** = in use / should remain  
- **Investigate only** = interesting; adopt only with demonstrated benefit  
- **Reject** = not appropriate now for stated reason
