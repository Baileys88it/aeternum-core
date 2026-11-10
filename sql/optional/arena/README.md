# Optional custom arena: 1v1 / 2v2 / 3v3 on Mak'gora Arena (map 2995)

Code: `src/server/scripts/Custom/Classic/world/wf_arena.cpp` (plus `RBAC.h`, `classic_script_loader.cpp`,
`worldserver.conf.dist`). Core lines touched by the feature are marked `// Aeternum arena`.

The feature is **off by default** (`Aeternum.Arena.Enable = 0`). The SQL in this folder is **not** applied by the
database updater; apply it by hand only if you want the arena.

## Order of application (after the build)

| # | File | DB | Notes |
|---|------|----|-------|
| 1 | `2026_10_28_00_characters_arena.sql` | characters | `custom_arena_*` tables (without them the script disables itself) |
| 2 | `2026_10_28_01_hotfixes_arena_map.sql` | hotfixes | Map 2995 InstanceType 4 + PVPDifficulty 1757 (the hotfix_data row for the client is commented out) |
| 3 | `2026_10_28_02_world_arena.sql` | world | provisional safe locations, battleground_template 1169, battleground_scripts, core season 32, NPC 9500000 |
| 4 | `2026_10_28_03_auth_arena_rbac.sql` | auth | permissions 1100 (players) and 1101 (admins) |
| 5 | `2026_10_28_04_world_arena_npc_spawns.sql` | world | 7 Arena Master spawns (Orgrimmar, Stormwind, Ironforge, Booty Bay, Undercity, Thunder Bluff, Darnassus) next to the battlemasters, checked against the navmesh |
| 6 | `2026_10_28_05_world_arena_start_locs.sql` | world | Z of the start points 1105000/1105001 placed on the navmesh |
| 7 | `2026_10_29_00_hotfixes_arena_pvp_difficulty_minlevel1.sql` | hotfixes | PvpDifficulty 1757 MinLevel 10 -> 1 |
| 8 | `2026_10_29_01_characters_arena_new_season.sql` | characters | optional: closes the running season without rankings or titles and opens the next one (worldserver stopped) |

Apply SQL 2 and 3 with the worldserver stopped (hotfixes and templates are read only at startup).

## Runtime configuration (`worldserver.conf`)

The `.dist` default of `Enable` is 0. To use the arena, set for example:

```
Aeternum.Arena.Enable = 1
Aeternum.Arena.UpdateInterval = 5000
Aeternum.Arena.MinLevel = 10
Aeternum.Arena.InviteTimeout = 60
Aeternum.Arena.Rating.Start = 1500
Aeternum.Arena.Rating.K = 32
Aeternum.Arena.Matchmaking.InitialWindow = 100
Aeternum.Arena.Matchmaking.WindowStep = 50
Aeternum.Arena.Matchmaking.WindowStepSeconds = 30
Aeternum.Arena.Matchmaking.MaxWindow = 1000
Aeternum.Arena.Leash.Radius = 60
Aeternum.Arena.Honor.RatedWin = 50
Aeternum.Arena.Honor.UnratedWin = 15
Aeternum.Arena.Honor.Loss = 0
Aeternum.Arena.Season.TopN = 3
Aeternum.Arena.Season.MinGames = 10
Aeternum.Arena.Season.TitleRank1 = 1352
Aeternum.Arena.Season.TitleTopN = 1330
```

Also set `Arena.ArenaSeason.ID = 32` and `Arena.ArenaSeason.InProgress = 1`: SQL 3 creates game event 200 for season 32,
which removes the error `ArenaSeason (32) must be an existing Arena Season`.

## Commands

| Command | Permission | What it does |
|---------|------------|--------------|
| `.arena queue 1v1\|2v2\|3v3 [skirmish]` | 1100 | rated queue (default, every member level 60) or skirmish (from level 1, at most 3 levels apart across all players of the match); 2v2/3v3 only for the group leader, with a group of the exact size |
| `.arena leave` | 1100 | leaves the queue (whole group) or the running arena |
| `.arena rating` | 1100 | season rating, wins and losses for the 3 brackets |
| `.arena top [1v1\|2v2\|3v3]` | 1100 | top 10 (without an argument: all 3) |
| `.arena season end` | 1101 | ends the season: titles for the top N of each bracket (at least MinGames matches), stores the ranking in `custom_arena_season_result`, resets ratings, opens the next season |

The core commands `.arena create/disband/rename/captain/info/lookup` (cs_arena.cpp) stay available.

## Invitation and cancelled matches

There is no accept popup: when the queue finds an opponent, players are moved into the arena at once
(1 minute of preparation, doors closed). These count as a **decline**:

- not arriving in the arena within `InviteTimeout` seconds (logout or disconnect while loading, failed teleport);
- leaving the arena before the doors open (Leave Arena button, `.arena leave`, logout).

In those cases the match is **cancelled**, rated or skirmish:

- no win or loss, no rating change and no honor for anyone;
- whoever is already in the arena (including late arrivals) returns to where they queued;
- players or groups in which everyone accepted go back into the queue with their original queue time
  (same priority and same rating window);
- the player who declined and their group are removed from the queue (their party members are invited to queue again);
- everyone receives a system message with the reason.

The match is also cancelled if a whole side is still missing when the doors open (the core would otherwise end it
with a walkover). Known limit: a player who disconnects *inside* the arena stays in the match (core offline rule)
and cancels nothing; after the doors open, leaving counts as a loss.

The code is in `wf_arena.cpp` (`UpdateMatches`, `HandleCancelLocked`, `CheckPrepLeavers`, `OnMatchEnd`).

## Levels

- Rated: only if ALL members are at level `Aeternum.Arena.Rated.Level` (60); checked when queueing and on every
  matchmaking pass. The Arena Master shows the rated options only to level 60 characters.
- Skirmish: from level `Aeternum.Arena.MinLevel` (1); a match is created only if the highest minus the lowest level
  of ALL players on both sides is <= `Aeternum.Arena.Skirmish.MaxLevelDiff` (3), using live levels on every pass.
  A group with an internal spread above 3 cannot queue; if a level-up while queued pushes it over, the group
  leaves the queue with a message.
