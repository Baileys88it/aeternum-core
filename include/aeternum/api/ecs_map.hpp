#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
namespace Aeternum::Ecs {
class MapDriver {
public:
 virtual ~MapDriver()=default;
 virtual void CustomUpdate(std::uint32_t diff) noexcept=0;
};
using Factory=std::unique_ptr<MapDriver>(*)(std::uint32_t map,std::uint32_t instance);
inline std::atomic<Factory> factory{nullptr}; // disabled unless trusted boot adapter opts in
inline std::unique_ptr<MapDriver> CreateMapDriver(std::uint32_t map,std::uint32_t instance) noexcept {
 try {auto f=factory.load(std::memory_order_acquire);return f?f(map,instance):nullptr;}catch(...){return nullptr;}
}
// The entire ECS-owned tick is CustomUpdate, at the tail of this single route.
// Legacy Map::Update is NOT called and there is no second update at its tail.
inline bool TryOwnedTick(MapDriver* driver,std::uint32_t diff) noexcept {
 if(!driver)return false;driver->CustomUpdate(diff);return true;
}
}
