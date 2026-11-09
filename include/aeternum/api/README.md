# aeternum/api — frozen public contracts (SDK 1.0)

Small, consumer-backed surface only. See `docs/independent_engine/en/OWNER_DECISIONS.md`.

| Header | Consumer today |
|---|---|
| `status.hpp` | all below |
| `protocol/opcode.hpp`, `protocol/profile.hpp` | Forever remap / client profiles / goldens harness |
- `protocol/adapter.hpp` — IE-M2 Discovery skeleton (`IOpcodeAdapter`, unknown policy); not wired
| `ops/admin.hpp` | `src/modules/admin_monitoring` |
| `ops/metrics.hpp` | Metric local sink + AdminPrometheus |
| `config/snapshot.hpp` | Config.Schema / `--check-config` |
| `sdk/lifecycle.hpp` | Module host + admin module |

**Forbidden types in this tree:** `Trinity gameplay pointer types`, `Trinity gameplay pointer types`, `Trinity gameplay pointer types`, `Trinity gameplay pointer types`, `Trinity gameplay pointer types`, `Trinity gameplay pointer types`.

Draft SDK under `src/modules/sdk` is **not** this freeze; it remains draft until the host no longer includes Trinity headers.

**Related (M1 C ABI, not this freeze):** [../abi](../abi/README.md) — AeternumABI.h for Level-3 boundary.
