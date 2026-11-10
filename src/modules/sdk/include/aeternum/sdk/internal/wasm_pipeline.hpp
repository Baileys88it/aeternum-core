#pragma once
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include "aeternum/sdk/internal/wasm_sandbox.hpp"
namespace aeternum::sdk::internal::wasm {
inline constexpr char const* HotIdentity="hot/Aeternum_BossAI";
inline constexpr char const* HotExport="aeternum_boss_tick";
class Pipeline final {
public:
 static Pipeline& Instance();
 bool Deploy(std::uint8_t const* data,std::size_t size);
 bool Ready() const;
 bool Tick(std::int32_t damage,std::int32_t& hp);
 void Reset() noexcept;
 std::string const& Error() const;
 std::size_t ImageBytes() const;
private:
 Pipeline()=default;
 mutable std::mutex mutex_;
 Sandbox vm_;
 std::vector<std::uint8_t> image_;
 std::string error_;
};
}
