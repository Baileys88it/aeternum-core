# Technical Risks (M0)

| ID | Risk | Severity | Mitigation direction |
|---|---|---|---|
| R1 | Forever client build shifts opcodes; wrong ExtraInserts silently break features | **Critical** | Profile isolation + wire↔handler tests (see create-char case study) |
| R2 | Dual maintenance Aeternum vs Trinity Engine drift | High | Clear promote path; short dual period decisions |
| R3 | Refactor urge breaking launcher contracts (1119/8085/70790) | High | Contract tests; freeze external ports/build |
| R4 | Adopting fashionable tech (WASM/allocators) without proof | High | modern≠better rule |
| R5 | Residual DBErrors / incomplete Forever data | Medium | Continue triage; don’t delete Classic portals blindly |
| R6 | Observability off → slow incident response | Medium | Enable Metric or alternative with overhead budget |
| R7 | Conf parser pitfalls (BOM; comments on value lines) | Medium | Conf write discipline UTF-8 no BOM; comments separate |
| R8 | Pushing to advocaite / enabling Actions | Medium | Policy + Actions disabled API |
| R9 | Native hot-unload modules UAF | High if attempted early | Not in early milestones |
| R10 | Secrets in repo | Critical if occurs | Never commit conf with DatabaseInfo / credenziali |

Rollback culture already present for SQL fixes — extend to binaries/conf profiles.
