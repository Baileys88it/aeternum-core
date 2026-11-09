# Corpus pacchetti golden — profilo Forever 1.60.1.70790

## Scopo

Dimostrare la **parità di comportamento** del boundary di protocollo Forever mentre la traccia Independent Engine estrae Protocol dietro `aeternum/api/protocol/*`.

## Contenuto

| Percorso | Ruolo |
|---|---|
| `corpus.manifest.json` | Indice fixture, profile id, build |
| `samples/*.jsonl` | Un oggetto JSON per riga: wire → expect_core (+ inserts) |
| (futuro) `captures/` | Catture live sanificate **solo da Aeternum** (mai Trinity produzione) |

## Policy cattura (decisione owner #9)

- Sniff/remap di wire sconosciuti (es. `0x3D013D`) solo su **Aeternum**.
- Mai su porte/processi Trinity di produzione.
- Rimuovere account, token e dati personali prima del commit.

## Harness

Catch2: `tests/protocol_goldens/GoldenCorpusHarness.cpp` (tag `[protocol][goldens]`).
