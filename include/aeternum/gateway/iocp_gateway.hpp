#pragma once
#include <cstddef>
#include <cstdint>
namespace aeternum::gateway {
inline constexpr std::uint32_t kMagic=0x47374C41u; // AL7G
inline constexpr std::uint32_t kMaxCapacity=50000;
inline constexpr std::uint32_t kMaxBackends=16;
inline constexpr std::uint32_t kRecvBytes=128;
struct FrameHeader {
 std::uint32_t magic;
 std::uint32_t conn;
 std::uint32_t backend;
 std::uint32_t bytes;
};
struct Backend {
 char address[64]{};
 std::uint16_t port=0;
};
struct Config {
 bool enabled=true;
 char bind[64]{"127.0.0.1"};
 std::uint16_t port=0;
 std::uint32_t capacity=kMaxCapacity,workers=2,acceptDepth=128;
 Backend backends[kMaxBackends]{};
 std::uint32_t backendCount=0;
};
struct Stats {
 std::uint64_t accepted=0,routed=0,routedBytes=0,dropped=0;
 std::uint32_t live=0,backends=0,capacity=0;
 std::size_t workingSetBytes=0,privateBytes=0,tableBytes=0;
};
class Gateway {
public:
 Gateway();
 ~Gateway();
 Gateway(Gateway const&)=delete;
 Gateway& operator=(Gateway const&)=delete;
 bool Start(Config const&);
 void Stop();
 std::uint16_t BoundPort() const noexcept;
 Stats Inspect() const;
 bool WaitLive(std::uint32_t count,std::uint32_t milliseconds) const;
 bool WaitRouted(std::uint64_t frames,std::uint32_t milliseconds) const;
 bool WaitRoutedBytes(std::uint64_t bytes,std::uint32_t milliseconds) const;
private:
 struct Impl;
 Impl* impl_;
};
std::size_t ProcessWorkingSetBytes();
std::size_t ProcessPrivateBytes();
}
