#include "aeternum/gateway/iocp_gateway.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")
#pragma comment(lib,"psapi.lib")
namespace aeternum::gateway {
namespace {
std::atomic<int> gWinsock{0};
bool WinsockHold() {
 if(gWinsock.fetch_add(1,std::memory_order_acq_rel)==0) {
  WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)!=0){gWinsock.fetch_sub(1,std::memory_order_acq_rel);return false;}
 }
 return true;
}
void WinsockRelease(){if(gWinsock.fetch_sub(1,std::memory_order_acq_rel)==1)WSACleanup();}
SOCKET NewIocpSocket(){return WSASocketW(AF_INET,SOCK_STREAM,IPPROTO_TCP,nullptr,0,WSA_FLAG_OVERLAPPED);}
bool Nodelay(SOCKET s){BOOL on=TRUE;return setsockopt(s,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<char*>(&on),sizeof(on))==0;}
bool ParseIpv4(char const* text,std::uint16_t port,sockaddr_in& out) {
 std::memset(&out,0,sizeof(out));out.sin_family=AF_INET;out.sin_port=htons(port);
 return text && port && InetPtonA(AF_INET,text,&out.sin_addr)==1;
}
bool SendAll(SOCKET s,char const* data,int n) {
 int sent=0;while(sent<n){int rc=send(s,data+sent,n-sent,0);if(rc<=0)return false;sent+=rc;}return true;
}
}
std::size_t ProcessWorkingSetBytes() {
 PROCESS_MEMORY_COUNTERS_EX c{};c.cb=sizeof(c);
 if(!GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c),sizeof(c)))return 0;
 return static_cast<std::size_t>(c.WorkingSetSize);
}
std::size_t ProcessPrivateBytes() {
 PROCESS_MEMORY_COUNTERS_EX c{};c.cb=sizeof(c);
 if(!GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c),sizeof(c)))return 0;
 return static_cast<std::size_t>(c.PrivateUsage);
}
enum class Kind:std::uint32_t {Accept=1,ClientRecv=2};
struct Overlap {OVERLAPPED ov{};Kind kind=Kind::Accept;std::uint32_t slot=0;SOCKET accept=INVALID_SOCKET;char addr[64]{};};
struct Conn {
 SOCKET sock=INVALID_SOCKET;Overlap recv{};char buf[kRecvBytes]{};
 std::uint32_t backend=0;std::atomic<bool> live{false};
};
struct Link {SOCKET sock=INVALID_SOCKET;std::unique_ptr<std::mutex> send=std::make_unique<std::mutex>();std::thread drain;};
struct Gateway::Impl {
 Config cfg{};HANDLE iocp=nullptr;SOCKET listen=INVALID_SOCKET;LPFN_ACCEPTEX acceptEx=nullptr;
 std::unique_ptr<Conn[]> table;std::uint32_t slots=0;std::vector<std::uint32_t> free;std::unique_ptr<Link[]> links;std::vector<std::thread> workers;
 std::mutex tableMutex;std::atomic<bool> stop{false};std::atomic<std::uint16_t> bound{0};
 std::atomic<std::uint64_t> accepted{0},routed{0},routedBytes{0},dropped{0};std::atomic<std::uint32_t> live{0};
 std::size_t tableBytes=0;
 void CloseSock(SOCKET& s){if(s==INVALID_SOCKET)return;shutdown(s,SD_BOTH);CancelIoEx(reinterpret_cast<HANDLE>(s),nullptr);closesocket(s);s=INVALID_SOCKET;}
 void Recycle(std::uint32_t slot) {
  if(slot>=slots)return;auto& c=table[slot];
  if(!c.live.exchange(false,std::memory_order_acq_rel))return;
  CloseSock(c.sock);live.fetch_sub(1,std::memory_order_acq_rel);
  std::lock_guard<std::mutex> lock(tableMutex);free.push_back(slot);
 }
 bool PostRecv(Conn& c,std::uint32_t slot) {
  std::memset(&c.recv.ov,0,sizeof(c.recv.ov));c.recv.kind=Kind::ClientRecv;c.recv.slot=slot;
  WSABUF buf{kRecvBytes,c.buf};DWORD flags=0;
  return WSARecv(c.sock,&buf,1,nullptr,&flags,&c.recv.ov,nullptr)==0 || WSAGetLastError()==WSA_IO_PENDING;
 }
 bool PostAccept() {
  if(stop.load(std::memory_order_acquire)||!acceptEx)return false;
  SOCKET s=NewIocpSocket();if(s==INVALID_SOCKET)return false;
  auto* ctx=new Overlap{};ctx->kind=Kind::Accept;ctx->accept=s;DWORD recvd=0;
  if(acceptEx(listen,s,ctx->addr,0,32,32,&recvd,&ctx->ov)==TRUE || WSAGetLastError()==WSA_IO_PENDING)return true;
  closesocket(s);delete ctx;return false;
 }
 bool Tunnel(std::uint32_t slot,char const* data,std::uint32_t n) {
  if(!n||n>kRecvBytes||slot>=slots){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
  auto backend=table[slot].backend;
  if(backend>=cfg.backendCount||links[backend].sock==INVALID_SOCKET){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
  FrameHeader header{kMagic,slot+1,backend,n};
  char packed[sizeof(FrameHeader)+kRecvBytes];
  std::memcpy(packed,&header,sizeof(header));std::memcpy(packed+sizeof(header),data,n);
  std::lock_guard<std::mutex> lock(*links[backend].send);
  if(!SendAll(links[backend].sock,packed,static_cast<int>(sizeof(header)+n))){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
  routed.fetch_add(1,std::memory_order_relaxed);routedBytes.fetch_add(n,std::memory_order_relaxed);return true;
 }
 void OnAccept(Overlap* ctx,bool ok) {
  SOCKET s=ctx->accept;delete ctx;
  if(!ok||stop.load(std::memory_order_acquire)){if(s!=INVALID_SOCKET)closesocket(s);if(!stop.load(std::memory_order_acquire))PostAccept();return;}
  setsockopt(s,SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,reinterpret_cast<char*>(&listen),sizeof(listen));Nodelay(s);
  if(!CreateIoCompletionPort(reinterpret_cast<HANDLE>(s),iocp,static_cast<ULONG_PTR>(Kind::ClientRecv),0)){closesocket(s);PostAccept();return;}
  std::uint32_t slot=0;
  {std::lock_guard<std::mutex> lock(tableMutex);if(free.empty()){closesocket(s);dropped.fetch_add(1,std::memory_order_relaxed);PostAccept();return;}slot=free.back();free.pop_back();}
  auto& c=table[slot];c.sock=s;c.backend=cfg.backendCount?slot%cfg.backendCount:0;c.live.store(true,std::memory_order_release);
  live.fetch_add(1,std::memory_order_relaxed);accepted.fetch_add(1,std::memory_order_relaxed);
  if(!PostRecv(c,slot))Recycle(slot);
  PostAccept();
 }
 void Worker() {
  for(;;) {
   DWORD bytes=0;ULONG_PTR key=0;OVERLAPPED* ov=nullptr;
   auto ok=GetQueuedCompletionStatus(iocp,&bytes,&key,&ov,200);
   if(stop.load(std::memory_order_acquire) && !ov)break;
   if(!ov)continue;
   auto* ctx=reinterpret_cast<Overlap*>(ov);
   if(ctx->kind==Kind::Accept){OnAccept(ctx,ok==TRUE);continue;}
   auto slot=ctx->slot;if(slot>=slots)continue;
   if(!ok||bytes==0||stop.load(std::memory_order_acquire)){Recycle(slot);continue;}
   auto& c=table[slot];if(!Tunnel(slot,c.buf,bytes)){Recycle(slot);continue;}
   if(!PostRecv(c,slot))Recycle(slot);
  }
 }
 bool ConnectBackends() {
  links=std::make_unique<Link[]>(cfg.backendCount);
  for(std::uint32_t i=0;i<cfg.backendCount;++i) {
   sockaddr_in addr{};if(!ParseIpv4(cfg.backends[i].address,cfg.backends[i].port,addr))return false;
   SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET)return false;Nodelay(s);
   if(connect(s,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0){closesocket(s);return false;}
   links[i].sock=s;
   links[i].drain=std::thread([this,i]{
    char buf[4096];
    while(!stop.load(std::memory_order_acquire)) {
     int n=recv(links[i].sock,buf,sizeof(buf),0);if(n<=0)break;
    }
   });
  }
  return true;
 }
};
Gateway::Gateway():impl_(nullptr){}
Gateway::~Gateway(){Stop();}
bool Gateway::Start(Config const& config) {
 Stop();
 if(!config.enabled||!config.port||!config.capacity||config.capacity>kMaxCapacity||!config.backendCount||
    config.backendCount>kMaxBackends||!config.workers||config.workers>64||!config.acceptDepth||config.acceptDepth>1024)
  return false;
 if(!WinsockHold())return false;
 auto* impl=new Impl{};impl->cfg=config;impl->slots=config.capacity;impl->table=std::make_unique<Conn[]>(config.capacity);impl->free.reserve(config.capacity);
 for(std::uint32_t i=0;i<config.capacity;++i)impl->free.push_back(config.capacity-1-i);
 impl->tableBytes=static_cast<std::size_t>(config.capacity)*sizeof(Conn)+static_cast<std::size_t>(config.backendCount)*sizeof(Link);
 impl->iocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE,nullptr,0,0);
 if(!impl->iocp){delete impl;WinsockRelease();return false;}
 sockaddr_in addr{};if(!ParseIpv4(config.bind,config.port,addr)){CloseHandle(impl->iocp);delete impl;WinsockRelease();return false;}
 impl->listen=NewIocpSocket();BOOL reuse=TRUE;setsockopt(impl->listen,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<char*>(&reuse),sizeof(reuse));
 if(impl->listen==INVALID_SOCKET||bind(impl->listen,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0||listen(impl->listen,SOMAXCONN)!=0) {
  impl->CloseSock(impl->listen);CloseHandle(impl->iocp);delete impl;WinsockRelease();return false;
 }
 GUID guid=WSAID_ACCEPTEX;DWORD bytes=0;
 if(WSAIoctl(impl->listen,SIO_GET_EXTENSION_FUNCTION_POINTER,&guid,sizeof(guid),&impl->acceptEx,sizeof(impl->acceptEx),&bytes,nullptr,nullptr)!=0) {
  impl->CloseSock(impl->listen);CloseHandle(impl->iocp);delete impl;WinsockRelease();return false;
 }
 if(!CreateIoCompletionPort(reinterpret_cast<HANDLE>(impl->listen),impl->iocp,static_cast<ULONG_PTR>(Kind::Accept),0)) {
  impl->CloseSock(impl->listen);CloseHandle(impl->iocp);delete impl;WinsockRelease();return false;
 }
 sockaddr_in bound{};int len=sizeof(bound);
 if(getsockname(impl->listen,reinterpret_cast<sockaddr*>(&bound),&len)==0)impl->bound.store(ntohs(bound.sin_port),std::memory_order_release);
 impl_=impl;
 if(!impl_->ConnectBackends()){Stop();return false;}
 for(std::uint32_t i=0;i<config.workers;++i)impl_->workers.emplace_back([state=impl_]{state->Worker();});
 for(std::uint32_t i=0;i<config.acceptDepth;++i)if(!impl_->PostAccept()){Stop();return false;}
 return true;
}
void Gateway::Stop() {
 if(!impl_)return;
 impl_->stop.store(true,std::memory_order_release);
 impl_->CloseSock(impl_->listen);
 if(impl_->links)for(std::uint32_t i=0;i<impl_->cfg.backendCount;++i)impl_->CloseSock(impl_->links[i].sock);
 if(impl_->links)for(std::uint32_t i=0;i<impl_->cfg.backendCount;++i)if(impl_->links[i].drain.joinable())impl_->links[i].drain.join();
 for(std::uint32_t i=0;i<impl_->slots;++i)impl_->Recycle(i);
 if(impl_->iocp)for(std::size_t i=0;i<impl_->workers.size()+4;++i)PostQueuedCompletionStatus(impl_->iocp,0,0,nullptr);
 for(auto& worker:impl_->workers)if(worker.joinable())worker.join();
 if(impl_->iocp)CloseHandle(impl_->iocp);
 delete impl_;impl_=nullptr;WinsockRelease();
}
std::uint16_t Gateway::BoundPort() const noexcept {return impl_?impl_->bound.load(std::memory_order_acquire):0;}
Stats Gateway::Inspect() const {
 Stats s;if(!impl_)return s;
 s.accepted=impl_->accepted.load(std::memory_order_relaxed);s.routed=impl_->routed.load(std::memory_order_relaxed);
 s.routedBytes=impl_->routedBytes.load(std::memory_order_relaxed);s.dropped=impl_->dropped.load(std::memory_order_relaxed);
 s.live=impl_->live.load(std::memory_order_relaxed);s.backends=impl_->cfg.backendCount;s.capacity=impl_->cfg.capacity;
 s.workingSetBytes=ProcessWorkingSetBytes();s.privateBytes=ProcessPrivateBytes();s.tableBytes=impl_->tableBytes;return s;
}
bool Gateway::WaitLive(std::uint32_t count,std::uint32_t milliseconds) const {
 auto until=GetTickCount64()+milliseconds;
 while(impl_ && impl_->live.load(std::memory_order_acquire)<count){if(GetTickCount64()>=until)return false;Sleep(1);}
 return impl_!=nullptr;
}
bool Gateway::WaitRouted(std::uint64_t frames,std::uint32_t milliseconds) const {
 auto until=GetTickCount64()+milliseconds;
 while(impl_ && impl_->routed.load(std::memory_order_acquire)<frames){if(GetTickCount64()>=until)return false;Sleep(1);}
 return impl_!=nullptr;
}
bool Gateway::WaitRoutedBytes(std::uint64_t bytes,std::uint32_t milliseconds) const {
 auto until=GetTickCount64()+milliseconds;
 while(impl_ && impl_->routedBytes.load(std::memory_order_acquire)<bytes){if(GetTickCount64()>=until)return false;Sleep(1);}
 return impl_!=nullptr;
}
}
