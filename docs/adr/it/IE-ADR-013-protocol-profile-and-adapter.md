# IE-ADR-013: Pacchetto Protocol Profile + seat Protocol Adapter

**Stato:** Proposta (IE-M2 Discovery) — in attesa di accettazione maintainer  
**Data:** 2026-11-05 (Europe/Rome)  
**EN:** [`../IE-ADR-013-protocol-profile-and-adapter.md`](../IE-ADR-013-protocol-profile-and-adapter.md)

## Problema

Id wire Forever, ExtraInserts, group-shift, knobs auth e fork `IsBuild*` sono sparsi in core/conf. Un bump client rischia remap silenziosi e review globale.

## Decisione

1. **Protocol Profile** on-disk come source of truth (JSON + SHA-256 M1, boot-only, un solo ACTIVE).
2. **Protocol Adapter** unico seat `Wire ↔ Core` su WorldSocket; handler sul core logico.
3. Hot path dense / zero heap.
4. Unknown: log-once + metric + drop.
5. Migrazione a blocchi M2 + goldens; strangler.

## Conseguenze

Nuovo build Forever = nuova cartella profile + gate verdi + restart. `ClassicOpcodes.cpp` diventa dettaglio Adapter. Conf resta override in transizione. Deploy solo dopo QA isolata e ok maintainer.
