#ifndef AETERNUM_SDK_INTERNAL_DATA_HOOKS_HPP
#define AETERNUM_SDK_INTERNAL_DATA_HOOKS_HPP
#include "aeternum/abi/AeternumABI.h"
namespace aeternum::sdk::internal::native {
// Trusted core backend, bound once before dispatch; never supplied by a module.
struct DataBackend {
    AeternumStatus (*getHP)(uint32_t, AeternumCreatureHPView*);
    AeternumStatus (*setHP)(uint32_t, uint32_t);
    AeternumStatus (*getStat)(uint32_t, uint32_t, int32_t*);
    AeternumStatus (*setStat)(uint32_t, uint32_t, int32_t);
    AeternumStatus (*getWeapon)(uint32_t, AeternumWeaponDamageView*);
    AeternumStatus (*setWeapon)(uint32_t, float, float);
    AeternumStatus (*getSpell)(uint32_t, AeternumSpellRecoveryView*) = nullptr;
    AeternumStatus (*setSpell)(uint32_t, uint32_t) = nullptr;
    AeternumStatus (*getLoot)(uint32_t, uint32_t, AeternumLootChanceView*) = nullptr;
    AeternumStatus (*setLoot)(uint32_t, uint32_t, float) = nullptr;
};
AeternumStatus ConfigureDataBackend(DataBackend const&) noexcept;
AeternumStatus DispatchDataLoaded() noexcept;
void ResetDataBootCycle() noexcept;
AeternumStatus AETERNUM_ABI_CALL AbiSubscribeDataLoaded(AeternumModuleHandle, AeternumDataLoadedFn);
AeternumStatus AETERNUM_ABI_CALL AbiGetCreatureHP(uint32_t, AeternumCreatureHPView*);
AeternumStatus AETERNUM_ABI_CALL AbiSetCreatureHP(uint32_t, uint32_t);
AeternumStatus AETERNUM_ABI_CALL AbiGetItemStat(uint32_t, uint32_t, int32_t*);
AeternumStatus AETERNUM_ABI_CALL AbiSetItemStat(uint32_t, uint32_t, int32_t);
AeternumStatus AETERNUM_ABI_CALL AbiGetWeaponDamage(uint32_t, AeternumWeaponDamageView*);
AeternumStatus AETERNUM_ABI_CALL AbiSetWeaponDamage(uint32_t, float, float);
AeternumStatus AETERNUM_ABI_CALL AbiGetSpellRecovery(uint32_t, AeternumSpellRecoveryView*);
AeternumStatus AETERNUM_ABI_CALL AbiSetSpellRecovery(uint32_t, uint32_t);
AeternumStatus AETERNUM_ABI_CALL AbiGetLootChance(uint32_t, uint32_t, AeternumLootChanceView*);
AeternumStatus AETERNUM_ABI_CALL AbiSetLootChance(uint32_t, uint32_t, float);
}
#endif
