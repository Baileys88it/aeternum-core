# M6 Blocco 2 - local hooked heuristic (no deployment)

Pure C11 standalone DLL, ABI v1, static CRT, single export aeternum_module_entry.
on_init subscribes successfully to CMSG_MOVE_HEARTBEAT (0x41002E),
CMSG_MOVE_START_FORWARD (0x410000), CMSG_MOVE_JUMP (0x410006).

## Session identity and ABI compatibility
PacketView appends uint32 session_id after mutation; all old field offsets remain
unchanged. New modules MUST check struct_size. Missing identity is UINT32_MAX
(not valid slot 0); old views fail closed. WorldSession construction allocates a
process-unique monotonic ID using a saturated atomic counter, never an account ID
or pointer hash. It is constant over that WorldSession lifetime (including socket
reattachment); new WorldSession/reconnect gets a new ID. IDs are never recycled;
UINT32_MAX is reserved, exhaustion throws rather than wraps. Allocation occurs
only in the constructor, never packet hot path. WorldSocket passes the ID while
holding its existing session lock; dispatch copies it into PacketView.
Outbound still has no session handle/ID (UINT32_MAX) as before.

65536 static BSS slots, checked direct index, 20 bytes each (1,310,720 bytes).
No malloc/HostAlloc. Out-of-range IDs (including the 65537th session lifetime)
DROP without table access. No aliasing/reuse to fit the array. This deliberate
finite lifetime capacity needs a future lifecycle/generation design before deploy.
Accepted samples update baseline; DROP does not. 3D speed uses squared distance
against 100 yards/second, O(1), without sqrt or CRT math imports. First sample
PASS; nonfinite/truncated prefix, duplicate/backwards timestamp and rollover DROP.
Packed GUID + MovementInfo prefix is verified against the engine deserializer.

M6 removes the movement-family subscribe deny (CMSG 0x41 / SMSG 0x5E).
Socket-level deny, on_init-only freeze, image/handle checks, quarantine/taint/60s
fail-closed and M5 DROP/MUTATE remain intact.

Catch2 [m6][blocco1] tests the C math/parser; [m6][blocco2] boots the actual DLL
through the native loader, verifies three published subscriptions and dispatches
normal/speedhack/out-of-range sessions through the guarded inbound callback.
No production anti-cheat claim: trusted server time, client timestamp forgery,
per-session callback concurrency, authorized teleports/transports/map changes,
high-speed exemptions and bounded session lifecycle remain future work.
No instance files installed; no deploy or runtime processes touched.
