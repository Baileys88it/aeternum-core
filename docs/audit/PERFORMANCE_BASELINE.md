# Performance Baseline (M0)

**Rule:** never invent numbers. Mark gaps UNMEASURED.

## Measured (read-only sample, 2026-11-04 Europe/Rome ~23:40)

| Metric | Value | Method |
|---|---|---|
| worldserver Working Set | **196.2 MB** | `Get-Process` |
| worldserver Private Memory | **2711.4 MB** | `Get-Process` |
| worldserver CPU time cumulative | **275.2 s** since start 22:32 | `Get-Process` |
| bnetserver Working Set | **4.9 MB** | `Get-Process` |
| bnetserver Private Memory | **10.6 MB** | `Get-Process` |
| World init time (last start log) | **0 minutes 7 seconds** | Server.log “World initialized in …” |

## Explicitly UNMEASURED (reason)

| Metric | Reason |
|---|---|
| Per-tick / Update time diff histograms | Not scraped; would need Metric enable or log patterns + maintainer-approved ops; **no console commands sent** per M0 safety |
| Per-map MapUpdater latency | No instrumentation dump collected |
| Allocation rate / fragmentation | No Tracy/profiler attached |
| DB queue latency / slow queries | Not queried from performance_schema in this audit |
| Network pkt/s, bandwidth | Packet log may be on in ops but not baselined here |
| Player-count scalability curve | No load test |

## Notes

- `Metric.Enable = 0` in worldserver.conf sample → Influx path idle.  
- Future M3 must capture identical workloads before/after any optimization.  
- Custom allocators/pools: **no baseline → no adoption**.
