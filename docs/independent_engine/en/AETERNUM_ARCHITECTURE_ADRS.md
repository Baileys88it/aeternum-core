# AETERNUM_ARCHITECTURE_ADRS (Independent Engine track)

**Status:** IE-ADR-001..012 **ACCEPTED** by owner 2026-11-05 (see OWNER_DECISIONS.md / docs/adr/IE-ADR-STATUS.md).
**Date:** 2026-11-05 (Europe/Rome)  
**Collision rule:** Existing Accepted `docs/adr/ADR-001..009` remain historical/operational. New records use **`IE-ADR-NNN`**.

## Relationship map

| Existing Accepted ADR | Topic | Relation |
|---|---|---|
| ADR-001 engine boundaries | Layering | Complemented by IE-ADR-001/002/004 |
| ADR-002 client compatibility | Single Forever profile | Preserved; IE-ADR-005 specializes protocol |
| ADR-003 module architecture | Static first | Preserved; IE-ADR-006/012 refine |
| ADR-004 module SDK | Draft direction | Intent superseded by IE-ADR-006 when accepted |
| ADR-005 threading | Map ownership | Preserved |
| ADR-006 database | MySQL SoR | Preserved; IE-ADR-007 adds ports above MySQL |
| ADR-007 observability | Metric sink | Preserved |
| ADR-008 admin monitoring pilot | Admin module | Preserved; honesty fix separate (decision #10) |
| ADR-009 dual maintenance | Trinity prod | Preserved |

---

### IE-ADR-001 — Independent Engine Direction
- **Context:** CMake `project(TrinityCore)`; product goal is Aeternum Forever engine.  
- **Problem:** Identity and architecture still Trinity-shaped; “independent” language risks over-promise.  
- **Proposed decision:** Pursue Independent Engine track; TrinityCore is foundation → future adapter; **current state is not independent**.  
- **Alternatives:** Soft-fork forever; immediate rewrite.  
- **Consequences:** IE milestones; careful wording; GPL still applies unless #1 says otherwise.  
- **Risks:** R1 marketing vs reality.  
- **Open questions:** Owner decision #1 (license goal).

### IE-ADR-002 — Modular Monolith
- **Context:** Single realm bnet+world processes.  
- **Problem:** Need modularity without distributed ops cost.  
- **Proposed decision:** Nine-domain modular monolith; no microservices by default.  
- **Alternatives:** Microservices; undifferentiated tree.  
- **Consequences:** Dependency rules + lint.  
- **Risks:** Paper architecture (R10).  
- **Open questions:** Lint tooling.

### IE-ADR-003 — TrinityCore Compatibility Boundary
- **Context:** Gameplay correctness is Trinity implementation today.  
- **Problem:** Public code includes Trinity types.  
- **Proposed decision:** `adapters/trinitycore` implements Aeternum contracts; legacy may remain until replaced.  
- **Alternatives:** Ban Trinity includes immediately (infeasible).  
- **Consequences:** Dual impl period; GPL in-process unchanged.  
- **Risks:** Adapter tax.  
- **Open questions:** When to physically move files.

### IE-ADR-004 — Public Engine Contracts
- **Context:** SDK draft host-coupled.  
- **Problem:** Modules see Player*/WorldSession*/Map*/Unit*/Creature*/WorldObject*.  
- **Proposed decision:** Forbid those types in public contracts/SDK; use IDs/PODs/handles.  
- **Alternatives:** Allow Trinity types for “friend” modules.  
- **Consequences:** Mapping code; safer API.  
- **Risks:** Incomplete façades.  
- **Open questions:** EntityId invalidation protocol.

### IE-ADR-005 — Protocol Independence
- **Context:** Profiles/ExtraInserts exist; Packets still entangled.  
- **Problem:** Client updates risk gameplay TU edits.  
- **Proposed decision:** Protocol Engine owns profiles, opcode map, codecs; gameplay consumes decoded messages.  
- **Alternatives:** Keep ClassicOpcodes entangled.  
- **Consequences:** Aligns Forever-only strategy; enables scenario 1 PASS path.  
- **Risks:** Wire regressions (mitigate goldens).  
- **Open questions:** First codec set scope; decision #9 for 0x3D013D.

### IE-ADR-006 — Module SDK and Versioning
- **Context:** v0.2 `AETERNUM_SDK_API_DRAFT`.  
- **Problem:** Draft may be treated as stable.  
- **Proposed decision:** Remain draft until Trinity-free headers; enforce sdkRange at HostDiscover before 1.0; no dynamic load yet.  
- **Alternatives:** Freeze tiny 1.0 now.  
- **Consequences:** Expectation management.  
- **Risks:** R4.  
- **Open questions:** Decision #4.

### IE-ADR-007 — Persistence Abstraction
- **Context:** Prepared statements permeate gameplay (Char ~600 CAPS lines, Hotfix ~846).  
- **Problem:** Cannot fake DB or evolve storage.  
- **Proposed decision:** Repository ports gradually; MySQL remains SoR (ADR-006).  
- **Alternatives:** Replace MySQL; do nothing.  
- **Consequences:** Long migration; vertical slices only.  
- **Risks:** Perf/TX.  
- **Open questions:** First slice (recommend CharacterSummary list).

### IE-ADR-008 — Engine Kernel and Lifecycle
- **Context:** Lifecycle in Main.cpp + bats.  
- **Problem:** No owned Kernel API.  
- **Proposed decision:** `IEngineHost` façade; bats remain UX.  
- **Alternatives:** Docs only.  
- **Consequences:** Testable boot later.  
- **Risks:** Façade without call sites.  
- **Open questions:** Admin drain interaction.

### IE-ADR-009 — Data Ownership and Content Versioning
- **Context:** Hotfixes + SQL custom + profile seams.  
- **Problem:** Content/protocol versioning mixed.  
- **Proposed decision:** Versioned content packages referenced by profiles; validators in tools.  
- **Alternatives:** SQL-only forever.  
- **Consequences:** Tooling overhead.  
- **Risks:** Process weight.  
- **Open questions:** Package format.

### IE-ADR-010 — Architecture Dependency Enforcement
- **Context:** Rules get violated without automation.  
- **Problem:** Forbidden includes regress.  
- **Proposed decision:** local_ci check on `engine/contracts` + `modules/sdk/include`.  
- **Alternatives:** Manual review only.  
- **Consequences:** Script maintenance; debt list for host/admin until M3.  
- **Risks:** False positives.  
- **Open questions:** Exact forbidden list (proposed in M1 plan).

### IE-ADR-011 — Testing and Regression Strategy
- **Context:** Catch2 strong; login manual; last green 1841/110 archived — **current green NOT VERIFIED** in M0 read-only.  
- **Problem:** Architecture changes can break Forever silently.  
- **Proposed decision:** Pyramid — unit + protocol goldens + periodic launcher checklist + soak metrics; Actions stay off unless owner changes policy.  
- **Alternatives:** Cloud CI.  
- **Consequences:** local_ci growth.  
- **Risks:** R12, R23.  
- **Open questions:** Scripted launcher login later?

### IE-ADR-012 — Runtime Module Strategy
- **Context:** Static only; WASM deferred.  
- **Problem:** Pressure for hot-load.  
- **Proposed decision:** Static through IE-M3 at least; dynamic only with security+ABI plan.  
- **Alternatives:** DLL modules now.  
- **Consequences:** Rebuild to add modules.  
- **Risks:** Slower iteration vs safer ops.  
- **Open questions:** Later appetite for dynamic.

---

## Acceptance of ADRs
Owner decision #3 recorded 2026-11-05: all IE-ADR-001..012 **ACCEPTED**.
