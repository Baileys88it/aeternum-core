# Registro rischi Independent Engine

> Versione italiana. EN: [`../en/AETERNUM_RISK_REGISTER.md`](../en/AETERNUM_RISK_REGISTER.md)

**Data:** 2026-11-05 (Europe/Rome)

| ID | Rischio | Severità | Mitigazione |
|---|---|---|---|
| R1 | Messaging «indipendente» vs realtà GPL | Alta | Wording obiettivo; decisione licenza registrata |
| R2 | Breakage Forever su estrazione protocol | Alta | Goldens + adapter thin + soak |
| R3 | Drift dual-tree / dual maintenance | Alta | Trinity prod freeze finché ok owner |
| R4 | SDK draft trattato come stabile | Media | Freeze `aeternum/api` 1.0; draft host esplicito |
| R5 | Gap opcode (380 UNHANDLED, wire ignoto) | Alta | Coverage + sniff Aeternum-only schedulato |
| R6 | README non onesto | Media | **Chiuso** in IE-M1 |
| R7 | Soak incompleto / contention build | Media | `build_iem1` separato; non toccare binari soak |
| R21 | Inventario licenze `dep/*` incompleto | Media | Backlog dedicato |
| R22 | Cert/LoginREST operativi | Alta | Nessun cambio cert in M1; runbook separato |
| R23 | Mancanza corpus golden | Media | **Harness avviato** IE-M1; capture live dopo |

## Criterio di accettazione rischio
Nessun rischio CRITICAL senza owner awareness. Nuovi rischi si aggiungono con ID, non si silenziano.
