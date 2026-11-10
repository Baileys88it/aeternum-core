# Quality checklist (M2 proposal)

**Status:** Proposal — **not enforced** in CI (GitHub Actions remain disabled).  
**Italian:** [`../it/QUALITY_CHECKLIST.md`](../it/QUALITY_CHECKLIST.md)

Use before merging non-trivial engine changes onto `aeternum/main`.

## Before you push

1. [ ] Change is localized (prefer `forever/`, module, or single subsystem).
2. [ ] No new Forever build `#ifdef` scattered in core — belongs in profile / `src/server/game/Forever/` (M4).
3. [ ] `Classic.ClientExtraInserts` / `ServerExtraInserts` value lines have **no inline comments**.
4. [ ] If ExtraInserts or profile touched: run `python tools/config-validator/validate_extra_inserts.py --profile forever/client_profiles/<id> --conf <conf>`.
5. [ ] If opcode remap touched: Catch2 `[classic][extrainsects]` green.
6. [ ] Docs for the change exist in **EN + IT** under `docs/`.
7. [ ] No secrets, no Trinity Engine (`<SERVER_ROOT>`) writes/restarts from agents.
8. [ ] Build dir is `<ENGINE_ROOT>\build` (never Trinity’s build/install).
9. [ ] Launcher contracts unchanged unless intentionally versioned with a new profile (1119 / 8085 / build).

## Optional local static analysis (configs only in M2)

```bash
# clang-tidy (proposal config at repo root)
clang-tidy -p <ENGINE_ROOT>/build <file.cpp>

# cppcheck (proposal config)
cppcheck --project=<ENGINE_ROOT>/build/compile_commands.json --enable=warning,style --suppressions-list=.cppcheck-suppressions
```

Configs: [`.clang-tidy`](../../../.clang-tidy), [`.cppcheck-suppressions`](../../../.cppcheck-suppressions) — **proposals**, not gates.
