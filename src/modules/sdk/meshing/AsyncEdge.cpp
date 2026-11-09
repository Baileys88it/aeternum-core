#include "aeternum/sdk/internal/meshing.hpp"
#include <boost/asio.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <limits>
#include <thread>
#include <stdexcept>
#include <vector>
namespace aeternum::sdk::internal::meshing {
namespace asio=boost::asio;using tcp=asio::ip::tcp;using Error=boost::system::error_code;
namespace {std::mutex generationMutex;std::uint32_t lastGeneration=0;std::mutex mapMutex;Server* mapEdge=nullptr;std::atomic<std::uint64_t> mapNonce{1};}

RoutingTable::RoutingTable(std::uint32_t capacity):slots_(capacity>0 && capacity<=Server::MaxCapacity?capacity:throw std::invalid_argument("routing capacity")) {
 free_.reserve(capacity);for(std::uint32_t n=capacity;n>0;--n)free_.push_back(n-1);
}
bool RoutingTable::Valid(SessionId id) const noexcept {
 auto low=std::uint32_t(id);return low && low<=slots_.size() && slots_[low-1].connection && slots_[low-1].generation==std::uint32_t(id>>32);
}
SessionId RoutingTable::Insert(std::uint32_t backend,std::shared_ptr<void> connection) {
 std::lock_guard<std::mutex> lock(mutex_);if(!connection || free_.empty())return 0;
 std::uint32_t generation;
 {std::lock_guard<std::mutex> serial(generationMutex);
  if(lastGeneration==std::numeric_limits<std::uint32_t>::max())return 0;
  generation=++lastGeneration;
 }
 auto index=free_.back();free_.pop_back();auto& slot=slots_[index];slot.generation=generation;
 slot.connected=false;slot.connection=std::move(connection);++count_;
 auto id=(SessionId(slot.generation)<<32)|(index+1);slot.target.store((SessionId(slot.generation)<<32)|(1u<<8)|(backend+1));slot.identity.store(id,std::memory_order_release);
 return (SessionId(slot.generation)<<32)|(index+1);
}
Route RoutingTable::Lookup(SessionId id) const {
 std::lock_guard<std::mutex> lock(mutex_);if(!Valid(id))return {};
 auto const& s=slots_[std::uint32_t(id)-1];auto target=s.target.load();return {id,(std::uint32_t(target)&255u)-1,s.connected,s.connection,target};
}
bool RoutingTable::Erase(SessionId id) {
 std::lock_guard<std::mutex> lock(mutex_);if(!Valid(id))return false;auto index=std::uint32_t(id)-1;auto& s=slots_[index];
 s.target.store(0);s.identity.store(0);if(s.connected)--connected_;s.connected=false;s.connection.reset();--count_;
 if(s.generation!=std::numeric_limits<std::uint32_t>::max())free_.push_back(index);return true;
}
bool RoutingTable::MarkConnected(SessionId id) {
 std::lock_guard<std::mutex> lock(mutex_);if(!Valid(id))return false;auto& s=slots_[std::uint32_t(id)-1];if(!s.connected){s.connected=true;++connected_;}return true;
}
Route RoutingTable::At(std::uint32_t index) const {
 std::lock_guard<std::mutex> lock(mutex_);if(index>=slots_.size() || !slots_[index].connection)return {};
 auto const& s=slots_[index];auto target=s.target.load();return {(SessionId(s.generation)<<32)|(index+1),(std::uint32_t(target)&255u)-1,s.connected,s.connection,target};
}
std::vector<Route> RoutingTable::Snapshot() const {
 std::lock_guard<std::mutex> lock(mutex_);std::vector<Route> out;out.reserve(count_);
 for(std::uint32_t i=0;i<slots_.size();++i){auto const& s=slots_[i];if(s.connection){auto target=s.target.load();out.push_back({(SessionId(s.generation)<<32)|(i+1),(std::uint32_t(target)&255u)-1,s.connected,{},target});}}
 return out; // no socket owner/raw descriptor escapes introspection
}
std::size_t RoutingTable::Size()const{std::lock_guard<std::mutex> lock(mutex_);return count_;}
std::size_t RoutingTable::Connected()const{std::lock_guard<std::mutex> lock(mutex_);return connected_;}
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,"Handoff requires native lock-free64 atomics; no hidden-lock fallback");
std::uint64_t RoutingTable::NextTarget(SessionId id,std::uint64_t expected,std::uint32_t backend) noexcept {
 auto epoch=std::uint32_t(expected)>>8;
 if(!id || (id>>32)!=(expected>>32) || !epoch || epoch==0xffffffu || backend>=16)return 0;
 return (id&0xffffffff00000000ull)|(std::uint64_t(epoch+1)<<8)|(backend+1);
}
bool RoutingTable::RetargetCAS(SessionId id,std::uint64_t expected,std::uint32_t backend) noexcept {
 auto low=std::uint32_t(id);auto desired=NextTarget(id,expected,backend);
 if(!desired || !low || low>slots_.size())return false;auto& s=slots_[low-1];
 if(s.identity.load(std::memory_order_acquire)!=id)return false;
 // Generation is IN the CAS token: erase/reuse cannot retarget another session.
 return s.target.compare_exchange_strong(expected,desired,std::memory_order_acq_rel);
}
bool RoutingTable::TargetIsLockFree()const noexcept{return slots_[0].target.is_lock_free();}
struct Server::Impl:std::enable_shared_from_this<Impl> {
 struct Connection;
 Config config;asio::io_context io;asio::executor_work_guard<asio::io_context::executor_type> work{io.get_executor()};
 tcp::acceptor listener{io};tcp::socket pending{io};RoutingTable routes;
 std::vector<tcp::endpoint> backends;std::thread worker;std::uint16_t port=0;std::uint32_t nextBackend=0;
 std::atomic<bool> stopping{false};bool stoppedOnIo=false;
 std::mutex observer;std::condition_variable changed;
 std::atomic<std::uint64_t> accepted{0},capacityRejected{0},connectFailed{0},closed{0},c2b{0},b2c{0},reads{0};
 std::atomic<std::uint32_t> peak{0};
 std::atomic<std::uint64_t> handoffs{0},resumed{0},failedHandoffs{0},staleCallbacks{0};
 std::array<std::uint32_t,Server::MaxCapacity> accounts{};
 explicit Impl(Config const& c):config(c),routes(c.capacity){accounts.fill(0);}
 void Notify(){std::lock_guard<std::mutex> lock(observer);changed.notify_all();}
 Snapshot Inspect()const {
  Snapshot s;s.routes=routes.Snapshot();s.accepted=accepted.load();s.capacityRejected=capacityRejected.load();s.connectFailed=connectFailed.load();s.closed=closed.load();s.clientToBackendBytes=c2b.load();s.backendToClientBytes=b2c.load();s.readOperations=reads.load();s.peakChunk=peak.load();s.handoffs=handoffs.load();s.resumed=resumed.load();s.failedHandoffs=failedHandoffs.load();s.staleCallbacks=staleCallbacks.load();return s;
 }
 void Accept();void StopOnIo();
 template<class Predicate>bool Wait(Predicate predicate,std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(observer);return changed.wait_for(lock,timeout,predicate);
 }
};
struct Server::Impl::Connection:std::enable_shared_from_this<Connection> {
 struct Leg {tcp::socket socket;std::uint32_t index;std::uint64_t token=0;Leg(Impl& p,std::uint32_t n):socket(p.io),index(n){}void Close(){Error e;socket.cancel(e);socket.close(e);}};
 Impl& owner;SessionId id=0;tcp::socket client;std::shared_ptr<Leg> active,candidate;
 asio::steady_timer connectTimer,idleTimer,handoffTimer;
 ByteRing<Server::RingBytes> ring;std::array<unsigned char,Server::BufferBytes> clientBytes{},serverBytes{};
 bool closed=false,clientRead=false,upWrite=false,downRead=false,downWrite=false,clientEof=false,backendEof=false,clientFin=false,backendFin=false;
 bool transitioning=false,sealed=false,candidateReady=false,stateWrite=false;HandoffControl control{};
 std::uint64_t prefix=0;
 std::array<unsigned char,128> capturedState{};std::uint16_t capturedStateBytes=0;std::vector<unsigned char> backendCarry;
 std::atomic<bool> controlBusy{false};std::atomic<std::uint64_t> lastNonce{0},observedPrefix{0},observedWraps{0},handoffRead{0};
 std::atomic<std::uint32_t> phase{0},buffered{0},bufferPeak{0};std::atomic<bool> observedClientFin{false},observedClientEof{false},stateDelivered{false};
 std::chrono::steady_clock::time_point activity=std::chrono::steady_clock::now();
 Connection(Impl& p,tcp::socket c,std::uint32_t b):owner(p),client(std::move(c)),active(std::make_shared<Leg>(p,b)),connectTimer(p.io),idleTimer(p.io),handoffTimer(p.io){}
 bool Owns()const{return !closed && owner.routes.Lookup(id).connection.get()==this;}
 void Observe(){buffered.store(std::uint32_t(ring.Size()));observedWraps.store(ring.Wraps());auto peak=bufferPeak.load();while(peak<ring.Size()&&!bufferPeak.compare_exchange_weak(peak,std::uint32_t(ring.Size()))){}owner.Notify();}
 void Stage(HandoffPhase p){phase.store(std::uint32_t(p));owner.Notify();}
 HandoffView View()const{return {HandoffPhase(phase.load()),buffered.load(),bufferPeak.load(),observedWraps.load(),observedPrefix.load(),observedClientFin.load(),handoffRead.load(),observedClientEof.load(),stateDelivered.load(),capturedStateBytes?capturedStateBytes:control.stateBytes};}
 void Close(){
  if(closed)return;closed=true;controlBusy.store(false);Error e;
  try{connectTimer.cancel();idleTimer.cancel();handoffTimer.cancel();}catch(...){}
  client.cancel(e);client.close(e);active->Close();if(candidate)candidate->Close();
  if(id)owner.accounts[std::uint32_t(id)-1]=0;
  owner.routes.Erase(id);++owner.closed;owner.Notify();
 }
 void Start(){
  auto self=shared_from_this();active->token=owner.routes.Lookup(id).target;auto leg=active;
  connectTimer.expires_after(std::chrono::milliseconds(owner.config.connectTimeoutMs));
  connectTimer.async_wait([self](Error e){if(!e&&self->Owns()){++self->owner.connectFailed;self->Close();}});
  leg->socket.async_connect(owner.backends[leg->index],[self,leg](Error e){
   if(!self->Owns())return;self->connectTimer.cancel();if(e){++self->owner.connectFailed;self->Close();return;}
   Error ignored;self->client.set_option(tcp::no_delay(true),ignored);leg->socket.set_option(tcp::no_delay(true),ignored);
   if(self->owner.config.socketBufferBytes){self->client.set_option(asio::socket_base::receive_buffer_size(self->owner.config.socketBufferBytes),ignored);leg->socket.set_option(asio::socket_base::send_buffer_size(self->owner.config.socketBufferBytes),ignored);}
   self->owner.routes.MarkConnected(self->id);self->owner.Notify();self->activity=std::chrono::steady_clock::now();self->Idle();self->ReadClient();self->ReadBackend();
  });
 }
 void Idle(){
  idleTimer.expires_at(activity+std::chrono::milliseconds(owner.config.idleTimeoutMs));auto self=shared_from_this();
  idleTimer.async_wait([self](Error e){if(e||!self->Owns())return;if(std::chrono::steady_clock::now()>=self->activity+std::chrono::milliseconds(self->owner.config.idleTimeoutMs))self->Close();else self->Idle();});
 }
 void Finish(){
  if(!Owns())return;if(transitioning){Progress();return;}Error e;
  if(clientEof&&!ring.Size()&&!upWrite&&!backendFin){active->socket.shutdown(tcp::socket::shutdown_send,e);backendFin=true;}
  if(backendEof&&!downWrite&&!clientFin){client.shutdown(tcp::socket::shutdown_send,e);clientFin=true;observedClientFin.store(true);}
  if(clientEof&&backendEof&&!ring.Size()&&!upWrite&&!downWrite)Close();
 }
 void ReadClient(){
  if(!Owns()||clientRead||clientEof||!ring.Free())return;clientRead=true;auto self=shared_from_this();
  auto amount=(std::min)(ring.Free(),clientBytes.size()); // reserve free space; only this one producer can push
  client.async_read_some(asio::buffer(clientBytes.data(),amount),[self](Error e,std::size_t n){
   if(!self->Owns())return;self->clientRead=false;++self->owner.reads;
   if(n){if(self->transitioning)self->handoffRead.fetch_add(n);if(!self->ring.Push(self->clientBytes.data(),n)){self->Close();return;}self->activity=std::chrono::steady_clock::now();self->Peak(n);self->Observe();}
   if(e){if(e!=asio::error::eof){self->Close();return;}self->clientEof=true;self->observedClientEof.store(true);self->owner.Notify();}
   self->WriteBackend();self->ReadClient();self->Finish();
  });
 }
 void Peak(std::size_t n){auto old=owner.peak.load();while(old<n&&!owner.peak.compare_exchange_weak(old,std::uint32_t(n))){} }
 void WriteBackend(){
  if(!Owns()||upWrite||transitioning||!ring.Size())return;
  auto leg=active;if(owner.routes.Lookup(id).target!=leg->token){Close();return;}
  upWrite=true;auto self=shared_from_this();auto n=(std::min)(ring.Contiguous(),std::size_t(Server::BufferBytes));
  asio::async_write(leg->socket,asio::buffer(ring.Data(),n),[self,leg](Error e,std::size_t written){
   if(!self->Owns())return;
   if(self->active!=leg||self->owner.routes.Lookup(self->id).target!=leg->token){++self->owner.staleCallbacks;return;}
   self->upWrite=false;self->owner.c2b.fetch_add(written);self->prefix+=written;
   if(!self->ring.Pop(written)){self->Close();return;}self->Observe();
   if(e){self->Close();return;}self->activity=std::chrono::steady_clock::now();self->WriteBackend();self->ReadClient();self->Finish();
  });
 }
 bool CaptureState(unsigned char const* p,std::size_t n){
  backendCarry.insert(backendCarry.end(),p,p+n);
  if(backendCarry.size()<8)return true;
  std::uint32_t magic=0;std::memcpy(&magic,backendCarry.data(),4);
  if(magic!=StateMagic)return false;
  std::uint16_t sz=0;std::memcpy(&sz,backendCarry.data()+4,2);
  if(!sz||sz>capturedState.size())return false;
  if(backendCarry.size()<std::size_t(8)+sz)return true;
  std::memcpy(capturedState.data(),backendCarry.data()+8,sz);capturedStateBytes=sz;
  backendCarry.erase(backendCarry.begin(),backendCarry.begin()+8+sz);owner.Notify();return true;
 }
 void ForwardBackend(std::shared_ptr<Leg> leg,Error e,unsigned char const* p,std::size_t n){
  Peak(n);activity=std::chrono::steady_clock::now();downWrite=true;auto self=shared_from_this();
  std::memcpy(serverBytes.data(),p,(std::min)(n,serverBytes.size()));
  asio::async_write(client,asio::buffer(serverBytes.data(),n),[self,leg,e](Error writeError,std::size_t written){
   if(!self->Owns())return;if(self->active!=leg){++self->owner.staleCallbacks;return;}
   self->downWrite=false;self->owner.b2c.fetch_add(written);if(writeError){self->Close();return;}
   self->activity=std::chrono::steady_clock::now();if(e)self->EndBackend(e);else self->ReadBackend();
  });
 }
 void ReadBackend(){
  if(!Owns()||downRead||downWrite||backendEof)return;downRead=true;auto self=shared_from_this();auto leg=active;
  leg->socket.async_read_some(asio::buffer(serverBytes),[self,leg](Error e,std::size_t n){
   if(!self->Owns())return;if(self->active!=leg){++self->owner.staleCallbacks;return;}
   self->downRead=false;++self->owner.reads;
   if(!n){self->EndBackend(e);return;}
   if(self->transitioning && self->CaptureState(self->serverBytes.data(),n)){
    self->activity=std::chrono::steady_clock::now();self->Peak(n);
    if(e)self->EndBackend(e);else self->ReadBackend();self->Progress();return;
   }
   if(!self->backendCarry.empty()){
    auto carry=std::move(self->backendCarry);self->backendCarry.clear();
    self->ForwardBackend(leg,e,carry.data(),carry.size());return;
   }
   self->ForwardBackend(leg,e,self->serverBytes.data(),n);
  });
 }
 void EndBackend(Error e){if(e!=asio::error::eof){Close();return;}backendEof=true;Finish();}
 void Timer(std::uint32_t ms){
  handoffTimer.expires_after(std::chrono::milliseconds(ms));auto self=shared_from_this();auto nonce=control.nonce;
  handoffTimer.async_wait([self,nonce](Error e){if(e||!self->Owns()||!self->transitioning||self->control.nonce!=nonce)return;
   if(self->sealed){++self->owner.failedHandoffs;self->Stage(HandoffPhase::FailedClosed);self->Close();}else self->Resume();
  });
 }
 void Resume(){
  if(!Owns())return;if(candidate)candidate->Close();candidate.reset();transitioning=false;candidateReady=false;
  handoffTimer.cancel();++owner.resumed;Stage(HandoffPhase::Resumed);controlBusy.store(false);
  WriteBackend();ReadClient();Finish(); // A send side still healthy; preserve every queued byte
 }
 void Begin(HandoffControl c){
  if(!Owns()||backendFin||clientFin||transitioning||owner.routes.Lookup(id).target!=c.expectedTarget){controlBusy.store(false);Stage(HandoffPhase::Rejected);return;}
  control=c;transitioning=true;sealed=false;candidateReady=false;Stage(HandoffPhase::Connecting);
  candidate=std::make_shared<Leg>(owner,c.backend);candidate->token=RoutingTable::NextTarget(id,c.expectedTarget,c.backend);
  if(!candidate->token){Resume();return;}auto self=shared_from_this();auto leg=candidate;Timer(owner.config.connectTimeoutMs);
  leg->socket.async_connect(owner.backends[c.backend],[self,leg](Error e){
   if(!self->Owns()||self->candidate!=leg||!self->transitioning)return;
   if(e){self->Resume();return;}Error ignored;leg->socket.set_option(tcp::no_delay(true),ignored);
   if(self->owner.config.socketBufferBytes)leg->socket.set_option(asio::socket_base::send_buffer_size(self->owner.config.socketBufferBytes),ignored);
   self->candidateReady=true;self->Timer(self->owner.config.handoffTimeoutMs);self->Progress();
  });
  ReadClient(); // while forwarding is paused, continue bounded reads until ring is full
 }
 void Progress(){
  if(!transitioning||!candidateReady||upWrite)return;
  if(!sealed){Error e;active->socket.shutdown(tcp::socket::shutdown_send,e);
   if(e){++owner.failedHandoffs;Stage(HandoffPhase::FailedClosed);Close();return;}
   sealed=true;observedPrefix.store(prefix);Stage(HandoffPhase::Draining);
  }
  // No old up-write, no old response read/write, and old EOF consumed after tail
  // flush. B was already connected but has not been allowed to read/write payload.
  if(!backendEof||downRead||downWrite)return;
  if(!owner.routes.RetargetCAS(id,control.expectedTarget,control.backend)){++owner.failedHandoffs;Stage(HandoffPhase::FailedClosed);Close();return;}
  auto old=active;active=std::move(candidate);old->Close();handoffTimer.cancel();
  transitioning=false;candidateReady=false;backendEof=false;backendFin=false;prefix=0;
  ++owner.handoffs;Stage(HandoffPhase::Committed);controlBusy.store(false);
  WriteStateThenFlush();
 }
 void WriteStateThenFlush(){
  auto n=capturedStateBytes?capturedStateBytes:control.stateBytes;
  auto src=capturedStateBytes?capturedState.data():control.state.data();
  if(!n){ReadBackend();WriteBackend();ReadClient();Finish();return;}
  if(stateWrite)return;
  auto frame=std::make_shared<std::array<unsigned char,8+128>>();std::uint32_t magic=StateMagic;std::uint16_t bytes=n,pad=0;
  std::memcpy(frame->data(),&magic,4);std::memcpy(frame->data()+4,&bytes,2);std::memcpy(frame->data()+6,&pad,2);std::memcpy(frame->data()+8,src,n);
  stateWrite=true;auto self=shared_from_this();auto leg=active;auto total=std::size_t(8)+n;
  asio::async_write(leg->socket,asio::buffer(frame->data(),total),[self,leg,total,frame](Error e,std::size_t written){
   if(!self->Owns())return;if(self->active!=leg){++self->owner.staleCallbacks;return;}
   self->stateWrite=false;if(e||written!=total){self->Close();return;}
   self->stateDelivered.store(true);self->owner.Notify();
   self->ReadBackend();self->WriteBackend();self->ReadClient();self->Finish();
  });
 }
};
void Server::Impl::Accept() {
 if(stopping.load())return;auto self=shared_from_this();
 listener.async_accept(pending,[self](Error e){
  if(self->stopping.load())return;
  if(e){self->StopOnIo();return;}
  {
   if(self->routes.Size()>=self->config.capacity){++self->capacityRejected;Error ignored;self->pending.close(ignored);self->Notify();}
   else {
    auto target=self->nextBackend++ % self->backends.size();
    auto connection=std::make_shared<Connection>(*self,std::move(self->pending),std::uint32_t(target));
    connection->id=self->routes.Insert(std::uint32_t(target),connection);
    if(connection->id)connection->Start();else {++self->capacityRejected;connection->Close();}
   }
  }
  ++self->accepted;self->Notify();
  self->pending=tcp::socket(self->io);self->Accept();
 });
}
void Server::Impl::StopOnIo() {
 if(stoppedOnIo)return;stoppedOnIo=true;stopping.store(true);Error e;listener.cancel(e);listener.close(e);pending.cancel(e);pending.close(e);
 for(std::uint32_t i=0;i<config.capacity;++i)if(auto r=routes.At(i);r.connection)std::static_pointer_cast<Connection>(r.connection)->Close();
 work.reset();Notify(); // run() drains cancelled handlers before owner joins/destroys io/sockets
}
Server::~Server(){Stop();}
bool Server::Start(Config const& c) {
 std::lock_guard<std::mutex> lock(lifecycle_);if(impl_ || !c.enabled || !c.capacity || c.capacity>MaxCapacity || c.backends.empty() || c.backends.size()>16 || !c.connectTimeoutMs || c.connectTimeoutMs>60000 || !c.idleTimeoutMs || c.idleTimeoutMs>3600000 || !c.handoffTimeoutMs || c.handoffTimeoutMs>60000 || c.transitions.size()>64 || c.socketBufferBytes>1048576)return false;
 std::shared_ptr<Impl> state;
 try {
  if(c.transportHandoff && c.transitions.empty())return false;
  for(auto const& r:c.transitions)if(r.from>=c.backends.size()||r.to>=c.backends.size()||r.from==r.to)return false;
  state=std::make_shared<Impl>(c);Error e;auto bind=asio::ip::make_address(c.bind,e);if(e)return false;
  for(auto const& b:c.backends){auto ip=asio::ip::make_address(b.address,e);if(e || !b.port || ip.is_unspecified() || ip.is_multicast())return false;state->backends.emplace_back(ip,b.port);}
  state->listener.open(bind.is_v6()?tcp::v6():tcp::v4(),e);if(e)return false;
  state->listener.bind({bind,c.port},e);if(e)return false;state->listener.listen(int((std::min)(c.capacity,128u)),e);if(e)return false;
  state->port=state->listener.local_endpoint().port();state->Accept();
  state->worker=std::thread([state]{
   try {state->io.run();}catch(...) {state->StopOnIo();try{state->io.run();}catch(...){}}
  });stoppedSnapshot_={};impl_=std::move(state);return true;
 }catch(...){if(state){state->StopOnIo();state->io.run();}return false;}
}
void Server::Stop() noexcept {
 try {
  DetachMapChangeEdge(this);
  std::unique_lock<std::mutex> lock(lifecycle_);auto state=impl_;if(!state)return;
  if(!state->stopping.exchange(true)) {
   try{asio::post(state->io,[state]{state->StopOnIo();});}
   catch(...){state->io.stop();} // allocation failure: owner joins before synchronous cleanup
  }
  if(state->worker.get_id()==std::this_thread::get_id())return; // owner must later join; never self-join
  if(state->worker.joinable())state->worker.join();
  state->StopOnIo();state->io.restart();state->io.run(); // drain every cancelled shared-owner handler
  stoppedSnapshot_=state->Inspect();impl_.reset();
 }catch(...){/* no exception crosses core teardown */}
}
std::uint16_t Server::BoundPort()const{std::lock_guard<std::mutex> lock(lifecycle_);return impl_?impl_->port:0;}
Snapshot Server::Inspect()const{std::lock_guard<std::mutex> lock(lifecycle_);return impl_?impl_->Inspect():stoppedSnapshot_;}
bool Server::HasRoute(SessionId id)const{std::lock_guard<std::mutex> lock(lifecycle_);return impl_ && bool(impl_->routes.Lookup(id).connection);}
bool Server::WaitForConnections(std::size_t count,std::chrono::milliseconds timeout)const {
 std::shared_ptr<Impl> s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s && s->Wait([&]{return s->routes.Connected()==count;},timeout);
}
bool Server::WaitForIdle(std::uint64_t accepted,std::chrono::milliseconds timeout)const {
 std::shared_ptr<Impl> s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s && s->Wait([&]{return s->accepted.load()>=accepted && s->routes.Size()==0;},timeout);
}
bool Server::WaitForRejections(std::uint64_t count,std::chrono::milliseconds timeout)const {
 std::shared_ptr<Impl> s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s && s->Wait([&]{return s->capacityRejected.load()>=count;},timeout);
}
ControlResult Server::RequestHandoff(HandoffControl const& c){
 std::lock_guard<std::mutex> lock(lifecycle_);auto s=impl_;
 if(!s||s->stopping.load())return ControlResult::Stopping;
 if(!s->config.transportHandoff)return ControlResult::Disabled;
 auto route=s->routes.Lookup(c.session);if(!route.connection)return ControlResult::UnknownSession;
 if(!route.connected)return ControlResult::Busy; // bootstrap connect is not an adoptable A
 if(route.target!=c.expectedTarget)return ControlResult::StaleRoute;
 bool allowed=false;for(auto const& r:s->config.transitions)if(r.from==route.backend&&r.to==c.backend&&r.map==c.map&&r.zone==c.zone)allowed=true;
 if(!allowed||!RoutingTable::NextTarget(c.session,c.expectedTarget,c.backend))return ControlResult::InvalidMapping;
 auto session=std::static_pointer_cast<Impl::Connection>(route.connection);bool expected=false;
 if(!session->controlBusy.compare_exchange_strong(expected,true))return ControlResult::Busy;
 auto previous=session->lastNonce.load();if(!c.nonce||c.nonce<=previous){session->controlBusy.store(false);return ControlResult::Replay;}
 session->lastNonce.store(c.nonce);session->Stage(HandoffPhase::Queued);
 try{asio::post(s->io,[session,c]{session->Begin(c);});}catch(...){session->controlBusy.store(false);return ControlResult::Stopping;}
 return ControlResult::Accepted;
}
HandoffView Server::InspectHandoff(SessionId id)const{
 std::lock_guard<std::mutex> lock(lifecycle_);if(!impl_)return {};auto r=impl_->routes.Lookup(id);
 return r.connection?std::static_pointer_cast<Impl::Connection>(r.connection)->View():HandoffView{};
}
bool Server::WaitForHandoffs(std::uint64_t n,std::chrono::milliseconds timeout)const{
 std::shared_ptr<Impl>s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s&&s->Wait([&]{return s->handoffs.load()>=n;},timeout);
}
bool Server::WaitForResumes(std::uint64_t n,std::chrono::milliseconds timeout)const{
 std::shared_ptr<Impl>s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s&&s->Wait([&]{return s->resumed.load()>=n;},timeout);
}
bool Server::WaitForBuffered(SessionId id,std::uint32_t n,std::chrono::milliseconds timeout)const{
 std::shared_ptr<Impl>s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s&&s->Wait([&]{auto r=s->routes.Lookup(id);return r.connection&&std::static_pointer_cast<Impl::Connection>(r.connection)->buffered.load()>=n;},timeout);
}

bool Server::WaitForClientEof(SessionId id,std::chrono::milliseconds timeout)const{
 std::shared_ptr<Impl>s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s&&s->Wait([&]{auto r=s->routes.Lookup(id);return r.connection&&std::static_pointer_cast<Impl::Connection>(r.connection)->observedClientEof.load();},timeout);
}
bool Server::WaitForStateDelivered(SessionId id,std::chrono::milliseconds timeout)const{
 std::shared_ptr<Impl>s;{std::lock_guard<std::mutex> lock(lifecycle_);s=impl_;}return s&&s->Wait([&]{auto r=s->routes.Lookup(id);return r.connection&&std::static_pointer_cast<Impl::Connection>(r.connection)->stateDelivered.load();},timeout);
}
bool Server::BindAccount(std::uint32_t account,SessionId id){
 std::lock_guard<std::mutex> lock(lifecycle_);if(!impl_||!account||!id)return false;
 auto r=impl_->routes.Lookup(id);if(!r.connection)return false;
 for(auto a:impl_->accounts)if(a==account)return false;
 impl_->accounts[std::uint32_t(id)-1]=account;return true;
}
bool Server::BindAccountToLatest(std::uint32_t account){
 std::lock_guard<std::mutex> lock(lifecycle_);if(!impl_||!account)return false;
 for(auto a:impl_->accounts)if(a==account)return true;
 SessionId latest=0;
 for(std::uint32_t i=0;i<impl_->config.capacity;++i){auto r=impl_->routes.At(i);if(r.connection&&!impl_->accounts[std::uint32_t(r.id)-1]&&r.id>latest)latest=r.id;}
 if(!latest)return false;impl_->accounts[std::uint32_t(latest)-1]=account;return true;
}
bool Server::BindAttachedAccount(std::uint32_t account){
 Server* edge=nullptr;{std::lock_guard<std::mutex> lock(mapMutex);edge=mapEdge;}
 return edge&&edge->BindAccountToLatest(account);
}
void Server::AttachMapChangeEdge(Server* s){std::lock_guard<std::mutex> lock(mapMutex);mapEdge=s;}
void Server::DetachMapChangeEdge(Server* s){std::lock_guard<std::mutex> lock(mapMutex);if(mapEdge==s)mapEdge=nullptr;}
ControlResult Server::NotifyMapChange(std::uint32_t account,std::uint32_t map,std::uint32_t zone,unsigned char const* state,std::uint16_t stateBytes){
 Server* edge=nullptr;{std::lock_guard<std::mutex> lock(mapMutex);edge=mapEdge;}
 if(!edge||!account)return ControlResult::Disabled;
 HandoffControl c{};c.map=map;c.zone=zone;c.account=account;
 if(stateBytes){if(!state||stateBytes>c.state.size())return ControlResult::InvalidMapping;std::memcpy(c.state.data(),state,stateBytes);c.stateBytes=stateBytes;}
 {
  std::lock_guard<std::mutex> lock(edge->lifecycle_);auto s=edge->impl_;if(!s)return ControlResult::Stopping;
  SessionId id=0;Route route{};
  for(std::uint32_t i=0;i<s->config.capacity;++i)if(s->accounts[i]==account){route=s->routes.At(i);id=route.id;break;}
  if(!id||!route.connection)return ControlResult::UnknownSession;
  bool allowed=false;for(auto const& r:s->config.transitions)if(r.from==route.backend&&r.map==map&&r.zone==zone){allowed=true;c.backend=r.to;break;}
  if(!allowed)return ControlResult::InvalidMapping;
  c.session=id;c.expectedTarget=route.target;c.nonce=mapNonce.fetch_add(1);
 }
 return edge->RequestHandoff(c);
}

}
