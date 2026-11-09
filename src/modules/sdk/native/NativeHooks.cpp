#include "aeternum/sdk/internal/data_hooks.hpp"
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
 * Aeternum IE-M3 Blocco2 - packet hook staging (on_init only), publish + freeze, guarded dispatch. HOT PATH.
 * Zero heap on every dispatch branch (call, skip, fault): fixed tables, atomics, stack buffers only.
 * Lint: contrib/aeternum/ci/check_adapter_no_map.ps1 scans this file and packet_hooks.hpp as hot path.
 */
#include "aeternum/sdk/internal/native_host.hpp"
#include "aeternum/sdk/internal/native_guard.h"
#include "aeternum/sdk/internal/packet_hooks.hpp"

#include "aeternum/sdk/internal/combat_hooks.hpp"
#include <cstdio>

namespace aeternum::sdk::internal::native
{
namespace
{
using hooks::g_packetHooks;
using hooks::HookEntry;
using hooks::kMaxHooksPerOpcode;
using hooks::kMaxNativeModules;
using hooks::kNumCmsgSlots;
using hooks::kNumSmsgSlots;
using hooks::OpcodeHooks;

std::atomic<int> g_subscribeWindow{ -1 };

std::atomic<std::uint64_t> g_statCalls{ 0 };
std::atomic<std::uint64_t> g_statSkipped{ 0 };
std::atomic<std::uint64_t> g_statFaults{ 0 };
std::atomic<std::uint64_t> g_statDropIgnored{ 0 };
std::atomic<std::uint64_t> g_statDropApplied{ 0 };
std::atomic<std::uint64_t> g_statMutateApplied{ 0 };
std::atomic<std::uint64_t> g_statMutateRejected{ 0 };

template <typename Fn>
void ForEachOpcode(Fn&& fn) noexcept
{
    for (std::uint32_t i = 0; i < kNumCmsgSlots; ++i)
        fn(g_packetHooks.cmsg[i]);
    for (std::uint32_t i = 0; i < kNumSmsgSlots; ++i)
        fn(g_packetHooks.smsg[i]);
}

bool PointerInImage(AeternumGuardImage const& img, void const* p) noexcept
{
    auto const a = reinterpret_cast<std::uintptr_t>(p);
    return img.base != 0 && a >= img.base && a < img.end;
}

void Dispatch(std::uint32_t direction, std::uint32_t index, AeternumPacketView const* view,
              hooks::PacketDispatchOutcome* outcome) noexcept
{
    NativeReadLease lease; if (!lease) return;
    std::uint32_t const limit = direction == AETERNUM_PACKET_DIR_CMSG ? kNumCmsgSlots : kNumSmsgSlots;
    if (index >= limit || !view || !outcome)
        return;
    OpcodeHooks const& oh = hooks::SlotsFor(direction)[index];
    std::uint32_t n = oh.count.load(std::memory_order_acquire);
    if (n > kMaxHooksPerOpcode)
        n = kMaxHooksPerOpcode;
    std::uint32_t const epoch = g_packetHooks.epoch.load(std::memory_order_acquire);
    for (std::uint32_t i = 0; i < n; ++i)
    {
        HookEntry const e = oh.entries[i];
        if (e.epoch != epoch || e.moduleSlot >= kMaxNativeModules || !e.fn)
        {
            g_statSkipped.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        int const slot = static_cast<int>(e.moduleSlot);
        CrossingScope scope(slot); // in_flight++ BEFORE the state check (F3: shutdown waits for in_flight == 0)
        if (GetSlotState(slot) != SlotState::Active || GetSlotGeneration(slot) != e.generation
            || g_packetHooks.epoch.load() != epoch)
        {
            g_statSkipped.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        // Reset the mutation OUT slot for this hook (SMSG only; CMSG view->mutation is nullptr).
        if (view->mutation)
        {
            view->mutation->data = nullptr;
            view->mutation->size = 0;
        }
        AeternumGuardFault fault;
        std::uint32_t verdict = AETERNUM_HOOK_PASS;
        AeternumGuardImage const* const image = GetSlotImage(slot);
        { AeternumProfiler::Scope timer(GetSlotProfiler(slot));
          aeternum_guard_call_hook(image, e.fn, GetSlotContext(slot), view, &verdict, &fault); }
        if (fault.faulted)
        {
            g_statFaults.fetch_add(1, std::memory_order_relaxed);
            char reason[kSlotReasonChars];
            std::uintptr_t const rva = fault.address >= image->base ? fault.address - image->base : 0;
            std::snprintf(reason, sizeof(reason), "native_fault:hook:%s:0x%06X:code=0x%08X:rva=0x%llX",
                          direction == AETERNUM_PACKET_DIR_CMSG ? "cmsg" : "smsg", static_cast<unsigned>(view->opcode),
                          static_cast<unsigned>(fault.code), static_cast<unsigned long long>(rva));
            QuarantineSlot(slot, reason);
            MarkTaintedAndArmGracefulShutdown(reason);
            continue;
        }
        g_statCalls.fetch_add(1, std::memory_order_relaxed);

        if (verdict == AETERNUM_HOOK_PASS)
            continue;

        if (verdict == AETERNUM_HOOK_DROP)
        {
            if (direction == AETERNUM_PACKET_DIR_CMSG)
            {
                outcome->drop = true; // IE-M5: discard before QueuePacket / legacy handler
                g_statDropApplied.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                // SMSG cancel not authorized; count and ignore (MUTATE is the SMSG path).
                g_statDropIgnored.fetch_add(1, std::memory_order_relaxed);
            }
            continue;
        }

        if (verdict == AETERNUM_HOOK_MUTATE)
        {
            char reason[kSlotReasonChars];
            auto fail_closed = [&](char const* why) {
                std::snprintf(reason, sizeof(reason), "native_mutate_%s:0x%06X", why,
                              static_cast<unsigned>(view->opcode));
                QuarantineSlot(slot, reason);
                MarkTaintedAndArmGracefulShutdown(reason);
                g_statMutateRejected.fetch_add(1, std::memory_order_relaxed);
            };

            if (direction != AETERNUM_PACKET_DIR_SMSG || !view->mutation)
            {
                fail_closed(direction != AETERNUM_PACKET_DIR_SMSG ? "wrong_dir" : "no_slot");
                continue;
            }
            uint8_t* const mutPtr = view->mutation->data;
            uint32_t const mutSize = view->mutation->size;
            if (!mutPtr || mutSize == 0)
            {
                fail_closed(!mutPtr ? "null" : "zero_size");
                continue;
            }
            // Validate + claim HostAlloc (registry remove, no free). No CRT malloc on this hot path.
            if (!HostClaimMutationBlock(slot, mutPtr, mutSize))
            {
                // Distinguish orphan vs size for diagnostics without a second registry walk when possible.
                std::uint64_t cookieSize = 0;
                bool const owned = HostPeekOwnedBlock(slot, mutPtr, &cookieSize);
                fail_closed(!owned ? "orphan_or_foreign" : "size_mismatch");
                continue;
            }
            if (outcome->mutateData)
                HostDisposeClaimed(outcome->mutateData); // last successful MUTATE wins
            outcome->mutateData = mutPtr;
            outcome->mutateSize = mutSize;
            g_statMutateApplied.fetch_add(1, std::memory_order_relaxed);
            continue;
        }

        // Unknown verdict: ignore (forward-compatible), do not fail-close.
        g_statDropIgnored.fetch_add(1, std::memory_order_relaxed);
    }
}

template<class Fn>
AeternumStatus SubscribeGameplay(AeternumModuleHandle module, Fn fn, combat::Table<Fn>& table) noexcept
{
    int const cur = CurrentCrossingSlot();
    if (cur < 0)
        return AETERNUM_ERR_DENIED;
    int const slot = ResolveModuleHandle(module);
    if (slot != cur)
    {
        QuarantineSlot(cur, "native_gameplay_subscribe_bad_handle");
        return AETERNUM_ERR_DENIED;
    }
    if (g_packetHooks.frozen.load() || g_subscribeWindow.load() != slot)
        return AETERNUM_ERR_DENIED;
    if (!fn || !PointerInImage(*GetSlotImage(slot), reinterpret_cast<void const*>(fn)))
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (table.staged >= combat::kMaxGameplayHooks)
        return AETERNUM_ERR_OVERFLOW;
    table.entries[table.staged++] = {fn, static_cast<std::uint32_t>(slot), GetSlotGeneration(slot), g_packetHooks.epoch.load()};
    return AETERNUM_OK;
}

template<class Fn, class View, class Invoke, class Accept>
void DispatchGameplay(combat::Table<Fn> const& table, View const& input, Invoke invoke, Accept accept) noexcept
{
    NativeReadLease lease; if (!lease) return;
    std::uint32_t const epoch = g_packetHooks.epoch.load(std::memory_order_acquire);
    auto n = table.count.load(std::memory_order_acquire);
    if (n > combat::kMaxGameplayHooks) n = combat::kMaxGameplayHooks;
    for (std::uint32_t i = 0; i < n; ++i)
    {
        auto const e = table.entries[i];
        if (!e.fn || e.epoch != epoch || e.moduleSlot >= kMaxNativeModules) continue;
        int const slot = static_cast<int>(e.moduleSlot);
        CrossingScope scope(slot);
        if (GetSlotState(slot) != SlotState::Active || GetSlotGeneration(slot) != e.generation
            || g_packetHooks.epoch.load() != epoch) continue;
        AeternumGuardFault fault{};
        std::uint32_t verdict = AETERNUM_HOOK_PASS;
        // Each callback has its own copy/OUT buffer; a fault/invalid verdict never commits it.
        View view = input;
        AeternumDamageMutation mutation{sizeof(AeternumDamageMutation), 0};
        auto const* image = GetSlotImage(slot);
        auto const status = [&] { AeternumProfiler::Scope timer(GetSlotProfiler(slot));
            return invoke(image, e.fn, GetSlotContext(slot), view, mutation, &verdict, &fault); }();
        if (fault.faulted || status != AETERNUM_OK)
        {
            char reason[kSlotReasonChars];
            std::snprintf(reason, sizeof(reason), "native_fault:gameplay:code=0x%08X:rva=0x%llX",
                static_cast<unsigned>(fault.code), static_cast<unsigned long long>(fault.address >= image->base ? fault.address-image->base : 0));
            QuarantineSlot(slot, reason);
            MarkTaintedAndArmGracefulShutdown(reason);
            continue;
        }
        if (GetSlotState(slot) == SlotState::Active && accept(verdict, mutation)) break;
    }
}

std::uint32_t DispatchCombat(AeternumCombatView const& input) noexcept
{
    if (!AETERNUM_FT_HAS(&input, AeternumCombatView, mutation)) return input.base_damage;
    auto damage = input.base_damage;
    DispatchGameplay(combat::g_gameplayHooks.damage, input,
        [](auto image, auto fn, void* ctx, AeternumCombatView& view, AeternumDamageMutation& mutation, auto verdict, auto fault) {
            mutation.override_damage = view.base_damage;
            view.mutation = &mutation;
            return aeternum_guard_call_combat(image, fn, ctx, &view, verdict, fault);
        },
        [&damage](std::uint32_t verdict, AeternumDamageMutation const& mutation) {
            if (verdict == AETERNUM_OVERRIDE_DAMAGE && AETERNUM_FT_HAS(&mutation, AeternumDamageMutation, override_damage))
                damage = mutation.override_damage;
            return false;
        });
    return damage;
}

bool DispatchSpellPrepare(AeternumSpellView const& input) noexcept
{
    if (!AETERNUM_FT_HAS(&input, AeternumSpellView, base_damage)) return false;
    bool cancel = false;
    DispatchGameplay(combat::g_gameplayHooks.spell, input,
        [](auto image, auto fn, void* ctx, AeternumSpellView& view, AeternumDamageMutation&, auto verdict, auto fault) {
            return aeternum_guard_call_spell(image, fn, ctx, &view, verdict, fault);
        },
        [&cancel](std::uint32_t verdict, AeternumDamageMutation const&) {
            cancel = verdict == AETERNUM_CANCEL_SPELL;
            return cancel;
        });
    return cancel;
}

template<class Action> void ForEachGameplay(Action action) noexcept
{
    action(combat::g_gameplayHooks.damage);
    action(combat::g_gameplayHooks.spell);
    action(combat::g_gameplayHooks.dataLoaded);
}

} // namespace

AeternumStatus AETERNUM_ABI_CALL AbiSubscribeCombat(AeternumModuleHandle module, AeternumCombatHookFn fn)
{
    return SubscribeGameplay(module, fn, combat::g_gameplayHooks.damage);
}
AeternumStatus AETERNUM_ABI_CALL AbiSubscribeSpell(AeternumModuleHandle module, AeternumSpellHookFn fn)
{
    return SubscribeGameplay(module, fn, combat::g_gameplayHooks.spell);
}

AeternumStatus AETERNUM_ABI_CALL AbiSubscribeDataLoaded(AeternumModuleHandle module, AeternumDataLoadedFn fn)
{
    return SubscribeGameplay(module, fn, combat::g_gameplayHooks.dataLoaded);
}

void SetSubscribeWindow(int slot) noexcept
{
    g_subscribeWindow.store(slot);
}

AeternumStatus SubscribePacketImpl(AeternumModuleHandle module, std::uint32_t direction, std::uint32_t opcode,
                                   AeternumPacketHookFn fn) noexcept
{
    int const cur = CurrentCrossingSlot();
    if (cur < 0)
        return AETERNUM_ERR_DENIED;                       // not inside a host crossing
    int const slot = ResolveModuleHandle(module);
    if (slot != cur)
    {
        QuarantineSlot(cur, "native_subscribe_bad_handle"); // module A acting as module B
        return AETERNUM_ERR_DENIED;
    }
    if (g_packetHooks.frozen.load() != 0 || g_subscribeWindow.load() != slot)
        return AETERNUM_ERR_DENIED;                       // only during this module's on_init; frozen afterwards
    if (direction > AETERNUM_PACKET_DIR_SMSG || !fn)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (!PointerInImage(*GetSlotImage(slot), reinterpret_cast<void const*>(fn)))
        return AETERNUM_ERR_INVALID_ARGUMENT;             // hook must live in the module image (F4.8)
    if (hooks::IsSocketLevelOpcode(direction, opcode))
        return AETERNUM_ERR_DENIED;                       // M6 permits movement; socket-level CMSG remain denied
    std::ptrdiff_t const idx = direction == AETERNUM_PACKET_DIR_CMSG ? hooks::CmsgIndex(opcode) : hooks::SmsgIndex(opcode);
    if (idx < 0 || idx >= static_cast<std::ptrdiff_t>(direction == AETERNUM_PACKET_DIR_CMSG ? hooks::kNumCmsgSlots : hooks::kNumSmsgSlots))
        return AETERNUM_ERR_NOT_FOUND;
    OpcodeHooks& oh = hooks::SlotsFor(direction)[idx];
    if (oh.staged >= kMaxHooksPerOpcode)
        return AETERNUM_ERR_OVERFLOW;
    HookEntry& e = oh.entries[oh.staged];
    e.fn = fn;
    e.moduleSlot = static_cast<std::uint32_t>(slot);
    e.generation = GetSlotGeneration(slot);
    e.epoch = g_packetHooks.epoch.load();
    ++oh.staged;
    return AETERNUM_OK;
}

void RetractStagedHooks(int slot) noexcept
{
    if (slot < 0 || g_packetHooks.frozen.load() != 0)
        return;
    ForEachGameplay([slot](auto& table) {
        std::uint32_t w = 0;
        for (std::uint32_t r = 0; r < table.staged; ++r)
            if (table.entries[r].moduleSlot != static_cast<std::uint32_t>(slot)) table.entries[w++] = table.entries[r];
        for (auto k = w; k < combat::kMaxGameplayHooks; ++k) table.entries[k] = {};
        table.staged = w;
    });
    ForEachOpcode([slot](OpcodeHooks& oh) {
        std::uint32_t w = 0;
        for (std::uint32_t r = 0; r < oh.staged && r < kMaxHooksPerOpcode; ++r)
            if (oh.entries[r].moduleSlot != static_cast<std::uint32_t>(slot))
                oh.entries[w++] = oh.entries[r];
        for (std::uint32_t k = w; k < kMaxHooksPerOpcode; ++k)
            oh.entries[k] = HookEntry{};
        oh.staged = w;
    });
}

void BeginHookBootCycle() noexcept
{
    ResetDataBootCycle();
    combat::g_gameplayHooks.damageDispatch.store(nullptr);
    combat::g_gameplayHooks.spellDispatch.store(nullptr);
    ForEachGameplay([](auto& table) {
        table.count.store(0);
        table.staged = 0;
        for (auto& e : table.entries) e = {};
    });
    g_packetHooks.dispatch.store(nullptr);
    ForEachOpcode([](OpcodeHooks& oh) {
        oh.count.store(0);
        oh.staged = 0;
        for (HookEntry& e : oh.entries)
            e = HookEntry{};
    });
    g_packetHooks.epoch.fetch_add(1u);
    g_packetHooks.published.store(0);
    g_packetHooks.frozen.store(0);
    g_subscribeWindow.store(-1);
    g_statCalls.store(0);
    g_statSkipped.store(0);
    g_statFaults.store(0);
    g_statDropIgnored.store(0);
    g_statDropApplied.store(0);
    g_statMutateApplied.store(0);
    g_statMutateRejected.store(0);
}

std::uint32_t PublishAndFreezeHooks() noexcept
{
    g_subscribeWindow.store(-1);
    std::uint32_t total = 0;
    ForEachOpcode([&total](OpcodeHooks& oh) { total += oh.staged; });
    ForEachGameplay([&total](auto& table) { total += table.staged; });
    if (total > 0)
        aeternum_guard_enable_persistent(); // POSIX: handlers installed once for multi-threaded dispatch
    g_packetHooks.dispatch.store(&Dispatch, std::memory_order_release);
    ForEachOpcode([](OpcodeHooks& oh) { oh.count.store(oh.staged, std::memory_order_release); });
    combat::g_gameplayHooks.damageDispatch.store(&DispatchCombat, std::memory_order_release);
    combat::g_gameplayHooks.spellDispatch.store(&DispatchSpellPrepare, std::memory_order_release);
    ForEachGameplay([](auto& table) { table.count.store(table.staged, std::memory_order_release); });
    g_packetHooks.published.store(total);
    g_packetHooks.frozen.store(1u);
    return total;
}

void UnpublishHooks() noexcept
{
    combat::g_gameplayHooks.damageDispatch.store(nullptr);
    combat::g_gameplayHooks.spellDispatch.store(nullptr);
    ForEachGameplay([](auto& table) { table.count.store(0); });
    g_packetHooks.dispatch.store(nullptr);
    ForEachOpcode([](OpcodeHooks& oh) { oh.count.store(0); });
    g_packetHooks.epoch.fetch_add(1u); // any entry read before this point is now stale
    g_packetHooks.published.store(0);
}

void PrepareSlotHookReload(int slot) noexcept {
    // Admission is closed and every root reader drained. Other slots keep their
    // entries/epoch. This is not BeginHookBootCycle (RAM data window stays frozen).
    g_packetHooks.frozen.store(0);
    RetractStagedHooks(slot);
}

bool HooksFrozen() noexcept
{
    return g_packetHooks.frozen.load() != 0;
}

std::uint32_t PublishedHookCount() noexcept
{
    return g_packetHooks.published.load();
}

HookDispatchStats GetHookDispatchStats() noexcept
{
    HookDispatchStats s;
    s.calls = g_statCalls.load();
    s.skipped = g_statSkipped.load();
    s.faults = g_statFaults.load();
    s.dropIgnored = g_statDropIgnored.load();
    s.dropApplied = g_statDropApplied.load();
    s.mutateApplied = g_statMutateApplied.load();
    s.mutateRejected = g_statMutateRejected.load();
    return s;
}

} // namespace aeternum::sdk::internal::native

namespace aeternum::sdk::internal::hooks
{
void AeternumReleaseMutationBytes(PacketDispatchOutcome& o) noexcept
{
    if (o.mutateData)
    {
        native::HostDisposeClaimed(o.mutateData);
        o.mutateData = nullptr;
        o.mutateSize = 0;
    }
}

} // namespace aeternum::sdk::internal::hooks
