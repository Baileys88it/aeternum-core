// Aeternum M7 gameplay hooks: fixed storage, no heap, no Trinity types.
#ifndef AETERNUM_SDK_INTERNAL_COMBAT_HOOKS_HPP
#define AETERNUM_SDK_INTERNAL_COMBAT_HOOKS_HPP
#include "aeternum/abi/AeternumABI.h"
#include <atomic>
#include <cstdint>
namespace aeternum::sdk::internal::combat
{
inline constexpr std::uint32_t kMaxGameplayHooks = 4;
template<class Fn> struct Entry
{
    Fn fn = nullptr;
    std::uint32_t moduleSlot = 0, generation = 0, epoch = 0;
};
template<class Fn> struct Table
{
    std::atomic<std::uint32_t> count{0};
    std::uint32_t staged = 0;
    Entry<Fn> entries[kMaxGameplayHooks]{};
};
using DamageDispatchFn = std::uint32_t (*)(AeternumCombatView const&) noexcept;
using SpellDispatchFn = bool (*)(AeternumSpellView const&) noexcept;
struct Tables
{
    Table<AeternumCombatHookFn> damage;
    Table<AeternumSpellHookFn> spell;
    Table<AeternumDataLoadedFn> dataLoaded;
    std::atomic<DamageDispatchFn> damageDispatch{nullptr};
    std::atomic<SpellDispatchFn> spellDispatch{nullptr};
};
inline constinit Tables g_gameplayHooks;
inline std::uint32_t DispatchDamage(AeternumCombatView const& view) noexcept
{
    if (!AETERNUM_FT_HAS(&view, AeternumCombatView, base_damage)) return 0;
    if (g_gameplayHooks.damage.count.load(std::memory_order_acquire) == 0)
        return view.base_damage;
    auto fn = g_gameplayHooks.damageDispatch.load(std::memory_order_acquire);
    return fn ? fn(view) : view.base_damage;
}
inline bool DispatchSpell(AeternumSpellView const& view) noexcept
{
    if (!AETERNUM_FT_HAS(&view, AeternumSpellView, base_damage)) return false;
    if (g_gameplayHooks.spell.count.load(std::memory_order_acquire) == 0)
        return false;
    auto fn = g_gameplayHooks.spellDispatch.load(std::memory_order_acquire);
    return fn && fn(view);
}
}
#endif
