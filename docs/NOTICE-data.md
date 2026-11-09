# Third-party data notices

Some world-database fixes shipped with or derived for this engine contain data reconstructed from these sources:

- **ForeverDB.net** — ForeverDB v0.54.8, data 2026-10-31, build 70707, GPL-3.0.
  Data from ForeverDB.net (https://foreverdb.net)
  Used for: NPC lists and positions, vendor sold lists, dungeon boss lists.
- **Efficient Games (EG Link)** — Data from Efficient Games (https://efficient.games), licensed under CC BY 4.0.
  Used for: quest fields/objectives/rewards, NPC names/subnames/displays/levels/health multipliers, encounter positions.
- **QuestieDB** (Questie / QuestieDB), GPL-3.0 — quest levels, starters/enders, prerequisites.
- **Forever Item Finder** v1.2.0, GPL-3.0 (itself crediting QuestieDB and CMaNGOS) — NPC/object map pins and drop rates flagged as real Forever rates.
- **RareNet** v0.8.15 — only the `ns.ForeverReports` drop/kill counts (chance = drops/kills*100).
  The ForeverChanges spawn/loot data was NOT used, as its NOTICE requires.
- Client DB2 data (UiMapAssignment, QuestMoneyReward, Item, ItemExtendedCost, FactionTemplate) via GrunzForever (MIT).
- **Karrz1/TrinityCore** (branch `forever`), GPL-2.0 — selected world data from the "Sync with WoW Forever 0.1.34" commit.

Coordinates: zone% -> world through UiMapAssignment; Z taken from the server mmaps (nearest navmesh polygon of the main
component, within 2 yd of the pin; for the instance maps 2998/2959 a vertical column query of the navmesh, only where the
main component had a single floor).
