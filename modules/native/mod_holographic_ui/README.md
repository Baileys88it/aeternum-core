# M11 local native countdown sample (no live/client validation)

## Protocol facts and explicit scope change
Target: active Forever Classic 1.60.1 build70790.
`SMSG_BATTLEGROUND_INFO` does not exist as an exact opcode in this tree/profile.
`SMSG_BATTLEGROUND_INFO_THROTTLED` is a different opcode, not a timer/text API.
Use the real `SMSG_START_TIMER`: logical0x45003E, profile wire0x47003E.
Source evidence: MiscPackets::StartTimer::Write, Group::CountdownTimerType,
WorldSession::HandleQueryCountdownTimer, Opcodes.h/cpp and profile opcodes.json.
Native serializer: total seconds int64 LE, type int32 LE, remaining seconds
int64 LE, optional PlayerGuid presence bit flushed to one byte. This sample
uses type Pvp=0, total=remaining=30, absent GUID: exactly21 bytes.
No arbitrary text field exists. Screen position/visibility outside battleground
state and actual client decoding/rendering are NOT validated by unit tests.
No claim of arbitrary center-screen holographic text or live wire success.
The version-mapped opcode/native serializer are verified; no client patch.

## Minimal C ABI and trigger
Append-only ABI v1 `aeternum_send_packet(session_id, HostAllocEnvelope*)`.
Envelope16bytes + payload21bytes. Registry/cookie ownership is checked BEFORE
any pointer dereference. Claimed owned envelopes are disposed on every success
or failure within Booting/Active/Stopping crossings (only Active may send);
this includes the F3 shutdown race after HostAlloc. Orphan/foreign/corrupt
pointers, quarantined states and calls outside crossings are not claimed. Sender must never reuse an envelope after successful claim.
Allowlists only a canonical no-GUID Pvp START_TIMER,1..60 seconds, left<=total;
no auth/movement/arbitrary packets or strings. Hook `/played` through real
CMSG_REQUEST_PLAYED_TIME0x3D014C, one-bit body, returns PASS without changing
normal handler behavior. Exactly one accepted timer per realm socket lifetime,
including across modules; repeat requests may allocate/free37bytes but enqueue
no further timer and generate no per-packet log. No periodic tick flood.

## Lifetime/thread/transport
WorldSocket installs a stack/TLS SendTargetScope under its existing
_worldSessionLock, before inbound hook dispatch and before WorldSession queue.
Only that exact process-unique M6 lifetime session_id is accepted synchronously;
there is no global raw-session pointer registry, no cached session capability,
no sends from lifecycle/module-created threads, no stale-ID/other-session sends.
The socket/session cannot rebind under this lock. Socket close can still race
AFTER enqueue: OK means copied to transport ownership, NOT delivered to client.
Socket read-thread-only one-shot flag; no new raw concurrent network writes.
Injection skips WorldSession gameplay dispatch, ScriptMgr OnPacketSend and
native outbound hooks (no recursion). Mandatory WorldSocket MPSC _bufferQueue,
WritePacketToBuffer adapter/framing/compression/AEAD and Socket async write queue
are preserved. It is NOT queue-free. Existing SendPacket path is unchanged.

## Tests and limitations
Tests load the actual standalone C DLL through the native loader; compare
bytes against BOTH literal golden21bytes and the production StartTimer encoder.
Production WorldSocket queue owns the copied WorldPacket.
A C++-only RAII thread-local adapter fixture loads the REAL profile with its
production SHA-verified loader; it never calls/reloads or resets the global boot
table/freeze guard. The same production Translate/Peek functions select the
scoped real table, and restore prior state at scope exit (explicit regression).
No fake opcode mapping and no live configuration edits. A synthetic local
TCP pair uses OS-selected ephemeral loopback port only to construct a real
WorldSocket (constructor requires remote_endpoint); never Start/Update/network
packet writes, never production connections. Test-only friend dequeues the real
MPSC queue and calls the real production framer. AEAD roundtrip uses an all-0x42
synthetic fixture key, not any live key/credential. Payload/framing/crypto tests
are NOT a live packet capture or client UI test. Compression is unchanged and
not exercised by this21byte sample (threshold1024).
Tests exercise invalid/stale/no-current session capability, disconnected and
unauthenticated socket, malformed/forbidden payloads, orphan/foreign/stale
ownership, module-created thread rejection, neighboring HostAlloc bytes and
cookies, repeat rate refusal and legacy FT rejection. Allocation assertions
cover monitored HostAlloc live blocks/bytes plus claimed/disposed counters,
not universal CRT/game/process leak freedom. ABI M3/M5/M6/M7/M9/M10 regressions
remain separate tests. No deployment, actual player session or server restart.

## Strike2 sourced trigger correction
WorldSocket.cpp417-492 retains the decrypted4-byte wire opcode after read<uint32>().
Its hook view is data()/size(), so the actual played request has5bytes, not the
body-only1byte used by the first module/tests. This is a source-proven mismatch,
not a retrospective live trace proving the cause of the first UI failure.
The pinned real profile maps wire0x3E014C and0x3E014E to logical0x3D014C
CMSG_REQUEST_PLAYED_TIME. /played is a dedicated client opcode, NOT a chat string;
there is no case folding or NUL-terminated chat parser to add. Accept exactly
wire4+flag1 and those aliases; the native decoder ignores the seven padding bits.
Exact host log immediately before send: TRIGGER: /played intercettato, iniezione timer in corso
After send: TIMER_SEND: status=<numeric C ABI status>;0 means queued, not delivered.
Tests now retain the real WorldPacket wire prefix, translate BOTH aliases through
the SHA-verified real profile, dispatch the real SDK hook to the real DLL/queue,
verify log ordering and unchanged native played bit/PASS; this is still not a
live socket/client proof. Timer21bytes/30s and all transport restrictions unchanged.
