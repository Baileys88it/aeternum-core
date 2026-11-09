/* M6: process-wide WorldSession lifetime identity. Allocation is constructor-only, never packet hot path. */
#ifndef AETERNUM_NATIVE_SESSION_ID_HPP
#define AETERNUM_NATIVE_SESSION_ID_HPP
#include <atomic>
#include <cstdint>
#include <stdexcept>
namespace aeternum::sdk::internal {
class NativeSessionIdSource {
public:
    explicit NativeSessionIdSource(std::uint32_t first = 0) noexcept : next_(first) { }
    std::uint32_t Allocate() {
        auto id = next_.load(std::memory_order_relaxed);
        for (;;) {
            if (id == UINT32_MAX)
                throw std::overflow_error("native WorldSession ID exhausted"); // never wrap/reuse
            if (next_.compare_exchange_weak(id, id + 1, std::memory_order_relaxed))
                return id;
        }
    }
private:
    std::atomic<std::uint32_t> next_;
};
inline NativeSessionIdSource g_nativeSessionIds;
} // namespace aeternum::sdk::internal
#endif
