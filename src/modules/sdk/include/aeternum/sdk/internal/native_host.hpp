/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Aeternum IE-M3 Blocco2 - INTERNAL host services behind the native Host FT (heap-free declarations).
 *
 *   Module slots   : fixed array of kMaxNativeModules; a module handle = (generation << 8) | (slot + 1), opaque to
 *                    the module. Slot state is atomic so packet threads can quarantine a module without locks.
 *   Crossing TLS   : the host records "which module am I calling on this thread" around EVERY crossing
 *                    (entry, lifecycle, hook). Host FT services refuse calls outside a crossing (F4.1) or with a
 *                    handle that is not the current one (anti-spoofing: module A cannot act as module B).
 *   HostAlloc (F2) : host CRT heap, 32-byte cookie header {magic, slot, generation, size, check} + 8-byte tail
 *                    canary; a fixed-capacity open-addressing registry of live blocks is consulted BEFORE any
 *                    dereference, so an orphan / foreign pointer is never read, never freed.
 *   Packet hooks   : staging during on_init only, publish + freeze at the end of boot (packet_hooks.hpp).
 *                    IE-M5: CMSG DROP honored; SMSG MUTATE via HostAlloc (F2) with fail-closed on malformed mutation.
 *   Graceful (F6)  : first contained fault => tainted + one-shot 60 s clean-shutdown request, consumed on the
 *                    world thread through aeternum::ops::IProcessControl::RequestShutdown (no TerminateProcess).
 */
#ifndef AETERNUM_SDK_INTERNAL_NATIVE_HOST_HPP
#define AETERNUM_SDK_INTERNAL_NATIVE_HOST_HPP

#include "aeternum/abi/AeternumABI.h"
#include "aeternum/sdk/internal/native_guard.h"
#include "aeternum/sdk/internal/packet_hooks.hpp"

#include "aeternum/sdk/internal/profiler.hpp"
#include "aeternum/sdk/internal/native_reload.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace aeternum::ops
{
class IProcessControl;
}

namespace aeternum::sdk::internal::native
{

// ---------------- module slots ----------------
enum class SlotState : std::uint32_t
{
    Free = 0,
    Booting,   // Validated..on_start: crossings allowed, hooks staged but not live
    Active,    // hooks live
    Stopping,  // shutdown in progress: dispatch refuses
    Stopped,
    Error      // quarantined (fault or policy violation): no crossing ever again
};

inline constexpr std::size_t kSlotIdChars = 64;
inline constexpr std::size_t kSlotReasonChars = 128;

/** Claim a slot for a module id (boot thread). Returns slot index or -1 when full. Bumps the slot generation. */
int ClaimModuleSlot(char const* id) noexcept;
AeternumProfiler::Record* GetSlotProfiler(int slot) noexcept;
char* AETERNUM_ABI_CALL AbiGetTelemetryDump(AeternumModuleHandle module);
void SetSlotImage(int slot, AeternumGuardImage const& image, void* modCtx) noexcept;
/** Never NULL (an empty image for an invalid slot). Written on the boot thread before hook publication. */
AeternumGuardImage const* GetSlotImage(int slot) noexcept;
void* GetSlotContext(int slot) noexcept;
void SetSlotState(int slot, SlotState state) noexcept;
SlotState GetSlotState(int slot) noexcept;
std::uint32_t GetSlotGeneration(int slot) noexcept;
std::uint32_t GetSlotInFlight(int slot) noexcept;
/** Quarantine (atomic, any thread). First reason wins. Returns true if this call moved the slot to Error. */
bool QuarantineSlot(int slot, char const* reason) noexcept;
/** Copy of the quarantine reason ("" if none). */
void GetSlotReason(int slot, char* out, std::size_t outSize) noexcept;
/** Release every slot (test isolation / new boot cycle). Not while packet threads dispatch. */
void ResetModuleSlots() noexcept;

AeternumModuleHandle MakeModuleHandle(int slot) noexcept;
/** -1 if the handle does not name a claimed slot of the current generation. */
int ResolveModuleHandle(AeternumModuleHandle h) noexcept;

// ---------------- crossing TLS ----------------
/** RAII: marks `slot` as the module being called on this thread (+in_flight), restores the previous one. */
class CrossingScope
{
public:
    explicit CrossingScope(int slot) noexcept;
    ~CrossingScope();
    CrossingScope(CrossingScope const&) = delete;
    CrossingScope& operator=(CrossingScope const&) = delete;

private:
    NativeReadLease _lease;
    int _slot;
    int _prev;
};
int CurrentCrossingSlot() noexcept;

void* AETERNUM_ABI_CALL AbiPersistentState(AeternumModuleHandle module, std::uint64_t schema, std::uint32_t version, std::uint32_t bytes);
bool RenewSlotForReload(int slot) noexcept; // exclusive, preserves verified persistent block only
std::uint32_t ReclaimSlotEphemeral(int slot) noexcept;

// ---------------- HostAlloc / HostFree (F2) ----------------
inline constexpr std::uint64_t kHostAllocMaxBlockBytes = 16ull * 1024 * 1024;   // one block
inline constexpr std::uint64_t kHostAllocModuleCapBytes = 64ull * 1024 * 1024;  // live bytes per module
inline constexpr std::uint64_t kHostAllocGlobalCapBytes = 256ull * 1024 * 1024; // live bytes, all modules
inline constexpr std::uint32_t kHostAllocMaxLiveBlocks = 16384;                  // live blocks, all modules
inline constexpr std::uint32_t kHostAllocHeaderBytes = 32;
inline constexpr std::uint32_t kHostAllocTailBytes = 8;

enum class HostFreeResult : std::uint32_t
{
    Freed = 0,
    Null,            // ptr == NULL: no-op
    NoCrossing,      // called outside a host crossing (module thread / after return): refused
    BadHandle,       // handle unknown / stale generation / not the current crossing module (spoof)
    Orphan,          // never returned by host_alloc (not in the live registry): NOT dereferenced, NOT freed
    Stale,           // recently freed by host_free (double free): NOT freed
    Foreign,         // live block owned by another module (cross-heap): NOT freed
    CookieCorrupt    // header / tail canary broken (overflow / underflow): NOT freed (leaked on purpose)
};

char const* HostFreeResultName(HostFreeResult r) noexcept;

void* HostAllocImpl(AeternumModuleHandle module, std::uint64_t size) noexcept;
HostFreeResult HostFreeImpl(AeternumModuleHandle module, void* ptr) noexcept;

struct HostAllocStats
{
    std::uint64_t liveBytes = 0;
    std::uint32_t liveBlocks = 0;
    std::uint64_t totalAllocs = 0;
    std::uint64_t totalFrees = 0;
    std::uint64_t refusedAllocs = 0;
    std::uint64_t rejectedFrees = 0;
    std::uint64_t leakedBlocks = 0;   // kept on purpose (faulted module / corrupt cookie)
};
HostAllocStats GetHostAllocStats() noexcept;
std::uint64_t GetSlotLiveBytes(int slot) noexcept;
std::uint32_t GetSlotLiveBlocks(int slot) noexcept;
/** Clean unload only: free (cookie-verified) the blocks a cleanly stopped module still owns. Returns count. */
std::uint32_t ReclaimSlotBlocks(int slot) noexcept;
/** Faulted / quarantined module: never walk its blocks; just mark them leaked in the stats. Returns count. */
std::uint32_t AbandonSlotBlocks(int slot) noexcept;
/** Test isolation: drop registry entries (does NOT free memory of abandoned blocks). */
void ResetHostAllocForTest() noexcept;

// ---------------- gameplay hooks (M7, same boot/freeze contract) ----------------
AeternumStatus AETERNUM_ABI_CALL AbiSubscribeCombat(AeternumModuleHandle module, AeternumCombatHookFn fn);
AeternumStatus AETERNUM_ABI_CALL AbiSubscribeSpell(AeternumModuleHandle module, AeternumSpellHookFn fn);

// ---------------- packet hook staging / publication ----------------
/** Opens the subscribe window for `slot` (only while its on_init crossing runs). -1 closes it. */
void SetSubscribeWindow(int slot) noexcept;
AeternumStatus SubscribePacketImpl(AeternumModuleHandle module, std::uint32_t direction, std::uint32_t opcode,
                                   AeternumPacketHookFn fn) noexcept;
/** Remove the staged hooks of a module that did not reach Active (boot thread, before publication). */
void RetractStagedHooks(int slot) noexcept;
/** New boot cycle: unpublish everything, bump the epoch, unfreeze, clear staging (no dispatch may be running). */
void BeginHookBootCycle() noexcept;
/** End of boot: publish staged hooks (release), install the dispatcher, freeze. Returns published entry count. */
std::uint32_t PublishAndFreezeHooks() noexcept;
/** Shutdown step 1: dispatcher off, counts 0, epoch bump (entries stale). Table stays frozen. */
void UnpublishHooks() noexcept;
void PrepareSlotHookReload(int slot) noexcept; // EXCLUSIVE write scope only
bool HooksFrozen() noexcept;
std::uint32_t PublishedHookCount() noexcept;

struct HookDispatchStats
{
    std::uint64_t calls = 0;
    std::uint64_t skipped = 0;        // stale epoch / module not Active
    std::uint64_t faults = 0;
    std::uint64_t dropIgnored = 0;    // SMSG DROP (no cancel) or CMSG MUTATE ignored paths counted for diagnostics
    std::uint64_t dropApplied = 0;    // IE-M5: CMSG DROP honored (packet discarded before legacy handler)
    std::uint64_t mutateApplied = 0;  // IE-M5: SMSG MUTATE accepted (HostAlloc validated, CRT copy produced)
    std::uint64_t mutateRejected = 0; // IE-M5: malformed MUTATE (orphan/size/null/wrong dir) => fail-closed
};
HookDispatchStats GetHookDispatchStats() noexcept;

/** IE-M5: registry lookup + cookie verify for a live HostAlloc block owned by slot.
 *  Never dereferences an orphan. On success writes cookie size. No free / no claim. */
bool HostPeekOwnedBlock(int slot, void* ptr, std::uint64_t* outSize) noexcept;
/** IE-M5: validate HostAlloc ownership + exact cookie size, remove from registry WITHOUT freeing.
 *  Caller owns the user pointer and MUST call HostDisposeClaimed after copying. Fail => false, no side effects
 *  other than leaving the live block untouched. */
bool HostClaimMutationBlock(int slot, void* ptr, std::uint32_t declaredSize) noexcept;
/** Free a user pointer previously returned by HostClaimMutationBlock (no crossing required). */
void HostDisposeClaimed(void* ptr) noexcept;

// ---------------- tainted + graceful shutdown (F6 / Q6) ----------------
inline constexpr std::uint32_t kTaintedGracefulShutdownSeconds = 60;

struct GracefulShutdownState
{
    bool armed = false;            // a contained fault armed the timer (one-shot per process)
    bool requested = false;        // the request was handed to IProcessControl
    std::uint32_t delaySeconds = 0;
    std::uint64_t armedAtMs = 0;   // steady clock
};

/** Flag tainted + arm the one-shot 60 s clean shutdown (any thread, lock-free, no heap). */
void MarkTaintedAndArmGracefulShutdown(char const* why) noexcept;
GracefulShutdownState GetGracefulShutdownState() noexcept;
/**
 * World thread (WorldScript::OnUpdate): reports pending quarantines, then, if armed and not yet requested, call process->RequestShutdown(delaySeconds, reason) once.
 * Returns true when the request was issued by this call. Safe to call every tick (one atomic load when idle).
 */
bool PumpGracefulShutdown(aeternum::ops::IProcessControl* process) noexcept;
/** Test seam: clear tainted + timer state (production never calls it). */
void ResetGracefulShutdownForTest() noexcept;

// ---------------- logging ----------------
/** Host diagnostics sink (NativeRuntime.cpp: the logger given to NativeModuleRuntime::Boot). 0 info, 1 warn, 2 error. */
void NativeHostLogLine(int level, char const* message) noexcept;
/** Log each quarantine once (boot thread / world thread only; never from a packet hook). Returns count logged. */
std::uint32_t ReportPendingQuarantines() noexcept;

/** Host FT functions (C ABI) wired into DefaultHostFT(). */
AeternumModuleHandle AETERNUM_ABI_CALL AbiModuleSelf(void);
void* AETERNUM_ABI_CALL AbiHostAlloc(AeternumModuleHandle module, std::uint64_t size);
AeternumStatus AETERNUM_ABI_CALL AbiHostFree(AeternumModuleHandle module, void* ptr);
AeternumStatus AETERNUM_ABI_CALL AbiSubscribePacket(AeternumModuleHandle module, std::uint32_t direction,
                                                    std::uint32_t opcode, AeternumPacketHookFn fn);
std::uint32_t AETERNUM_ABI_CALL AbiRuntimeTainted(void);
bool NativeTaintedFlag() noexcept;
void ClearNativeTaintedFlag() noexcept;

} // namespace aeternum::sdk::internal::native

#endif /* AETERNUM_SDK_INTERNAL_NATIVE_HOST_HPP */
