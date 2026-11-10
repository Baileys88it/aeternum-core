# Client update procedure — as done TODAY

**Status:** Partially implemented (manual process; not yet a formal Aeternum profile pipeline)

## Current practice (Forever 70755 → 70790 experience)

1. Discover new client build; extract data (`extract_*.ps1`).  
2. Update `auth.build_info` + `realmlist.gamebuild`.  
3. Adjust `Classic.ClientExtraInserts` / `ServerExtraInserts` from sniff or community PR — **must re-validate** (PR#271 70755 inserts were wrong for 70790 create).  
4. Fix packet layouts in `Server/Packets` as needed (who/party/arena precedent).  
5. Stub/ignore new empty probes.  
6. Apply hotfixes SQL / Aeternum fixes with rollbacks.  
7. Rebuild worldserver/bnetserver locally; maintainer-approved restart with 120s warning on Trinity Engine.  
8. Validate: login, **character create**, who, party, arena queue, core quests.  
9. Document NOTES under `sql/custom` or `_fixes`.

## Pain

Knowledge is scattered; mistakes fail closed as “button does nothing”.  

## Direction (not M0 design)

Isolate into client profiles + automated remap tests (priority #1 with Module SDK). See audit Forever doc.
