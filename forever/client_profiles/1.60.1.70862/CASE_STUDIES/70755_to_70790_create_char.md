# Case study: 70755 → 70790 character create (2026-11-04)

| | |
|---|---|
| Broken `Classic.ClientExtraInserts` | `0x440063,0x440064` |
| Symptom | Wire `0x440070` remapped to `CMSG_PET_BATTLE_REPLACE_FRONT_PET` |
| Fix | `0x440071,0x440072` |
| Lesson | Never copy inserts across builds without sniff + remap unit test |

Regression covered by `tests/game/ClassicExtraInsertsRemap.cpp`.
