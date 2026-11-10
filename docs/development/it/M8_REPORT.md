# Report M8 -- Harden admin/osservabilita sul Module SDK

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Commit:** `3665019d46` -- *M8: harden admin/observability on Module SDK* (23 file, +734/-257)  
**EN:** [`../en/M8_REPORT.md`](../en/M8_REPORT.md)

## Mapping

MASTER M8 = Administration & observability. Sequenza standing: harden admin/metriche su SDK M7.

## Consegnato

Top-N + MaxSeries 256; DenseScrape/SamplePercent; budget 75 MB / retention 14g (valori live store); token constant-time + min length + rate-limit 401; audit arricchito; admin ancora read-only; docs EN+IT.

## Build / test

Log intermedio `m8_tests.log`: **1 fail** (token corto vs TokenMinLength) poi fix. Suite Catch2 completa verde conservata: **1841 assertions / 110 test cases PASS** (`_buildlogs/m8_catch2_full.log`, 2026-11-05 ~09:36 Europe/Rome, RelWithDebInfo). Verifica successiva M9: Catch2 `[admin],[metric],...` **150 assert / 23 case PASS**.

## Soak / overhead

Countdown 120s IT+EN (`STOP_PROD_DONE 04:55:40`). Soak **21,67 min**: bnet **11676**, world **16084**. Store T0 1â€¯518â€¯897 B -> T20 1â€¯828â€¯208 B; stima **~20,6 MB/giorno**; dense CPU ~**29,3 ms/giorno**; budget use **2,32%**. Prod: bnet **10764**, world **21876**.

## Falso allarme path metriche

Verifica M8 su `_run/metrics` (assente). I file andavano in `_run/logs/metrics` (conf). Dettaglio e match disco in report M9.

## Non verificato

Login client reale; soak 72h; file `1h`/`1d`/`daily_` in live (soak troppo breve / idle).