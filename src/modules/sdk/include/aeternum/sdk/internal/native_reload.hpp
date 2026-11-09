#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
namespace aeternum::sdk::internal::native {
// Every pointer-table read is leased BEFORE its first non-atomic read. Nested
// crossings inherit their root lease, including sends/HostAlloc/telemetry calls.
class NativeReadLease {
 bool active_=false,root_=false;
public:
 NativeReadLease() noexcept;~NativeReadLease();explicit operator bool()const noexcept{return active_;}
 NativeReadLease(NativeReadLease const&)=delete;NativeReadLease& operator=(NativeReadLease const&)=delete;
};
// Nonblocking: closes admission; false means defer to a later global tick.
bool TryNativeWrite() noexcept;
void EndNativeWrite() noexcept;
void CancelNativePause() noexcept;
class NativeWriteScope {public:~NativeWriteScope(){EndNativeWrite();}};
// Windows ReadDirectoryChangesW, recursive. One bounded/coalesced dirty bit;
// watcher never reads/calls a module, allocates HostAlloc or captures Trinity.
class NativeFileWatcher {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 NativeFileWatcher();~NativeFileWatcher();bool Start(std::filesystem::path const& root);void Stop();
 bool TakeDirty();bool WaitDirty(std::chrono::milliseconds timeout); // deterministic test/event wait
};
}
