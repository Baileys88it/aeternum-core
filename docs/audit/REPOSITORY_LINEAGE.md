# Repository Lineage (M0)

## Fork chain (evidence-based)

```text
TrinityCore upstream (historical)
        │
        ▼
advocaite/TrinityCore  (origin on live tree)
  branches: forever (~70658), forever-bots, karrz-* feature branches
  PR #271 (open as of audit): opcode inserts toward 70755
        │
        ▼
<owner>/aeternum-server  (remote "mine" on live)
  branches: wf-70707 (snapshots), main, …
        │
        ▼
Local <SERVER_ROOT>\TrinityCore
  working branch wf-70707-karrz @ 1e356a185f (2026-11-04)
  snapshots pushed to mine/wf-70707 (latest 113226264c)
        │
        ▼
<owner>/aeternum-engine @ aeternum/main
  based on snapshot 113226264c
  GitHub Actions disabled
```

## Snapshot chosen for Aeternum

| Field | Value |
|---|---|
| Commit | `113226264ce0c299f6d5ca7181851281670f2ab2` |
| Message | snapshot: wf-70707-karrz 1e356a185f (dberr_all5 …) 2026-11-04 |
| Points at | `1e356a185f` content |

## Live tree uncommitted (read-only observation at M0)

On Trinity Engine working tree (NOT copied into Aeternum snapshot):

- Modified: vendor forever SQL/rollback, dberr_all5 SQL/README/rollback, `build_launcher_everforge.ps1`
- Untracked: `spelldesc.pkl`, `spellcache.pkl`, launcher bak, `__pycache__/`

Aeternum clone intentionally starts clean at the snapshot; pkl/cache/secrets are excluded from policy.

## Remotes on Aeternum clone

- `origin` → `<owner>/aeternum-engine`
- `upstream-Aeternum` → `<owner>/aeternum-server`
- `upstream-advocaite` → `advocaite/TrinityCore` (fetch reference only; **never push**)

## Diff vs advocaite master (live merge-base)

From live repo: merge-base `HEAD` vs `origin/master` = `85d3c25180…`; shortstat ≈ **1292 files**, +1.4M / −1.6k lines (Forever + Aeternum content dwarfs master). Treat as “Forever tree is a long-lived divergent line,” not a small patch set.
