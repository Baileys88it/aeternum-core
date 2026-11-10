# Scripting & hooks (ScriptMgr)

**Status:** Implemented

`ScriptMgr` registers C++ scripts and exposes hooks used across gameplay (accounts, achievements, AI, maps, …). This is the closest existing “module” boundary — but scripts still compile into the server (static), not a versioned SDK.

**Sources:** `Scripting/ScriptMgr.*`, `src/server/scripts`.
