# Checklist qualità (proposta M2)

**Stato:** Proposta — **non enforced** in CI (Actions restano disabilitate).  
**EN:** [`../en/QUALITY_CHECKLIST.md`](../en/QUALITY_CHECKLIST.md)

1. [ ] Cambio localizzato (`forever/`, modulo, o un solo sottosistema).
2. [ ] Niente `#ifdef` di build Forever sparsi nel core → profilo / `src/server/game/Forever/` (M4).
3. [ ] Nessun commento inline sulle righe ExtraInserts.
4. [ ] Se tocchi ExtraInserts/profilo: lancia il validator Python.
5. [ ] Se tocchi remap opcode: Catch2 `[classic][extrainsects]` verde.
6. [ ] Documentazione **EN + IT**.
7. [ ] Niente secret; niente write/restart su Trinity Engine.
8. [ ] Build solo in `<ENGINE_ROOT>\build`.
9. [ ] Contratti launcher invariati salvo nuovo profilo.

Analisi statica opzionale: `.clang-tidy`, `.cppcheck-suppressions` (solo proposta).
