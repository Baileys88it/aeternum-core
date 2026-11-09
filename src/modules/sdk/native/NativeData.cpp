// M9: boot-only, bounded post-load dispatch. Persistent RAM values belong to core caches, not this host.
#include "aeternum/sdk/internal/data_hooks.hpp"
#include "aeternum/sdk/internal/native_host.hpp"
#include "aeternum/sdk/internal/combat_hooks.hpp"
#include <atomic>
#include <cstdio>
namespace aeternum::sdk::internal::native {
namespace {
DataBackend backend{};
std::atomic<bool> dispatched{false};
thread_local bool window = false;
bool allowed() noexcept {
    int slot = CurrentCrossingSlot();
    return window && slot >= 0 && GetSlotState(slot) == SlotState::Active;
}
}
void ResetDataBootCycle() noexcept { backend = {}; dispatched.store(false); window = false; }
AeternumStatus ConfigureDataBackend(DataBackend const& b) noexcept {
    if (CurrentCrossingSlot() >= 0 || dispatched.load()) return AETERNUM_ERR_DENIED;
    if (!b.getHP || !b.setHP || !b.getStat || !b.setStat || !b.getWeapon || !b.setWeapon)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    backend = b; return AETERNUM_OK;
}
AeternumStatus DispatchDataLoaded() noexcept {
    if (CurrentCrossingSlot() >= 0) return AETERNUM_ERR_DENIED;
    if (!HooksFrozen() || !backend.getHP) return AETERNUM_ERR_NOT_READY;
    NativeReadLease lease; if (!lease) return AETERNUM_ERR_BUSY;
    if (dispatched.exchange(true)) return AETERNUM_ERR_BUSY;
    auto const& table = combat::g_gameplayHooks.dataLoaded;
    auto epoch = hooks::g_packetHooks.epoch.load();
    auto n = table.count.load();
    if (n > combat::kMaxGameplayHooks) n = combat::kMaxGameplayHooks;
    AeternumStatus result = AETERNUM_OK;
    for (uint32_t i=0; i<n; ++i) {
        auto e=table.entries[i];
        if (!e.fn || e.epoch != epoch || e.moduleSlot >= hooks::kMaxNativeModules) continue;
        int slot=static_cast<int>(e.moduleSlot);
        CrossingScope crossing(slot);
        if (GetSlotState(slot)!=SlotState::Active || GetSlotGeneration(slot)!=e.generation) continue;
        AeternumGuardFault fault{};
        window=true;
        auto status=[&]{ AeternumProfiler::Scope timer(GetSlotProfiler(slot));
            return aeternum_guard_call_ctx(GetSlotImage(slot),e.fn,GetSlotContext(slot),&fault); }();
        window=false;
        if (fault.faulted) {
            char reason[kSlotReasonChars];
            std::snprintf(reason,sizeof(reason),"native_fault:data_loaded:code=0x%08X",fault.code);
            QuarantineSlot(slot,reason); MarkTaintedAndArmGracefulShutdown(reason);
            result=AETERNUM_ERR_INTERNAL;
        } else if (status!=AETERNUM_OK) {
            QuarantineSlot(slot,"native_data_loaded_failed"); result=status;
        }
    }
    return result;
}
AeternumStatus AETERNUM_ABI_CALL AbiGetCreatureHP(uint32_t id,AeternumCreatureHPView* out) {
    return allowed()?backend.getHP(id,out):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiSetCreatureHP(uint32_t id,uint32_t hp) {
    return allowed()?backend.setHP(id,hp):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiGetItemStat(uint32_t id,uint32_t type,int32_t* out) {
    return allowed()?backend.getStat(id,type,out):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiSetItemStat(uint32_t id,uint32_t type,int32_t value) {
    return allowed()?backend.setStat(id,type,value):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiGetWeaponDamage(uint32_t id,AeternumWeaponDamageView* out) {
    return allowed()?backend.getWeapon(id,out):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiSetWeaponDamage(uint32_t id,float lo,float hi) {
    return allowed()?backend.setWeapon(id,lo,hi):AETERNUM_ERR_DENIED;
}
AeternumStatus AETERNUM_ABI_CALL AbiGetSpellRecovery(uint32_t id,AeternumSpellRecoveryView* out) {
    if(!allowed()) return AETERNUM_ERR_DENIED;
    return backend.getSpell?backend.getSpell(id,out):AETERNUM_ERR_UNSUPPORTED;
}
AeternumStatus AETERNUM_ABI_CALL AbiSetSpellRecovery(uint32_t id,uint32_t ms) {
    if(!allowed()) return AETERNUM_ERR_DENIED;
    return backend.setSpell?backend.setSpell(id,ms):AETERNUM_ERR_UNSUPPORTED;
}
AeternumStatus AETERNUM_ABI_CALL AbiGetLootChance(uint32_t id,uint32_t item,AeternumLootChanceView* out) {
    if(!allowed()) return AETERNUM_ERR_DENIED;
    return backend.getLoot?backend.getLoot(id,item,out):AETERNUM_ERR_UNSUPPORTED;
}
AeternumStatus AETERNUM_ABI_CALL AbiSetLootChance(uint32_t id,uint32_t item,float chance) {
    if(!allowed()) return AETERNUM_ERR_DENIED;
    return backend.setLoot?backend.setLoot(id,item,chance):AETERNUM_ERR_UNSUPPORTED;
}
}
