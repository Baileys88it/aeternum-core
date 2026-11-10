# Tabella opcode — profilo 1.60.1.70790

| Wire (gruppo client 0x44) | Dopo ExtraInserts + shift gruppo 70245 | Handler core |
|---|---|---|
| `0x440070` | `0x430070` | `CMSG_CREATE_CHARACTER` |
| `0x440073` | `0x430071` | `CMSG_CHECK_CHARACTER_NAME_AVAILABILITY` |

Insert **corretti** `0x440071,0x440072`: CREATE non viene scalato.
Insert **errati** `0x440063,0x440064`: CREATE → pet-battle (incidente 70790).
