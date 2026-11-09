#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace aeternum::sdk::internal {
// Process-lifetime metrics keyed by kind/module id, aggregated across reloads (not slot index).
// Registration/dump are cold paths. Records never move; no heap/formatting/lock on timed crossings.
// Fixed cap: 256 distinct identities; refused registrations are visible in the JSON dump.
class AeternumProfiler final {
public:
 using Clock = std::chrono::high_resolution_clock;
 struct alignas(64) Record {
  std::string id;
  std::atomic<std::uint64_t> count{0}, ticks{0};
 };
 static AeternumProfiler& Instance() noexcept;
 Record* Register(char const* kind, char const* id) noexcept;
 // Independent atomic samples: a live dump is approximate, quiescent counts/times are exact.
 std::string Dump();
 void ResetCountersForTest() noexcept; // quiescent only, identities/pointers retained; not in C ABI
 class Scope final {
 public:
  explicit Scope(Record* record) noexcept;
  ~Scope() noexcept;
  Scope(Scope const&) = delete; Scope& operator=(Scope const&) = delete;
 private:
  Record* record_; Scope* previous_; bool owns_ = false; Clock::time_point start_{};
  static thread_local Scope* top_;
 };
private:
 AeternumProfiler() = default;
 std::mutex registration_;
 std::array<Record,256> records_{};
 std::size_t size_ = 0;
 std::atomic<std::uint64_t> registrationDrops_{0};
};
}
