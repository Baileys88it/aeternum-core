# Opcode expectations — profile 1.60.1.70790

| Wire (client main group 0x44) | After RemoveClientExtraInserts + 70245 group-1 | Core handler |
|---|---|---|
| `0x440070` | `0x430070` | `CMSG_CREATE_CHARACTER` |
| `0x440073` | `0x430071` | `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY` |

With **correct** inserts `0x440071,0x440072` (both > CREATE wire): CREATE is unchanged by insert subtract.

With **wrong** inserts `0x440063,0x440064` (both < CREATE): CREATE wire `0x440070` → `0x44006E` → core `0x43006E` = `CMSG_PET_BATTLE_REPLACE_FRONT_PET` (70790 incident).

IT: vedi `opcodes_table.it.md`.
