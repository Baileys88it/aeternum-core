# Module Candidates (M0)

**Status:** Planned (candidates only — no module framework yet)

| Candidate | Today’s anchor | Notes |
|---|---|---|
| C++ content scripts | `src/server/scripts` | De facto static modules |
| SmartAI content | DB `smart_scripts` | Data-driven |
| AuctionHouseBot | `game/AuctionHouseBot` | Optional gameplay |
| Playerbots (karrz lines) | upstream `karrz-bots*` branches | **Not** merged wholesale; evaluate later |
| OutdoorPvP / events | OutdoorPvP, event_scripts | Partially disabled (Silithus) |
| Admin/GM commands | Chat commands (e.g. `.online`) | |
| Monitoring | Metric (disabled) | |
| Forever compatibility pack | ClassicOpcodes + inserts + packets | **Should become first-class profile**, not a “module” that can be omitted |
| World events / custom quests / economy | mostly absent as framework | M7+ |

**SDK note:** Module SDK is priority #1 *with* client compatibility. Static modules first; native dynamic later; WASM only with proven isolation benefit.
