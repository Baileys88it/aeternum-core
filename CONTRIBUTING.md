# Contributing to the Aeternum Engine

Thanks for helping. Bug reports, SQL fixes and C++ fixes are all welcome.

The `main` branch is protected, the same way TrinityCore works:

- nobody pushes to `main` directly;
- every change arrives through a pull request;
- a pull request needs at least one approving review from a maintainer, and every review conversation must be resolved before it can be merged;
- force pushes and branch deletion are blocked on `main`.

## How to send a change

1. **Fork** the repository with the GitHub "Fork" button.
2. **Clone** your fork and create a **branch** for one fix:
   ```
   git clone https://github.com/<you>/aeternum-core.git
   cd aeternum-core
   git checkout -b fix/short-description
   ```
   Use one branch per fix, so the next fix does not have to wait for the first one to be merged.
3. **Commit** with a clear title, for example `DB/Quest: fix A Lesson in Divinity (94438)` or `Core/Spells: ...`.
4. **Push** the branch to your fork: `git push origin fix/short-description`.
5. Open a **pull request** against `main` and fill in the template (what changes, how it was tested, the client build).

## Rules for changes

- **Keep the engine neutral.** Do not commit server-specific settings: realm names or ids, IP addresses, hostnames, passwords, tokens, keys, drive-letter or user-profile paths, website URLs, or custom rates. Configuration defaults in `*.conf.dist` stay at retail values (rates `1`) and every new option ships **disabled**.
- **Never invent data.** SQL fixes must cite their source (sniff, wago.tools, ForeverDB, Wowhead, VMaNGOS, ...) in a comment.
- **SQL file naming:** `sql/custom/<database>/YYYY_MM_DD_NN_<database>_<short_name>.sql`, where `NN` is the index of the file for that day and database. Ship a matching `_rollback.sql` when the change is not trivially reversible. While the pull request is open you may use an impossible date (for example `2099_13_32_00_world_...`) to avoid name clashes; it is renamed on merge.
- **C++:** follow the existing TrinityCore code style. The project must build on Windows (MSVC) and Linux (GCC/Clang).
- **Client build:** say which Forever client build you tested with (for example `1.60.1.70862`).
- **License:** by contributing you agree that your contribution is licensed under GPL-2.0, like the rest of the project.

## Reporting a bug

Open an issue with the "Generic issue" or "SQL fix" form. Include:

- the commit hash you run;
- the client build;
- the ids of the affected creatures, items, quests or spells, with a link to a reference page;
- steps to reproduce, and what you expected instead.

For a crash, attach a backtrace from a Debug or RelWithDebInfo build.

Security problems must **not** be reported in public issues: see [SECURITY.md](SECURITY.md).

## Neutrality guard

This repository is a generic engine tree. Do not commit anything that identifies a
particular server: hostnames, realm names, IP addresses, local drive or user-profile
paths, personal names or e-mail addresses.

`contrib/neutrality/check_neutrality.py` enforces this. It runs in CI
(`.github/workflows/neutrality.yml`) on every push to `main` and every pull request.
Enable the same check locally once per clone:

```sh
git config core.hooksPath .githooks
```

The `pre-commit` hook checks staged files and `pre-push` checks the whole tree.
Upstream or third-party files that must stay byte-identical are listed in
`.github/neutrality-allow.txt`.
