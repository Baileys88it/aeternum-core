#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace aeternum::sdk::internal::wasm {
// Registry owns metadata only, NEVER a VM. Identity is the canonical UTF-8 path
// used by the Phase1 profiler (without its "wasm/" prefix). Generation is required.
struct ModuleState { std::string module; std::uint64_t generation; bool running, abortRequested; };
enum class AbortResult { Accepted, Missing, StaleGeneration, Idle, AlreadyRequested };
std::vector<ModuleState> SnapshotModules();
AbortResult RequestAbort(std::string const& module, std::uint64_t generation);
}
