# IE-M2 Blocco1 — Loader Protocol Profile + AdapterOpcodeTable densa

**Milestone:** IE-M2 Blocco1 (autorizzato da maintainer t654u + scope t655u)  
**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `feat/m2-protocol` (worktree `<ENGINE_ROOT>\wt_m2`), tag base `M1-Complete` (`a52e748262`)  
**EN:** [`M2_BLOCK1.md`](M2_BLOCK1.md) · **Design:** [`PROTOCOL_ADAPTER_DESIGN.it.md`](PROTOCOL_ADAPTER_DESIGN.it.md)

Stato: solo commit locali. Nessun push, nessun deploy. **Zero cambi di comportamento**: adapter solo-boot, **non** collegato a `WorldSocket` / `WorldSession` / `Opcodes` (nessun file core toccato).

## 1. Consegnato

| Artefatto | Path |
|---|---|
| Generatore tabella opcode (documentato, deterministico) | `contrib/aeternum/protocol/gen_opcode_table.py` |
| Tabella generata (coperta da integrity) | `forever/client_profiles/1.60.1.70790/opcodes/opcodes.json` |
| Policy unknown / ignored_known (coperta da integrity) | `forever/client_profiles/1.60.1.70790/opcodes/unknown_policy.json` |
| Manifest `load_policy` + `integrity.files[]` | `forever/client_profiles/1.60.1.70790/manifest.json` |
| Hash byte-esatto su checkout Windows | `opcodes/.gitattributes` (`*.json -text`) |
| Tabella densa + classificazione + rate limiter (hot path) | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/opcode_table.hpp` |
| Loader solo-boot | `src/aeternum/protocol_adapter/include/aeternum/protocol_adapter/profile_loader.hpp` |
| Lint strutturale | `contrib/aeternum/ci/check_adapter_no_map.ps1` → target `aeternum-adapter-nomap-lint` (dipendenza di `tests`) |
| Catch2 `[m2][blocco1]` A–F | `tests/protocol_adapter/ProtocolAdapterBlocco1.cpp` |

## 2. Generazione

`python contrib/aeternum/protocol/gen_opcode_table.py --repo .` · `--check` (fallisce se opcodes.json o hash manifest non aggiornati) · `--out <file>` (dry-run).

Porta esattamente `TranslateClientOpcode` / `TranslateServerOpcode` per un build: shift 1, insert index client 0x13B, main 0x98 (= conf produzione). Conteggi 70790: cmsg 1186 righe (189 alias; 1185 nominati + 1 ignored_known), handled 610, enum 1028 (32 irraggiungibili), smsg 1410 (3 blocked, 4 collisioni wire), override `.inc` 687, ignored_known 1 (`0x3D013D` ← wire `0x3E013D`).

Regola alias: wire preferito = numerazione shiftata > override esplicito > wire più alto. AUTH_SESSION logico `0x440001` → preferito `0x460001`, alias `0x450001`.

## 3. Loader (solo boot)

Stesse regole integrity di M1: `files[]` XOR legacy, max 64, path duplicati rifiutati, path sicuri, `Sha256FileHex`. opcodes.json e unknown_policy.json devono essere coperti. Conteggi ricontrollati. Fail-closed: tabella `Reset()`, `Loaded()` false, motivo esplicito con path (`integrity_sha256_mismatch:<path>`, `integrity_file_missing:<path>`, `integrity_uncovered:<path>`, `opcodes_count_mismatch:<key>`, `opcodes_duplicate_key:<map>:0x…`, `unknown_policy_invalid:<campo>`, `load_policy_unsupported`).

## 4. Tabella densa

Vietati `std::map` / `std::unordered_map` / `std::set` (e famiglia) in `src/aeternum/protocol_adapter/**` (lint); niente heap in `opcode_table.hpp` (lint). Lookup O(1) `noexcept`:

```
key = 0xGGIIII; key > 0xFFFFFF -> non trovato
span = groups[GG] {base, minIndex, count}
off = IIII - minIndex (unsigned); off >= count -> non trovato
slot = slots[base + off]; flag VALID -> hit
```

Gestisce id grandi come `0x3D013D` perché indicizza per gruppo e alloca slot solo per la finestra `[min..max]` popolata di ogni gruppo: 16,7 M id → poche migliaia di slot contigui, senza hash né probing. Capacità fissa a compile time (4096 slot/mappa); se un profilo futuro non ci sta, il load fallisce.

Footprint misurato: `OpcodeEntry` 12 B · `DenseOpcodeMap` 70 924 B · `AdapterOpcodeTable` 283 800 B (≈ 277 KB, una sola allocazione al boot) · slot usati 2011 / 1937 / 1649 / 1652 su 4096.

## 5. Regola d'oro — ignored_known vs unknown

| Classe | Decisione | Effetti |
|---|---|---|
| `Handled` | Dispatch | contatore hit per slot |
| `KnownUnhandled` | Dispatch (decide il core come oggi) | contatore hit per slot |
| `IgnoredKnown` | **Drop** | contatore — **ZERO log, ZERO I/O, ZERO heap** |
| `Unknown` (buco, gruppo assente, key > 0xFFFFFF) | **Drop** | contatore + **ERROR rate-limited** |

Policy (`unknown_policy.json`): `window_ms` 10000, `threshold_per_window` 50, `max_errors_per_window` 1. Sotto/uguale soglia: nessun log. Sopra: al massimo N chiamate al sink ERROR per finestra, il resto conteggiato come soppresso. Sink = puntatore a funzione installato al boot; tempo iniettato (test deterministici); contatori atomici relaxed preallocati.

`0x3D013D` (dec 3998013) è un id CMSG **logico (post-traduzione)** nel buco senza nome `0x3D013C..0x3D013F`; wire derivato `0x3E013D`. Classificato `IgnoredKnown`: scartato in silenzio e contato. Ricerca: `<DOCS_ROOT>\IE_M2\RESEARCH_0x3D013D.md`.

## 6. Test `[m2][blocco1]`

A load + conteggi + sizeof · B hash corrotto/file manomesso/mancante → motivo con path · C lookup valido, inverso, alias, buchi, out-of-range · D `0x3D013D` IgnoredKnown, contatore, sink 0 chiamate su 20 000 pacchetti · E spam unknown: 0 ERROR sotto soglia, limitati sopra · F out-of-range = unknown, nessun crash.

Esito: `[m2]` **6/6 casi, 50 200 asserzioni OK**; `[m1]` **39/39 (258 asserzioni)**; suite completa 175/175 (52 416 asserzioni). Build dir `<ENGINE_ROOT>\build_m2`.

Lint: PASS; verificato FAIL (exit 1) iniettando `#include <unordered_map>` / `std::map` o `new` nell'header hot-path.

## 7. Fuori da Blocco1

Collegamento a WorldSocket (shadow + parity, Blocco2), sink reale `TC_LOG_ERROR` + metriche, sostituzione della `unordered_map` `.inc` nel core (Blocco4), flag auth da Profile (Blocco5).
