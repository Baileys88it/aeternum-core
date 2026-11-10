# Golden packet corpus — Forever profile 1.60.1.70790

## Purpose

Prove **behavior parity** for the Forever protocol boundary while the Independent Engine track extracts Protocol behind `aeternum/api/protocol/*`.

## What lives here

| Path | Role |
|---|---|
| `corpus.manifest.json` | Index of fixtures, profile id, build |
| `samples/*.jsonl` | One JSON object per line: wire → expect_core (+ inserts) |
| (future) `captures/` | Sanitized live captures from **Aeternum only** (never Trinity production) |

## Fixture line format (JSONL)

```json
{"id":"create_ok","wire":"0x440070","expect_core":"0x430070","inserts":["0x440071","0x440072"]}
```

Payload bytes are optional in IE-M1; remap parity is the first gate. Typed codecs add `payload_b64` later.

## Capture policy (owner decision #9)

- Live sniff/remap of unknown wire (e.g. `0x3D013D`) is allowed **on Aeternum only**.
- Never capture against Trinity production ports/processes.
- Strip account names, tokens, and personal data before committing.

## Harness

Catch2: `tests/protocol_goldens/GoldenCorpusHarness.cpp` (tag `[protocol][goldens]`).
