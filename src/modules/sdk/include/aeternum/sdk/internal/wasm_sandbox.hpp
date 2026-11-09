#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <cstddef>
#include <cstdint>
namespace aeternum::sdk::internal::wasm {
// Owner-thread only (normally boot thread), including destruction. One bounded
// module per VM, no WASI. Only wasm_abort.hpp metadata API is cross-thread safe.
struct AllocationStats { std::size_t blocks, bytes, peakBytes; };
AllocationStats Allocations();
void FailAllocationAfterForTest(long long successfulAllocations); // -1 disables injection
class Sandbox final {
public:
 using Log = std::function<void(std::string const&)>;
 Sandbox(); ~Sandbox();
 Sandbox(Sandbox const&) = delete; Sandbox& operator=(Sandbox const&) = delete;
 // Logical compiled-op budget per on_init; host callbacks are not preemptible.
 bool Initialize(Log log, std::uint64_t fuel = 1000000);
 std::uint64_t FuelUsed() const;
 bool LoadAndRun(std::filesystem::path const& path);
 bool LoadImage(std::uint8_t const* data,std::size_t size,std::string identity);
 bool BindI32Export(char const* exported);
 bool CallI32(char const* exported,std::int32_t argument,std::int32_t& result);
 void Reset() noexcept;
 std::string const& Error() const;
 bool Ready() const;
private:
 struct Impl; std::unique_ptr<Impl> impl_;
};
}
