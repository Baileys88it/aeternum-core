# M1 Kernel — Block 4 (D10 SHA-256 + admin lifecycle)

**Branch:** `feat/m1-kernel` (local commits; push only when maintainer authorizes)  
**Scope:** Close out M1 foundation integrity + ops introspection. No hot-reload. `WITH_DYNAMIC_LINKING=OFF`.

## Deliverables

| Item | Path | Notes |
|---|---|---|
| File SHA-256 (D10) | `sha256_file.hpp` + `manifest_gate.hpp` | OpenSSL `EVP_sha256` (same primitive as `Trinity::Crypto::SHA256`); Trinity-free header |
| Manifest fields | `manifest.hpp` | `integrity.sha256` (64 hex) + `integrity.file` (relative allowlist path under `<manifestDir>/<id>/`) |
| Admin HTTP | `AdminHttpServer.cpp` | `GET /modules/lifecycle` — read-only JSON from `ListModuleLifecycle()` |
| Catch2 | `ManifestGate.cpp`, `ManifestParser.cpp` | match / mismatch / missing file / unsafe path |
| This doc | `M1_KERNEL_BLOCK4.md` | |

## D10 — how the hash is computed and compared

1. Parser accepts optional `integrity` object. If `sha256` is present it must be exactly 64 hex characters (`AETERNUM_ERR_INTEGRITY` otherwise). Optional sibling `file` is a non-empty string.
2. **Missing `integrity.sha256`:** unchanged M1 policy — no hash check; do **not** flip `RequireManifest`.
3. **When `sha256` is present (fail-closed):**
   - `integrity.file` is **required** and must pass `IsSafeIntegrityRelativePath` (no `..`, no absolute, no `\`, safe segment charset).
   - Host reads `<manifestDir>/<moduleId>/<integrity.file>` (boot-only I/O).
   - Streams the file through OpenSSL `EVP_Digest*` SHA-256 (max 256 MiB) → lowercase hex.
   - Compares to manifest digest with case-insensitive nibble equality (`Sha256HexEqual`).
   - Mismatch / missing file / I/O / unsafe path → `ManifestGateOutcome::Rejected`, `AETERNUM_ERR_INTEGRITY`, FSM **Error**, module not Started (critical → abort boot via existing gate).
4. Static Level 1 has no separate DLL: the allowlisted **file** is whatever ops puts next to the manifest (payload, config blob, future `.bin`). Authenticode remains OPTIONAL FUTURE.

## Admin — `GET /modules/lifecycle`

| | |
|---|---|
| Bind | Existing admin loopback (default `127.0.0.1`) |
| Auth | Bearer / `X-Admin-Token` — same dual-token path as `/uptime`; requires `admin.read` (ReadOnly or Operator) |
| Anon | **No** (unlike `/health`) |
| Body | `{"modules":[{"id":"...","critical":bool,"state":"Discovered\|Validated\|Loaded\|Active\|Error","reason":"..."}]}` |
| Mutating | No — snapshot only; no enable/disable |

### Manual verify (when an isolata/admin instance is up)

```text
curl -s -H "Authorization: Bearer $ADMIN_TOKEN" http://127.0.0.1:<adminPort>/modules/lifecycle
```

Expect `state":"Active"` for `admin.monitoring` after a clean boot. Do **not** point this at `_public_iem1` unless maintainer authorizes.

## Out of scope (this block)

M2 protocol / DBUpdater, hot-reload, Admin UI enable/disable, Authenticode, full M1-lint extension over `src/modules/**` (candidate **Blocco5**), MapUpdate.Threads doc (defer if not cheap).

## Residual — done in Blocco5

Multi-file integrity + ABI STL lint + STOP_TESTINST template: see [M1_KERNEL_BLOCK5.md](M1_KERNEL_BLOCK5.md). Trinity-type forbidden-includes expansion over all of `src/modules/**` remains optional follow-up (host SDK STL is intentional).
