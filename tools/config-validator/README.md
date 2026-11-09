# Config validator (ExtraInserts + client profile)

Standalone Python 3 tool (stdlib only). Validates:

1. `Classic.ClientExtraInserts` / `Classic.ServerExtraInserts` value format
2. No inline comments on the value line
3. Values match the active profile manifest under `forever/client_profiles/`

## Usage

```bash
python tools/config-validator/validate_extra_inserts.py \
  --conf path/to/worldserver.conf \
  --profile forever/client_profiles/1.60.1.70790
```

Exit `0` = OK, `1` = validation errors, `2` = usage/IO errors.

## C++ startup check

**Deferred to M4.** A worldserver log-only check would need to re-parse conf after `sConfigMgr` load and compare to a profile path; M2 keeps risk zero by shipping the offline tool + unit tests only. Justification: ExtraInserts are already parsed once via static locals in `ClassicOpcodes.cpp`; a second reader in M2 without the profile loader would duplicate policy and invite drift.

Italian: [`README.it.md`](README.it.md)
