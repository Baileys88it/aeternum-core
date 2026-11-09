#pragma once
#include "aeternum/api/ecs_map.hpp"
#include "aeternum/ecs/kernel.hpp"
namespace aeternum::sdk::internal::native {
std::unique_ptr<Aeternum::Ecs::MapDriver> CreateOwnedEcsMap(std::uint32_t map,std::uint32_t instance,ecs::Options options={}) noexcept;
void ShutdownEcs() noexcept;
bool EnableEcsForBoot() noexcept;
AeternumStatus AETERNUM_ABI_CALL AbiEcsRegister(AeternumModuleHandle,std::uint32_t,std::uint32_t,AeternumEcsEntityState const*,AeternumEcsEntityHandle*);
AeternumStatus AETERNUM_ABI_CALL AbiEcsUnregister(AeternumModuleHandle,std::uint32_t,std::uint32_t,AeternumEcsEntityHandle);
AeternumStatus AETERNUM_ABI_CALL AbiEcsRead(AeternumModuleHandle,std::uint32_t,std::uint32_t,AeternumEcsEntityHandle,AeternumEcsEntityState*);
}
