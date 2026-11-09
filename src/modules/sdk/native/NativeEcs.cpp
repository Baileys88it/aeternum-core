#include "aeternum/sdk/internal/native_ecs.hpp"
#include "aeternum/sdk/internal/native_host.hpp"
#include <array>
#include <mutex>
namespace aeternum::sdk::internal::native {
namespace {
struct Entry {std::uint32_t map=0,instance=0,lifetime=0;std::weak_ptr<ecs::Kernel> kernel;};
std::mutex registryMutex;std::array<Entry,16> registry;AeternumModuleHandle coreOwner=nullptr;bool accepting=false;
void* allocate(void* owner,std::size_t bytes){
 auto module=reinterpret_cast<AeternumModuleHandle>(owner);int slot=ResolveModuleHandle(module);if(slot<0)return nullptr;
 CrossingScope crossing(slot);return HostAllocImpl(module,bytes); // ordinary ledger/cookie/cap checks, not a bypass
}
void release(void* owner,void* memory){
 auto module=reinterpret_cast<AeternumModuleHandle>(owner);int slot=ResolveModuleHandle(module);if(slot<0)return;
 CrossingScope crossing(slot);HostFreeImpl(module,memory);
}
bool caller(AeternumModuleHandle module){
 int slot=CurrentCrossingSlot();return slot>=0&&ResolveModuleHandle(module)==slot&&GetSlotState(slot)==SlotState::Active;
}
std::shared_ptr<ecs::Kernel> lookup(std::uint32_t map,std::uint32_t instance){
 std::lock_guard lock(registryMutex);for(auto const& e:registry)if(e.lifetime&&e.map==map&&e.instance==instance)return e.kernel.lock();return {};
}
class Driver final:public Aeternum::Ecs::MapDriver {
 std::shared_ptr<ecs::Kernel> kernel;
public:
 explicit Driver(std::shared_ptr<ecs::Kernel> k):kernel(std::move(k)){}
 ~Driver() override {
  kernel->Shutdown();std::lock_guard lock(registryMutex);
  for(auto& e:registry)if(e.lifetime==kernel->Lifetime())e={};
 }
 void CustomUpdate(std::uint32_t diff) noexcept override {(void)kernel->Tick(diff);}
};
}
bool EnableEcsForBoot() noexcept {
 if(!HooksFrozen()||CurrentCrossingSlot()>=0)return false;
 std::lock_guard lock(registryMutex);accepting=true;return true;
}
std::unique_ptr<Aeternum::Ecs::MapDriver> CreateOwnedEcsMap(std::uint32_t map,std::uint32_t instance,ecs::Options options) noexcept {
 try {
  if(!HooksFrozen())return nullptr; // slot boot/reset must have finished
  std::lock_guard lock(registryMutex);if(!accepting)return nullptr;Entry* available=nullptr;
  for(auto& e:registry){if(e.lifetime&&e.map==map&&e.instance==instance&&!e.kernel.expired())return nullptr;if(!e.lifetime||e.kernel.expired())available=&e;}
  if(!available)return nullptr;
  if(!coreOwner){int slot=ClaimModuleSlot("core.ecs");if(slot<0)return nullptr;coreOwner=MakeModuleHandle(slot);SetSlotState(slot,SlotState::Active);}
  if(ResolveModuleHandle(coreOwner)<0)return nullptr;
  auto unique=ecs::Kernel::Create({reinterpret_cast<void*>(coreOwner),allocate,release},std::move(options));if(!unique)return nullptr;
  std::shared_ptr<ecs::Kernel> kernel(std::move(unique));
  auto driver=std::make_unique<Driver>(kernel);*available={map,instance,kernel->Lifetime(),kernel};return driver;
 }catch(...){return nullptr;}
}
void ShutdownEcs() noexcept {
 // Prevent new ownership. Retain strong references while dropping the registry;
 // never hold its mutex while joining workers or acquiring a kernel gate.
 Aeternum::Ecs::factory.store(nullptr,std::memory_order_release);
 std::array<std::shared_ptr<ecs::Kernel>,16> kernels;AeternumModuleHandle owner;
 {std::lock_guard lock(registryMutex);accepting=false;for(unsigned i=0;i<registry.size();++i){kernels[i]=registry[i].kernel.lock();registry[i]={};}owner=coreOwner;coreOwner=nullptr;}
 for(auto& k:kernels)if(k)k->Shutdown();
 if(owner){int slot=ResolveModuleHandle(owner);if(slot>=0)SetSlotState(slot,SlotState::Stopped);}
}
AeternumStatus AETERNUM_ABI_CALL AbiEcsRegister(AeternumModuleHandle module,std::uint32_t map,std::uint32_t instance,AeternumEcsEntityState const* state,AeternumEcsEntityHandle* out){
 if(!caller(module))return AETERNUM_ERR_DENIED;if(!state||!out)return AETERNUM_ERR_INVALID_ARGUMENT;
 auto kernel=lookup(map,instance);if(!kernel)return AETERNUM_ERR_NOT_READY;
 return kernel->Register(reinterpret_cast<std::uintptr_t>(module),*state,*out);
}
AeternumStatus AETERNUM_ABI_CALL AbiEcsUnregister(AeternumModuleHandle module,std::uint32_t map,std::uint32_t instance,AeternumEcsEntityHandle entity){
 if(!caller(module))return AETERNUM_ERR_DENIED;auto kernel=lookup(map,instance);if(!kernel)return AETERNUM_ERR_NOT_READY;
 return kernel->Unregister(reinterpret_cast<std::uintptr_t>(module),entity);
}
AeternumStatus AETERNUM_ABI_CALL AbiEcsRead(AeternumModuleHandle module,std::uint32_t map,std::uint32_t instance,AeternumEcsEntityHandle entity,AeternumEcsEntityState* out){
 if(!caller(module))return AETERNUM_ERR_DENIED;if(!out)return AETERNUM_ERR_INVALID_ARGUMENT;auto kernel=lookup(map,instance);if(!kernel)return AETERNUM_ERR_NOT_READY;
 return kernel->Read(reinterpret_cast<std::uintptr_t>(module),entity,*out);
}
}
