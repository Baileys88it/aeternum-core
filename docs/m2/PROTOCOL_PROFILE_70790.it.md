# Mappa Protocol Profile — client Forever 1.60.1.70790

**Milestone:** IE-M2 Discovery (branch `feat/m2-protocol`)  
**Data:** 2026-11-05 (Europe/Rome)  
**Worktree:** `<ENGINE_ROOT>\wt_m2` @ `545d9a2c93`  
**EN:** [`PROTOCOL_PROFILE_70790.md`](PROTOCOL_PROFILE_70790.md)

Inventario di dove il protocollo Forever è hardcoded oggi, quantificazione degli opcode e proposta del pacchetto Profile on-disk per il build **70790**. Nessun cambio di comportamento su produzione/Trinity in questa Discovery.

## Profilo attivo

- `ACTIVE` → `1.60.1.70790`
- ExtraInserts: Client `0x440071,0x440072`; Server `0x470125`
- Gate: CREATE `0x440070`→`0x430070`; CHECK NAME `0x440073`→`0x430071`

## Inventario hardcoding (sintesi cite)

| Area | Dove | Note |
|---|---|---|
| Enum CMSG/SMSG | `Opcodes.h` | 1032 CMSG, 1411 SMSG |
| Handler table | `Opcodes.cpp` Initialize | Dense array 1984 / 1671 |
| Translate Forever | `ClassicOpcodes.cpp:160+` | ExtraInserts + group shift + mappa 70245 |
| Remap puro | `Forever/ClassicOpcodeRemap.h` | Condiviso Catch2/adapter |
| `.inc` wire→core | `ClassicClientOpcodes.inc` | 687 entry |
| Recv/Send | `WorldSocket.cpp:412-413` / `:539+` | Translate* |
| Skip auth key | `WorldSocket.cpp:747-761` | Obbligatorio su 70790 |
| RegionGroup | `WorldSocket.cpp:968-969`, `1054-1055` | Ops = 8 |
| Layout gates | `UpdateFields.cpp` (13), Packets (2) | `IsBuild70291/70425` |

## Quantità (HEAD `545d9a2c93`)

| Metrica | N |
|---:|---:|
| DEFINE_HANDLER CMSG | 1032 |
| STATUS_UNHANDLED CMSG | 380 (~37%) |
| Active CMSG (LOGGEDIN+AUTHED+…) | 628 |
| DEFINE_SERVER SMSG | 1410 |
| ClassicClientOpcodes.inc | 687 |
| Unknown noto | `0x3D013D` |

Delta Forever vs Trinity retail: non è un `git diff` pulito (snapshot Forever). Catalogo = `.inc` + ExtraInserts + gate `IsBuild*` + knobs auth.

## Struttura Profile proposta

Vedi EN §4: `opcodes/`, `handshake/flags.json`, `layouts/gates.json`, `integrity.files[]` con SHA-256 come M1, `load_policy=boot_only`, un solo profilo attivo.

## Riferimenti

- Design Adapter: [`PROTOCOL_ADAPTER_DESIGN.it.md`](PROTOCOL_ADAPTER_DESIGN.it.md)
- ADR: [`../adr/it/IE-ADR-013-protocol-profile-and-adapter.md`](../adr/it/IE-ADR-013-protocol-profile-and-adapter.md)
