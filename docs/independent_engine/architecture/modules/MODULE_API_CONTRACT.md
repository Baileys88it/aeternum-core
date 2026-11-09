# Module API Contract

**Fase:** M0 | **Data:** 2026-11-05 (Europe/Rome)  
**Regola d'oro Decision #6 + owner #1:** header in `include/aeternum/api/` solo con consumer reale; no `Player*`/`Unit*`/…  
**D2/D11 DECISIONE APPROVATA 2026-11-05:** C ABI per dinamico; **no exception cross-boundary**.

---

## 0. Decisioni confine

| Tema | Regola |
|---|---|
| Level 1 static | C++ `aeternum/api` OK nello stesso CRT |
| Level 3 / DLL | Solo C (`DYNAMIC_LOADING` §9.1) |
| Eccezioni | **Mai** attraverso confine (D11); wrapper → `AeternumStatus` |
| STL cross-DLL | Vietato (D3) |
| Event MOVE/aura | Vietato fino M3 (D4) |
| loadPolicy | default `on-boot-only` (D1); manifest boot M1 (D9) |

---

## 1. Contratti ESISTENTI (SDK 1.0)

| Header | Consumer |
|---|---|
| `status.hpp` | tutti |
| `sdk/lifecycle.hpp` | ModuleHost, admin |
| `ops/admin.hpp` `ops/metrics.hpp` `ops/runtime.hpp` `ops/services.hpp` | admin / host |
| `config/snapshot.hpp` | `--check-config` |
| `protocol/opcode.hpp` `profile.hpp` | Forever adapter, goldens |

Lint: `contrib/aeternum/ci/check_forbidden_includes.ps1`.

---

## 2. Pattern servizi (ESISTENTE)

```cpp
aeternum::ops::HostServices& aeternum::ops::Services();
void aeternum::ops::Install(HostServices ports);
using ConfigReloadHook = void (*)(bool); // già C
```

Inter-modulo: solo servizi pubblicati, mai header privati altrui.

---

## 3. Event Bus (PROPOSTO — vincolato)

| Regola | Stato |
|---|---|
| Zero-subscriber fast path atomico | Obbligatorio quando esisterà |
| MOVE/aura | **VIETATO fino M3** (D4) |
| Alloc in dispatch hot | Vietato (P-c se su packet path) |
| Exception da subscriber | Catturata (D11 spirit) → fault counter |

---

## 4. Handle opachi (PROPOSTO)

`AeternumPlayerHandle` / `AeternumSessionHandle` — solo dopo World extraction. Lifetime: generation counter host; stale → `NOT_FOUND`.

---

## 5. Mapping C++ API → C (sintesi)

| C++ oggi | C |
|---|---|
| `Status` / `Result<T>` | `AeternumStatus` + out-params |
| `IModule` virtual | `AeternumModuleFT` |
| `std::string` return | `AeternumOutBuffer` |
| `ConfigReloadHook` | invariato |

Versione: `kApiVersionMajor/Minor/Patch = 1.0.0` in `lifecycle.hpp`.  
ABI C: `AETERNUM_MODULE_ABI_VERSION` separato.

---

## 6. Verifica contratto

| Check | Mezzo |
|---|---|
| No Trinity types in api/ | CI script |
| No exception escape su entry C | Catch2 M2 |
| Consumer reale | review Decision #6 |

## Registro revisioni

| Data | Modifica |
|---|---|
| 2026-11-05 | Pass 1 |
| 2026-11-05 | Block 3: D11, mapping C, event vincoli |
| 2026-11-05 | M1-Blocco1: live C ABI at include/aeternum/abi/AeternumABI.h |
