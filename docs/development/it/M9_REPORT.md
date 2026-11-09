# Report M9 -- Harden ops + evidenza path metric store

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Commit:** `67ebac5702` -- *M9: ops harden + metric store path evidence* (30 file, +1013/-65)  
**EN:** [`../en/M9_REPORT.md`](../en/M9_REPORT.md)

## Mapping

MASTER M9 = Release engineering (packaging: solo parziale). Sequenza standing: crash budget, schema/`--check-config`, health esteso, drain operator opzionale, fix evidenza path metriche.

## Fix path metric store (falso allarme M8)

I file erano in `_run/logs/metrics`. Soak M9: PRE 907 file / 1 830 168 B; T20 store **file_count=1062** / **bytes=2143304** = match disco; live solo `10s`/`1m`/`latest_report` (niente `1h`/`1d`/`daily_` in ~22 min).

## Consegnato

Crash dump budget + marker Catch2; schema config + `--check-config` OK; `/health` con DB/script/moduli; drain disabilitato di default, token operator distinto, min 120s, announce IT+EN, audit.

## Build / test

Catch2: **150 assertion / 23 case PASS**; smoke aggregator 7228 us / 200k sample; `--check-config` OK (0 failure required).

## Soak / produzione

Countdown 120s IT+EN (`STOP_PROD_DONE 05:36:07`). Soak **21,79 min**: bnet **21048**, world **6040**; health scripts=5909; drain readonly 403 / operator 200. Prod: bnet **20904**, world **23048**.

## Non verificato

Login client reale; soak 72h; file metriche `1h`/`1d`/`daily_` live; dump Wheaty reale sotto carico; packaging/installer completo MASTER M9.

## Follow-up: test login client remoto (2026-11-05)

Vedi [CLIENT_TEST_REPORT.md](CLIENT_TEST_REPORT.md) per i fix cert LoginREST + Network.SkipBuildAuthKeyCheck, note di ripristino e backlog opcode non gestiti dalla sessione di maintainer.
