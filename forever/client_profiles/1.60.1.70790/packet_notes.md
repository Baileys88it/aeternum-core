# Packet notes — 1.60.1.70790

- Opcode insert set differs from 70755: do **not** reuse `0x440063,0x440064`.
- Character-create path: sniff confirmed CREATE wire `0x440070`, CHECK `0x440073`; inserts sit between them.
- Packet layout deltas vs 70707/70755 for create: follow Trinity NOTES under Aeternum fixes when porting; adapters will live in `src/server/game/Forever/` (M4).
- No runtime code in this folder yet (M2 data-only).

IT: `packet_notes.it.md`
