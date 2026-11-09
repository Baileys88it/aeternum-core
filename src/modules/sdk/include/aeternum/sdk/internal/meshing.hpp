#pragma once
#include <chrono>
#include <atomic>
#include <array>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
namespace aeternum::sdk::internal::meshing {
using SessionId=std::uint64_t;
struct Route {SessionId id=0;std::uint32_t backend=0;bool connected=false;std::shared_ptr<void> connection;std::uint64_t target=0;};
// Fixed slots/free-stack. Insert/Lookup/Erase are worst-case O(1) table work,
// excluding mutex scheduling/socket lifetime costs. Snapshot is explicitly O(cap).
// ID = process-unique 32-bit generation + 32-bit slot+1. Also rejects stale IDs
// across table/server restarts within this process. Generation exhaustion refuses
// admission, never wraps. IDs are not persisted/authenticated game session IDs.
class RoutingTable {
 struct Slot {std::uint32_t generation=0;bool connected=false;std::shared_ptr<void> connection;std::atomic<std::uint64_t> identity{0},target{0};};
 mutable std::mutex mutex_;std::vector<Slot> slots_;std::vector<std::uint32_t> free_;
 std::size_t count_=0,connected_=0;
 bool Valid(SessionId) const noexcept;
public:
 explicit RoutingTable(std::uint32_t capacity);
 SessionId Insert(std::uint32_t backend,std::shared_ptr<void> connection);
 Route Lookup(SessionId) const;bool Erase(SessionId);bool MarkConnected(SessionId);
 Route At(std::uint32_t index) const;std::vector<Route> Snapshot() const;
 std::size_t Size() const;std::size_t Connected() const;
 // ONLY publication is lock-free. Lifetime/lookup/admission/observers still lock.
 bool RetargetCAS(SessionId,std::uint64_t expected,std::uint32_t backend) noexcept;
 bool TargetIsLockFree() const noexcept;
 static std::uint64_t NextTarget(SessionId,std::uint64_t expected,std::uint32_t backend) noexcept;
};
// Fixed, non-overwriting byte ring. Caller is the one Asio owner thread; an
// in-flight write retains its head span until Pop after composed write completion.
template<std::size_t N> class ByteRing {
 std::array<unsigned char,N> data_{};std::size_t head_=0,size_=0,wraps_=0;
public:
 std::size_t Size()const{return size_;}std::size_t Free()const{return N-size_;}std::size_t Wraps()const{return wraps_;}
 bool Push(unsigned char const* data,std::size_t n){if(n>Free())return false;auto tail=(head_+size_)%N;auto first=(std::min)(n,N-tail);std::memcpy(data_.data()+tail,data,first);std::memcpy(data_.data(),data+first,n-first);if(n && tail+n>=N)++wraps_;size_+=n;return true;}
 unsigned char const* Data()const{return data_.data()+head_;}
 std::size_t Contiguous()const{return (std::min)(size_,N-head_);}
 bool Pop(std::size_t n){if(n>size_)return false;head_=(head_+n)%N;size_-=n;return true;}
};
struct TransitionRule {std::uint32_t from=0,map=0,zone=0,to=0;};
constexpr std::uint32_t StateMagic=0x31485341u; // ASH1 internal backend frame, never client-forwarded
struct HandoffControl {
 SessionId session=0;std::uint64_t expectedTarget=0,nonce=0;std::uint32_t map=0,zone=0,backend=0,account=0;
 std::array<unsigned char,128> state{};std::uint16_t stateBytes=0;
};
enum class ControlResult {Accepted,Disabled,UnknownSession,StaleRoute,InvalidMapping,Replay,Busy,Stopping};
enum class HandoffPhase:std::uint32_t {None,Queued,Connecting,Draining,Committed,Resumed,FailedClosed,Rejected};
struct HandoffView {HandoffPhase phase=HandoffPhase::None;std::uint32_t buffered=0,peak=0;std::uint64_t wraps=0,oldPrefix=0;bool clientFinSent=false;std::uint64_t readDuringHandoff=0;bool clientInputEof=false;bool stateDelivered=false;std::uint16_t stateBytes=0;};
struct Backend {std::string address;std::uint16_t port=0;};
struct Config {
 bool enabled=false;std::string bind="127.0.0.1";std::uint16_t port=0;
 std::uint32_t capacity=128,connectTimeoutMs=2000,idleTimeoutMs=30000;
 bool transportHandoff=false;std::uint32_t handoffTimeoutMs=5000,socketBufferBytes=0; //0 keeps OS defaults
 std::vector<TransitionRule> transitions; // trusted host map/zone/backend allow-list, max64
 std::vector<Backend> backends; // trusted operator mapping ONLY; numeric IPs, max16
};
struct Snapshot {
 std::vector<Route> routes;
 std::uint64_t accepted=0,capacityRejected=0,connectFailed=0,closed=0;
 std::uint64_t clientToBackendBytes=0,backendToClientBytes=0,readOperations=0;
 std::uint64_t handoffs=0,resumed=0,failedHandoffs=0,staleCallbacks=0;
 std::uint32_t peakChunk=0; // at most BufferBytes; one outstanding write/direction
};
class Server {
 struct Impl;std::shared_ptr<Impl> impl_;mutable std::mutex lifecycle_;Snapshot stoppedSnapshot_;
public:
 static constexpr std::uint32_t BufferBytes=16384,MaxCapacity=1024,RingBytes=65536;
 Server()=default;~Server();Server(Server const&)=delete;Server& operator=(Server const&)=delete;
 bool Start(Config const&);void Stop() noexcept;std::uint16_t BoundPort() const;
 Snapshot Inspect() const;bool HasRoute(SessionId) const;
 // Trusted in-process injection ONLY. Accepted means queued, not completed.
 // Optional ASH1 state is written to B before client ring flush. MapChange bind is opt-in.
 ControlResult RequestHandoff(HandoffControl const&);
 bool BindAccount(std::uint32_t account,SessionId);
 bool BindAccountToLatest(std::uint32_t account);
 static bool BindAttachedAccount(std::uint32_t account);
 static void AttachMapChangeEdge(Server*);
 static void DetachMapChangeEdge(Server*);
 static ControlResult NotifyMapChange(std::uint32_t account,std::uint32_t map,std::uint32_t zone,unsigned char const* state,std::uint16_t stateBytes);
 HandoffView InspectHandoff(SessionId) const;
 bool WaitForClientEof(SessionId,std::chrono::milliseconds) const;
 bool WaitForStateDelivered(SessionId,std::chrono::milliseconds) const;
 bool WaitForHandoffs(std::uint64_t,std::chrono::milliseconds) const;
 bool WaitForResumes(std::uint64_t,std::chrono::milliseconds) const;
 bool WaitForBuffered(SessionId,std::uint32_t,std::chrono::milliseconds) const;
 bool WaitForConnections(std::size_t,std::chrono::milliseconds) const;
 bool WaitForIdle(std::uint64_t acceptedAtLeast,std::chrono::milliseconds) const;
 bool WaitForRejections(std::uint64_t,std::chrono::milliseconds) const;
};
// Opaque TCP plus optional ASH1 state frame on the backend link. SessionId is this
// host's connection ID until BindAccount. Stop cancels outstanding I/O.
}
