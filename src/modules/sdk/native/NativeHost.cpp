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
 * Aeternum IE-M3 Blocco2 - module slots, crossing TLS, HostAlloc/HostFree (F2), tainted + 60 s graceful shutdown (F6).
 * See native_host.hpp for the contract. No STL container at the ABI; fixed static storage only.
 */
#include "aeternum/sdk/internal/native_host.hpp"

#include "aeternum/api/ops/runtime.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string_view>

namespace aeternum::sdk::internal::native
{
namespace
{
using hooks::kMaxNativeModules;

// ---------------- module slots ----------------
struct Slot
{
    std::atomic<std::uint32_t> state{ static_cast<std::uint32_t>(SlotState::Free) };
    std::atomic<std::uint32_t> generation{ 0 };
    std::atomic<std::uint32_t> inFlight{ 0 };
    std::atomic<std::uint32_t> reasonSet{ 0 };   // 0 none, 1 writing, 2 readable
    std::atomic<std::uint32_t> reported{ 0 };
    std::atomic<std::uint64_t> liveBytes{ 0 };
    std::atomic<std::uint32_t> liveBlocks{ 0 };
    void* persistent=nullptr; std::uint64_t schema=0; std::uint32_t schemaVersion=0, persistentBytes=0;
    AeternumGuardImage image{};
    void* modCtx = nullptr;
    AeternumProfiler::Record* profiler = nullptr; // published with the module slot; process-lifetime record
    char id[kSlotIdChars]{};
    char reason[kSlotReasonChars]{};
};

Slot g_slots[kMaxNativeModules];
thread_local int t_crossingSlot = -1;

void CopyBounded(char* dst, std::size_t cap, char const* src) noexcept
{
    if (!dst || cap == 0)
        return;
    std::size_t n = 0;
    if (src)
        while (n + 1 < cap && src[n] != '\0')
        {
            dst[n] = src[n];
            ++n;
        }
    dst[n] = '\0';
}

bool ValidSlot(int slot) noexcept
{
    return slot >= 0 && static_cast<std::uint32_t>(slot) < kMaxNativeModules;
}

// ---------------- tainted / graceful ----------------
std::atomic<bool> g_tainted{ false };
std::atomic<std::uint32_t> g_gsArmed{ 0 };
std::atomic<std::uint32_t> g_gsRequested{ 0 };
std::atomic<std::uint64_t> g_gsArmedAtMs{ 0 };
std::atomic<std::uint32_t> g_gsReasonSet{ 0 };
char g_gsReason[kSlotReasonChars]{};

std::uint64_t SteadyMs() noexcept
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ---------------- HostAlloc registry ----------------
constexpr std::uint32_t kBlockMagic = 0x41454D33u;  // "AEM3"
constexpr std::uint32_t kFreedMagic = 0x46524545u;  // "FREE"
constexpr std::uint64_t kTailSalt = 0xA5E7E2B1C3D4F607ull;
constexpr std::uint32_t kRegCap = kHostAllocMaxLiveBlocks * 2u; // power of two, load factor <= 0.5
constexpr std::uint32_t kRegMask = kRegCap - 1u;
constexpr std::uint32_t kRecentCap = 256;
static_assert((kRegCap & kRegMask) == 0, "registry capacity must be a power of two");

struct BlockHeader
{
    std::uint32_t magic;
    std::uint32_t slot;
    std::uint32_t generation;
    std::uint32_t reserved;
    std::uint64_t size;
    std::uint64_t check;
};
static_assert(sizeof(BlockHeader) == kHostAllocHeaderBytes, "cookie header must be 32 bytes (keeps 16-byte alignment)");

struct RegEntry
{
    std::uintptr_t user = 0; // 0 = empty
    std::uint32_t slot = 0;
    std::uint32_t generation = 0;
    std::uint64_t size = 0;
    bool persistent = false;
};

struct RecentFree
{
    std::uintptr_t user = 0;
    std::uint32_t slot = 0;
};

std::mutex g_allocMutex;
RegEntry g_reg[kRegCap];
RecentFree g_recent[kRecentCap];
std::uint32_t g_recentNext = 0;
HostAllocStats g_stats;          // guarded by g_allocMutex
std::uint64_t g_leakedBytes = 0; // counts toward the global cap

std::uint64_t Mix64(std::uint64_t x) noexcept
{
    x ^= x >> 33;
    x *= 0xFF51AFD7ED558CCDull;
    x ^= x >> 33;
    x *= 0xC4CEB9FE1A85EC53ull;
    x ^= x >> 33;
    return x;
}

std::uint64_t Secret() noexcept
{
    static std::uint64_t const s = Mix64(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&g_reg))
                                         ^ (static_cast<std::uint64_t>(SteadyMs()) << 17) ^ 0x9E3779B97F4A7C15ull);
    return s;
}

std::uint64_t CookieCheck(std::uint32_t slot, std::uint32_t generation, std::uint64_t size, std::uintptr_t user) noexcept
{
    std::uint64_t const a = Secret() ^ (static_cast<std::uint64_t>(kBlockMagic) << 32) ^ slot ^ (static_cast<std::uint64_t>(generation) << 8);
    return Mix64(a) ^ Mix64(size ^ static_cast<std::uint64_t>(user) ^ 0x5851F42D4C957F2Dull);
}

std::uint32_t Home(std::uintptr_t user) noexcept
{
    return static_cast<std::uint32_t>(Mix64(static_cast<std::uint64_t>(user) >> 4)) & kRegMask;
}

int RegFind(std::uintptr_t user) noexcept
{
    std::uint32_t i = Home(user);
    for (std::uint32_t n = 0; n < kRegCap; ++n, i = (i + 1u) & kRegMask)
    {
        if (g_reg[i].user == 0)
            return -1;
        if (g_reg[i].user == user)
            return static_cast<int>(i);
    }
    return -1;
}

bool RegInsert(RegEntry const& e) noexcept
{
    std::uint32_t i = Home(e.user);
    for (std::uint32_t n = 0; n < kRegCap; ++n, i = (i + 1u) & kRegMask)
        if (g_reg[i].user == 0)
        {
            g_reg[i] = e;
            return true;
        }
    return false;
}

/** Backward-shift deletion (linear probing, no tombstones). */
void RegErase(std::uint32_t i) noexcept
{
    std::uint32_t j = i;
    for (;;)
    {
        j = (j + 1u) & kRegMask;
        if (g_reg[j].user == 0)
            break;
        std::uint32_t const k = Home(g_reg[j].user);
        bool const move = (j > i) ? (k <= i || k > j) : (k <= i && k > j);
        if (move)
        {
            g_reg[i] = g_reg[j];
            i = j;
        }
    }
    g_reg[i] = RegEntry{};
}

bool RecentlyFreed(std::uintptr_t user) noexcept
{
    for (RecentFree const& r : g_recent)
        if (r.user == user)
            return true;
    return false;
}

void PushRecent(std::uintptr_t user, std::uint32_t slot) noexcept
{
    g_recent[g_recentNext % kRecentCap] = RecentFree{ user, slot };
    g_recentNext = (g_recentNext + 1u) % kRecentCap;
}

bool VerifyCookie(RegEntry const& e) noexcept
{
    // Only ever called on a pointer found in the live registry: the 32 + size + 8 bytes are ours.
    auto const* h = reinterpret_cast<BlockHeader const*>(e.user - kHostAllocHeaderBytes);
    if (h->magic != kBlockMagic || h->slot != e.slot || h->generation != e.generation || h->size != e.size
        || h->check != CookieCheck(e.slot, e.generation, e.size, e.user))
        return false;
    std::uint64_t tail = 0;
    std::memcpy(&tail, reinterpret_cast<void const*>(e.user + e.size), sizeof(tail));
    return tail == (h->check ^ kTailSalt);
}

void ReleaseBlock(RegEntry const& e) noexcept
{
    auto* h = reinterpret_cast<BlockHeader*>(e.user - kHostAllocHeaderBytes);
    h->magic = kFreedMagic;
    h->check = 0;
    std::free(h);
}

void AccountRemove(RegEntry const& e) noexcept
{
    if (ValidSlot(static_cast<int>(e.slot)))
    {
        g_slots[e.slot].liveBytes.fetch_sub(e.size);
        g_slots[e.slot].liveBlocks.fetch_sub(1u);
    }
    g_stats.liveBytes -= e.size;
    g_stats.liveBlocks -= 1u;
}

bool AllocAllowedState(SlotState s) noexcept
{
    return s == SlotState::Booting || s == SlotState::Active;
}

bool FreeAllowedState(SlotState s) noexcept
{
    return s == SlotState::Booting || s == SlotState::Active || s == SlotState::Stopping;
}
} // namespace

// ================= slots =================
int ClaimModuleSlot(char const* id) noexcept
{
    for (std::uint32_t i = 0; i < kMaxNativeModules; ++i)
    {
        Slot& s = g_slots[i];
        std::uint32_t expected = static_cast<std::uint32_t>(SlotState::Free);
        if (!s.state.compare_exchange_strong(expected, static_cast<std::uint32_t>(SlotState::Booting)))
            continue;
        std::uint32_t g = s.generation.load() + 1u;
        if (g > 0x00FFFFFFu) { s.state.store(static_cast<std::uint32_t>(SlotState::Free));continue; } // retire, never alias an old handle
        s.generation.store(g);
        s.persistent=nullptr;s.schema=0;s.schemaVersion=0;s.persistentBytes=0;
        s.inFlight.store(0);
        s.reported.store(0);
        s.reasonSet.store(0);
        s.reason[0] = '\0';
        s.liveBytes.store(0);
        s.liveBlocks.store(0);
        s.image = AeternumGuardImage{};
        s.modCtx = nullptr;
        CopyBounded(s.id, sizeof(s.id), id);
        s.profiler = AeternumProfiler::Instance().Register("cabi", s.id);
        return static_cast<int>(i);
    }
    return -1;
}

AeternumProfiler::Record* GetSlotProfiler(int slot) noexcept {
    return ValidSlot(slot) ? g_slots[slot].profiler : nullptr;
}
char* AETERNUM_ABI_CALL AbiGetTelemetryDump(AeternumModuleHandle module) {
    int slot=CurrentCrossingSlot();
    if (slot<0 || ResolveModuleHandle(module)!=slot) return nullptr;
    try {
        auto text=AeternumProfiler::Instance().Dump();
        auto* result=static_cast<char*>(HostAllocImpl(module,text.size()+1));
        if(result)std::memcpy(result,text.c_str(),text.size()+1);
        return result;
    } catch (...) { return nullptr; }
}

void SetSlotImage(int slot, AeternumGuardImage const& image, void* modCtx) noexcept
{
    if (!ValidSlot(slot))
        return;
    g_slots[slot].image = image;
    g_slots[slot].modCtx = modCtx;
}

void SetSlotState(int slot, SlotState state) noexcept
{
    if (!ValidSlot(slot))
        return;
    Slot& s = g_slots[slot];
    // Error is sticky: nothing but a slot reset leaves it.
    std::uint32_t cur = s.state.load();
    while (cur != static_cast<std::uint32_t>(SlotState::Error)
           && !s.state.compare_exchange_weak(cur, static_cast<std::uint32_t>(state)))
    {
    }
}

AeternumGuardImage const* GetSlotImage(int slot) noexcept
{
    static AeternumGuardImage const empty{};
    return ValidSlot(slot) ? &g_slots[slot].image : &empty;
}

void* GetSlotContext(int slot) noexcept
{
    return ValidSlot(slot) ? g_slots[slot].modCtx : nullptr;
}

SlotState GetSlotState(int slot) noexcept
{
    return ValidSlot(slot) ? static_cast<SlotState>(g_slots[slot].state.load()) : SlotState::Free;
}

std::uint32_t GetSlotGeneration(int slot) noexcept
{
    return ValidSlot(slot) ? g_slots[slot].generation.load() : 0u;
}

std::uint32_t GetSlotInFlight(int slot) noexcept
{
    return ValidSlot(slot) ? g_slots[slot].inFlight.load() : 0u;
}

bool QuarantineSlot(int slot, char const* reason) noexcept
{
    if (!ValidSlot(slot))
        return false;
    Slot& s = g_slots[slot];
    std::uint32_t expectedReason = 0;
    if (s.reasonSet.compare_exchange_strong(expectedReason, 1u))
    {
        CopyBounded(s.reason, sizeof(s.reason), reason ? reason : "native_quarantined");
        s.reasonSet.store(2u);
    }
    std::uint32_t const prev = s.state.exchange(static_cast<std::uint32_t>(SlotState::Error));
    return prev != static_cast<std::uint32_t>(SlotState::Error) && prev != static_cast<std::uint32_t>(SlotState::Free);
}

void GetSlotReason(int slot, char* out, std::size_t outSize) noexcept
{
    if (!out || outSize == 0)
        return;
    out[0] = '\0';
    if (ValidSlot(slot) && g_slots[slot].reasonSet.load() == 2u)
        CopyBounded(out, outSize, g_slots[slot].reason);
}

void ResetModuleSlots() noexcept
{
    for (std::uint32_t i = 0; i < kMaxNativeModules; ++i)
    {
        Slot& s = g_slots[i];
        if (s.state.load() == static_cast<std::uint32_t>(SlotState::Free))
            continue;
        AbandonSlotBlocks(static_cast<int>(i)); // never walk a previous cycle's heap blocks
        s.persistent=nullptr;s.schema=0;s.schemaVersion=0;s.persistentBytes=0;
        s.state.store(static_cast<std::uint32_t>(SlotState::Free)); // generation kept => old handles stay invalid
    }
}

AeternumModuleHandle MakeModuleHandle(int slot) noexcept
{
    if (!ValidSlot(slot))
        return nullptr;
    std::uintptr_t const v = (static_cast<std::uintptr_t>(g_slots[slot].generation.load()) << 8)
        | static_cast<std::uintptr_t>(slot + 1);
    return reinterpret_cast<AeternumModuleHandle>(v);
}

int ResolveModuleHandle(AeternumModuleHandle h) noexcept
{
    std::uintptr_t const v = reinterpret_cast<std::uintptr_t>(h);
    if (v == 0)
        return -1;
    int const slot = static_cast<int>(v & 0xFFu) - 1;
    std::uint32_t const gen = static_cast<std::uint32_t>(v >> 8);
    if (!ValidSlot(slot) || gen == 0 || (v >> 32) != 0)
        return -1;
    Slot const& s = g_slots[slot];
    if (s.state.load() == static_cast<std::uint32_t>(SlotState::Free) || s.generation.load() != gen)
        return -1;
    return slot;
}

// ================= crossing TLS =================
CrossingScope::CrossingScope(int slot) noexcept : _slot(slot), _prev(t_crossingSlot)
{
    if (!_lease) _slot = -1;
    if (ValidSlot(_slot))
        g_slots[_slot].inFlight.fetch_add(1u); // seq_cst: pairs with Stopping/Error store + in_flight poll (F3)
    t_crossingSlot = _slot;
}

CrossingScope::~CrossingScope()
{
    t_crossingSlot = _prev;
    if (ValidSlot(_slot))
        g_slots[_slot].inFlight.fetch_sub(1u);
}

int CurrentCrossingSlot() noexcept
{
    return t_crossingSlot;
}

// ================= HostAlloc / HostFree =================
char const* HostFreeResultName(HostFreeResult r) noexcept
{
    switch (r)
    {
        case HostFreeResult::Freed:         return "freed";
        case HostFreeResult::Null:          return "null";
        case HostFreeResult::NoCrossing:    return "no_crossing";
        case HostFreeResult::BadHandle:     return "bad_handle";
        case HostFreeResult::Orphan:        return "orphan";
        case HostFreeResult::Stale:         return "stale";
        case HostFreeResult::Foreign:       return "foreign";
        case HostFreeResult::CookieCorrupt: return "cookie_corrupt";
        default:                            return "?";
    }
}

void* HostAllocImpl(AeternumModuleHandle module, std::uint64_t size) noexcept
{
    int const cur = CurrentCrossingSlot();
    int const slot = ResolveModuleHandle(module);
    try
    {
        if (cur < 0 || slot != cur)
        {
            if (cur >= 0)
                QuarantineSlot(cur, "native_host_alloc_bad_handle"); // module A acting as module B
            std::lock_guard<std::mutex> g(g_allocMutex);
            ++g_stats.refusedAllocs;
            return nullptr;
        }
        if (!AllocAllowedState(GetSlotState(slot)) || size == 0 || size > kHostAllocMaxBlockBytes)
        {
            std::lock_guard<std::mutex> g(g_allocMutex);
            ++g_stats.refusedAllocs;
            return nullptr;
        }
        std::lock_guard<std::mutex> g(g_allocMutex);
        Slot& s = g_slots[slot];
        if (s.liveBytes.load() + size > kHostAllocModuleCapBytes
            || g_stats.liveBytes + g_leakedBytes + size > kHostAllocGlobalCapBytes
            || g_stats.liveBlocks >= kHostAllocMaxLiveBlocks)
        {
            ++g_stats.refusedAllocs; // cap reached: fail closed, never abort
            return nullptr;
        }
        auto* raw = static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(kHostAllocHeaderBytes + size + kHostAllocTailBytes)));
        if (!raw)
        {
            ++g_stats.refusedAllocs;
            return nullptr;
        }
        std::uintptr_t const user = reinterpret_cast<std::uintptr_t>(raw) + kHostAllocHeaderBytes;
        BlockHeader h{};
        h.magic = kBlockMagic;
        h.slot = static_cast<std::uint32_t>(slot);
        h.generation = s.generation.load();
        h.reserved = 0;
        h.size = size;
        h.check = CookieCheck(h.slot, h.generation, size, user);
        std::memcpy(raw, &h, sizeof(h));
        std::uint64_t const tail = h.check ^ kTailSalt;
        std::memcpy(reinterpret_cast<void*>(user + size), &tail, sizeof(tail));
        RegEntry e;
        e.user = user;
        e.slot = h.slot;
        e.generation = h.generation;
        e.size = size;
        if (!RegInsert(e))
        {
            std::free(raw);
            ++g_stats.refusedAllocs;
            return nullptr;
        }
        s.liveBytes.fetch_add(size);
        s.liveBlocks.fetch_add(1u);
        g_stats.liveBytes += size;
        g_stats.liveBlocks += 1u;
        ++g_stats.totalAllocs;
        return reinterpret_cast<void*>(user);
    }
    catch (...)
    {
        return nullptr; // D11: nothing unwinds into module code
    }
}

HostFreeResult HostFreeImpl(AeternumModuleHandle module, void* ptr) noexcept
{
    if (!ptr)
        return HostFreeResult::Null;
    int const cur = CurrentCrossingSlot();
    int const slot = ResolveModuleHandle(module);
    HostFreeResult r = HostFreeResult::Freed;
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        std::uintptr_t const user = reinterpret_cast<std::uintptr_t>(ptr);
        if (cur < 0)
            r = HostFreeResult::NoCrossing;
        else if (slot != cur || !FreeAllowedState(GetSlotState(slot)))
            r = HostFreeResult::BadHandle;
        else
        {
            int const idx = RegFind(user); // registry lookup BEFORE any dereference of `ptr`
            if (idx < 0)
                r = RecentlyFreed(user) ? HostFreeResult::Stale : HostFreeResult::Orphan;
            else
            {
                RegEntry const e = g_reg[idx];
                if (e.persistent || e.slot != static_cast<std::uint32_t>(slot) || e.generation != g_slots[slot].generation.load())
                    r = HostFreeResult::Foreign; // who allocates frees: never across modules
                else if (e.persistent)
                    r = HostFreeResult::Foreign; // host-owned persistent storage is never module-freed
                else if (!VerifyCookie(e))
                {
                    r = HostFreeResult::CookieCorrupt;
                    RegErase(static_cast<std::uint32_t>(idx)); // leak it: never hand a smashed block back to the CRT
                    AccountRemove(e);
                    ++g_stats.leakedBlocks;
                    g_leakedBytes += e.size;
                }
                else
                {
                    RegErase(static_cast<std::uint32_t>(idx));
                    AccountRemove(e);
                    PushRecent(user, e.slot);
                    ReleaseBlock(e);
                    ++g_stats.totalFrees;
                    return HostFreeResult::Freed;
                }
            }
        }
        ++g_stats.rejectedFrees;
    }
    catch (...)
    {
        r = HostFreeResult::BadHandle;
    }
    if (cur >= 0)
    {
        char reason[kSlotReasonChars];
        std::snprintf(reason, sizeof(reason), "native_host_free_%s", HostFreeResultName(r));
        QuarantineSlot(cur, reason); // policy F2: leak > double free; offending module is quarantined
    }
    return r;
}

bool HostPeekOwnedBlock(int slot, void* ptr, std::uint64_t* outSize) noexcept
{
    if (!ptr || !outSize || !ValidSlot(slot))
        return false;
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        std::uintptr_t const user = reinterpret_cast<std::uintptr_t>(ptr);
        int const idx = RegFind(user); // registry BEFORE any dereference
        if (idx < 0)
            return false;
        RegEntry const e = g_reg[idx];
        if (e.persistent || e.slot != static_cast<std::uint32_t>(slot) || e.generation != g_slots[slot].generation.load())
            return false;
        if (!VerifyCookie(e))
            return false;
        *outSize = e.size;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool HostClaimMutationBlock(int slot, void* ptr, std::uint32_t declaredSize) noexcept
{
    if (!ptr || !ValidSlot(slot) || declaredSize == 0)
        return false;
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        std::uintptr_t const user = reinterpret_cast<std::uintptr_t>(ptr);
        int const idx = RegFind(user);
        if (idx < 0)
            return false;
        RegEntry const e = g_reg[idx];
        if (e.persistent || e.slot != static_cast<std::uint32_t>(slot) || e.generation != g_slots[slot].generation.load())
            return false;
        if (!VerifyCookie(e) || e.size != declaredSize)
            return false;
        RegErase(static_cast<std::uint32_t>(idx));
        AccountRemove(e);
        PushRecent(user, e.slot);
        // Memory remains allocated for the caller; HostDisposeClaimed frees it.
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void HostDisposeClaimed(void* ptr) noexcept
{
    if (!ptr)
        return;
    try
    {
        std::uintptr_t const user = reinterpret_cast<std::uintptr_t>(ptr);
        auto* h = reinterpret_cast<BlockHeader*>(user - kHostAllocHeaderBytes);
        // Best-effort: only free if the header still looks like a live AEM3 block we claimed.
        if (h->magic == kBlockMagic)
        {
            h->magic = kFreedMagic;
            h->check = 0;
            std::free(h);
        }
    }
    catch (...)
    {
    }
}


HostAllocStats GetHostAllocStats() noexcept
{
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        return g_stats;
    }
    catch (...)
    {
        return HostAllocStats{};
    }
}

std::uint64_t GetSlotLiveBytes(int slot) noexcept
{
    return ValidSlot(slot) ? g_slots[slot].liveBytes.load() : 0u;
}

std::uint32_t GetSlotLiveBlocks(int slot) noexcept
{
    return ValidSlot(slot) ? g_slots[slot].liveBlocks.load() : 0u;
}

static std::uint32_t ReclaimBlocks(int slot, bool keepPersistent) noexcept
{
    if (!ValidSlot(slot))
        return 0;
    std::uint32_t n = 0;
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        for (std::uint32_t i = 0; i < kRegCap; ++i)
            while (g_reg[i].user != 0 && g_reg[i].slot == static_cast<std::uint32_t>(slot) && !(keepPersistent && g_reg[i].persistent))
            {
                RegEntry const e = g_reg[i];
                RegErase(i);
                AccountRemove(e);
                if (VerifyCookie(e))
                {
                    ReleaseBlock(e);
                    ++g_stats.totalFrees;
                }
                else
                {
                    ++g_stats.leakedBlocks;
                    g_leakedBytes += e.size;
                }
                ++n;
            }
    }
    catch (...)
    {
    }
    return n;
}

std::uint32_t ReclaimSlotBlocks(int slot) noexcept {
 auto n=ReclaimBlocks(slot,false);if(ValidSlot(slot)){g_slots[slot].persistent=nullptr;g_slots[slot].schema=0;}return n;
}
std::uint32_t ReclaimSlotEphemeral(int slot) noexcept {return ReclaimBlocks(slot,true);}
void* AETERNUM_ABI_CALL AbiPersistentState(AeternumModuleHandle module,std::uint64_t schema,std::uint32_t version,std::uint32_t bytes) {
 int slot=CurrentCrossingSlot();if(slot<0 || ResolveModuleHandle(module)!=slot || !schema || !version || !bytes || bytes>65536)return nullptr;
 auto& s=g_slots[slot];
 // Called only under a root reader/exclusive owner. Serialize first acquisition
 // and cookie migration with the same allocator ledger (never bypass HostAlloc).
 {std::lock_guard<std::mutex> lock(g_allocMutex);
  if(s.persistent){
   int index=RegFind(reinterpret_cast<std::uintptr_t>(s.persistent));
   if(index<0 || !g_reg[index].persistent || !VerifyCookie(g_reg[index])){QuarantineSlot(slot,"native_persistent_cookie_corrupt");return nullptr;}
   return s.schema==schema && s.schemaVersion==version && s.persistentBytes==bytes?s.persistent:nullptr;
  }
 }
 if(GetSlotState(slot)!=SlotState::Booting)return nullptr;
 void* ptr=HostAllocImpl(module,bytes);if(!ptr)return nullptr;
 {std::lock_guard<std::mutex> lock(g_allocMutex);
  int i=RegFind(reinterpret_cast<std::uintptr_t>(ptr));if(i<0)return nullptr;
  g_reg[i].persistent=true;s.persistent=ptr;s.schema=schema;s.schemaVersion=version;s.persistentBytes=bytes;std::memset(ptr,0,bytes);
 }
 return ptr;
}
bool RenewSlotForReload(int slot) noexcept {
 if(!ValidSlot(slot))return false;auto& s=g_slots[slot];
 std::lock_guard<std::mutex> lock(g_allocMutex);auto next=s.generation.load()+1;
 if(next>0x00ffffff || s.inFlight.load())return false;
 for(auto& e:g_reg)if(e.user && e.slot==static_cast<std::uint32_t>(slot)) {
  if(!e.persistent || !VerifyCookie(e))return false;
  e.generation=next;auto* h=reinterpret_cast<BlockHeader*>(e.user-kHostAllocHeaderBytes);
  h->generation=next;h->check=CookieCheck(e.slot,next,e.size,e.user);auto tail=h->check^kTailSalt;
  std::memcpy(reinterpret_cast<void*>(e.user+e.size),&tail,sizeof(tail));
 }
 s.generation.store(next);s.image={};s.modCtx=nullptr;s.reasonSet.store(0);s.reported.store(0);s.reason[0]=0;s.state.store(static_cast<std::uint32_t>(SlotState::Booting));return true;
}

std::uint32_t AbandonSlotBlocks(int slot) noexcept
{
    if (!ValidSlot(slot))
        return 0;
    std::uint32_t n = 0;
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        for (std::uint32_t i = 0; i < kRegCap; ++i)
            while (g_reg[i].user != 0 && g_reg[i].slot == static_cast<std::uint32_t>(slot))
            {
                RegEntry const e = g_reg[i];
                RegErase(i); // registry bookkeeping only: the block memory is NOT touched (F2 after AV)
                AccountRemove(e);
                ++g_stats.leakedBlocks;
                g_leakedBytes += e.size;
                ++n;
            }
    }
    catch (...)
    {
    }
    return n;
}

void ResetHostAllocForTest() noexcept
{
    try
    {
        std::lock_guard<std::mutex> g(g_allocMutex);
        for (RegEntry& e : g_reg)
            e = RegEntry{};
        for (RecentFree& r : g_recent)
            r = RecentFree{};
        g_recentNext = 0;
        g_stats = HostAllocStats{};
        g_leakedBytes = 0;
        for (Slot& s : g_slots)
        {
            s.liveBytes.store(0);
            s.liveBlocks.store(0);
        }
    }
    catch (...)
    {
    }
}

// ================= tainted + graceful shutdown =================
bool NativeTaintedFlag() noexcept { return g_tainted.load(); }
void ClearNativeTaintedFlag() noexcept { g_tainted.store(false); }

void MarkTaintedAndArmGracefulShutdown(char const* why) noexcept
{
    g_tainted.store(true);
    std::uint32_t expected = 0;
    if (g_gsArmed.compare_exchange_strong(expected, 1u))
    {
        g_gsArmedAtMs.store(SteadyMs());
        std::uint32_t r = 0;
        if (g_gsReasonSet.compare_exchange_strong(r, 1u))
        {
            CopyBounded(g_gsReason, sizeof(g_gsReason), why ? why : "native_fault");
            g_gsReasonSet.store(2u);
        }
    }
}

GracefulShutdownState GetGracefulShutdownState() noexcept
{
    GracefulShutdownState s;
    s.armed = g_gsArmed.load() != 0;
    s.requested = g_gsRequested.load() != 0;
    s.delaySeconds = s.armed ? kTaintedGracefulShutdownSeconds : 0u;
    s.armedAtMs = g_gsArmedAtMs.load();
    return s;
}

bool PumpGracefulShutdown(aeternum::ops::IProcessControl* process) noexcept
{
    ReportPendingQuarantines();
    if (g_gsArmed.load() == 0 || g_gsRequested.load() != 0 || !process)
        return false;
    std::uint32_t expected = 0;
    if (!g_gsRequested.compare_exchange_strong(expected, 1u))
        return false;
    char msg[kSlotReasonChars + 96];
    std::snprintf(msg, sizeof(msg), "Aeternum native runtime TAINTED by a contained module fault (%s): clean shutdown in %u s",
                  g_gsReasonSet.load() == 2u ? g_gsReason : "native_fault", kTaintedGracefulShutdownSeconds);
    NativeHostLogLine(2, msg);
    try
    {
        process->RequestShutdown(kTaintedGracefulShutdownSeconds, std::string_view(msg));
    }
    catch (...)
    {
        // D11: the world tick must not unwind because of the shutdown request.
    }
    return true;
}

void ResetGracefulShutdownForTest() noexcept
{
    g_tainted.store(false);
    g_gsArmed.store(0);
    g_gsRequested.store(0);
    g_gsArmedAtMs.store(0);
    g_gsReasonSet.store(0);
    g_gsReason[0] = '\0';
}

std::uint32_t ReportPendingQuarantines() noexcept
{
    std::uint32_t n = 0;
    for (std::uint32_t i = 0; i < kMaxNativeModules; ++i)
    {
        Slot& s = g_slots[i];
        if (s.state.load() != static_cast<std::uint32_t>(SlotState::Error) || s.reasonSet.load() != 2u)
            continue;
        std::uint32_t expected = 0;
        if (!s.reported.compare_exchange_strong(expected, 1u))
            continue;
        char msg[kSlotIdChars + kSlotReasonChars + 64];
        std::snprintf(msg, sizeof(msg), "Native module %s QUARANTINED (%s) - no further crossings", s.id, s.reason);
        NativeHostLogLine(2, msg);
        ++n;
    }
    return n;
}

// ================= Host FT (C ABI) =================
AeternumModuleHandle AETERNUM_ABI_CALL AbiModuleSelf(void)
{
    int const cur = CurrentCrossingSlot();
    return cur < 0 ? nullptr : MakeModuleHandle(cur);
}

void* AETERNUM_ABI_CALL AbiHostAlloc(AeternumModuleHandle module, std::uint64_t size)
{
    return HostAllocImpl(module, size);
}

AeternumStatus AETERNUM_ABI_CALL AbiHostFree(AeternumModuleHandle module, void* ptr)
{
    HostFreeResult const r = HostFreeImpl(module, ptr);
    return (r == HostFreeResult::Freed || r == HostFreeResult::Null) ? AETERNUM_OK : AETERNUM_ERR_DENIED;
}

AeternumStatus AETERNUM_ABI_CALL AbiSubscribePacket(AeternumModuleHandle module, std::uint32_t direction,
                                                    std::uint32_t opcode, AeternumPacketHookFn fn)
{
    return SubscribePacketImpl(module, direction, opcode, fn);
}

std::uint32_t AETERNUM_ABI_CALL AbiRuntimeTainted(void)
{
    return g_tainted.load() ? 1u : 0u;
}

} // namespace aeternum::sdk::internal::native
