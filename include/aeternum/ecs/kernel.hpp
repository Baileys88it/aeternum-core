#pragma once
#include "aeternum/abi/AeternumABI.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
namespace aeternum::ecs {
struct Allocator { void* context; void* (*allocate)(void*,std::size_t); void (*release)(void*,void*); };
// Test control owns only atomics, never a Map/Trinity object. No guest callbacks.
struct TestControl {std::atomic<bool> entered{false},release{false};};
struct Options {
 std::uint32_t capacity=1024,workers=16;
 bool deterministicProbe=false; // fixture-only arithmetic probe
 bool movementWorkload=false;   // parallel XYZ/O coordinate update; production stays identity stub
 int failWorkerForTest=-1,failStartAfterForTest=-1;
 std::shared_ptr<TestControl> control;
};
struct Stats {
 std::uint64_t ticks=0,totalDiff=0,applied=0,workerJobs=0;
 std::uint32_t entities=0,workers=0,distinctWorkers=0;
 std::thread::id coordinator;
};
class Kernel final {
public:
 static std::unique_ptr<Kernel> Create(Allocator,Options={}) noexcept;
 ~Kernel();Kernel(Kernel const&)=delete;Kernel& operator=(Kernel const&)=delete;
 AeternumStatus Register(std::uint64_t owner,AeternumEcsEntityState const&,AeternumEcsEntityHandle&) noexcept;
 AeternumStatus Unregister(std::uint64_t owner,AeternumEcsEntityHandle) noexcept;
 AeternumStatus Read(std::uint64_t owner,AeternumEcsEntityHandle,AeternumEcsEntityState&) noexcept;
 AeternumStatus Tick(std::uint32_t diff) noexcept;
 AeternumStatus Inspect(Stats&) noexcept;
 AeternumStatus ExportTransforms(void* dst,std::size_t bytes) noexcept; // HostAlloc SOA X|Y|Z|O
 std::size_t TransformBytes() const noexcept;
 void Shutdown() noexcept; // cancels, waits for L, joins workers, then releases SOA blocks
 std::uint32_t Lifetime() const noexcept;
private:
 struct Impl;explicit Kernel(std::unique_ptr<Impl>);std::unique_ptr<Impl> p_;
};
}
