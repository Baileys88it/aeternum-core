#include "aeternum/ecs/kernel.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <cstring>
#include <utility>
namespace aeternum::ecs {
namespace {
std::atomic<std::uint64_t> nextLifetime{1};
constexpr std::uint32_t none=UINT32_MAX;
struct AeternumTransforms {float *X=nullptr,*Y=nullptr,*Z=nullptr,*O=nullptr;};
struct AeternumVitals {std::uint32_t *HP=nullptr,*MaxHP=nullptr;};
struct AeternumAuraState {std::uint64_t* Mask=nullptr;};
struct Slot {AeternumGuid128 guid{};std::uint64_t owner=0;std::uint32_t next=none;std::uint16_t generation=1;bool active=false;};
struct Bucket {AeternumGuid128 guid{};std::uint32_t index=0;std::uint8_t state=0;};
struct Delta {float x,y,z,o;std::uint32_t hp,maxHp;std::uint64_t aura;bool apply;};
struct Snapshot {float const *x,*y,*z,*o;std::uint32_t const *hp,*maxHp;std::uint64_t const* aura;Slot const* slots;std::uint32_t diff;};
bool same(AeternumGuid128 a,AeternumGuid128 b){return a.low==b.low&&a.high==b.high;}
std::uint64_t mix(std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
}
struct Kernel::Impl {
 Allocator allocator;Options options;std::uint32_t lifetime,hashSize=1,freeHead=0,count=0,tombstones=0;
 AeternumTransforms transforms;AeternumVitals vitals;AeternumAuraState auras;
 Slot* slots=nullptr;Bucket* buckets=nullptr;Delta* deltas=nullptr;std::array<void*,6> blocks{};
 std::mutex gate,pool;std::condition_variable work,complete,ready;
 std::array<std::thread,16> threads;std::array<std::thread::id,16> ids{};
 std::array<std::uint64_t,16> passes{};std::array<bool,16> failed{};
 std::atomic<bool> cancel{false};bool stopped=false,poolStop=false;
 std::uint64_t epoch=0;std::uint32_t done=0,started=0;Snapshot snapshot{};Stats stats;
 // gate pins every SOA column: workers borrow an immutable snapshot for L.
 // Only the caller holding gate (logical thread0, not a fixed OS thread) writes S.
 Impl(Allocator a,Options o,std::uint32_t id):allocator(a),options(std::move(o)),lifetime(id){}
 template<class T> T* allocate(std::size_t n,unsigned block){
  void* memory=allocator.allocate(allocator.context,n*sizeof(T));if(!memory)throw std::bad_alloc();blocks[block]=memory;
  auto p=static_cast<T*>(memory);std::uninitialized_value_construct_n(p,n);return p;
 }
 void initialize(){
  auto n=options.capacity;while(hashSize<n*2)hashSize*=2;
  auto t=allocate<float>(n*4,0);transforms={t,t+n,t+2*n,t+3*n};
  auto v=allocate<std::uint32_t>(n*2,1);vitals={v,v+n};auras.Mask=allocate<std::uint64_t>(n,2);
  slots=allocate<Slot>(n,3);buckets=allocate<Bucket>(hashSize,4);deltas=allocate<Delta>(n,5);
  for(std::uint32_t i=0;i<n;++i)slots[i].next=i+1<n?i+1:none;
  for(std::uint32_t i=0;i<options.workers;++i){
   if(options.failStartAfterForTest==int(i))throw std::runtime_error("ECS partial startup test fault");
   threads[i]=std::thread([this,i]{worker(i);});
  }
  std::unique_lock lock(pool);ready.wait(lock,[&]{return started==options.workers;});
 }
 std::uint32_t bucket(AeternumGuid128 guid,bool insertion)const {
  auto at=std::uint32_t(mix(guid.low)^mix(guid.high+0x9e3779b97f4a7c15ULL))&(hashSize-1),tomb=none;
  for(std::uint32_t i=0;i<hashSize;++i,at=(at+1)&(hashSize-1)){
   auto const& b=buckets[at];if(b.state==1&&same(b.guid,guid))return at;
   if(b.state==2&&tomb==none)tomb=at;
   if(!b.state)return insertion?(tomb==none?at:tomb):none;
  }return insertion?tomb:none;
 }
 void compact(){
  if(tombstones<=hashSize/4)return;
  std::fill_n(buckets,hashSize,Bucket{});tombstones=0;
  for(std::uint32_t i=0;i<options.capacity;++i)if(slots[i].active){auto b=bucket(slots[i].guid,true);buckets[b]={slots[i].guid,i,1};}
 }
 AeternumStatus decode(std::uint64_t owner,AeternumEcsEntityHandle handle,std::uint32_t& index)const {
  auto word=std::uint32_t(handle&0xffff);if(!word||word>options.capacity||std::uint32_t(handle>>32)!=lifetime)return AETERNUM_ERR_NOT_FOUND;
  index=word-1;auto const& s=slots[index];if(!s.active||s.generation!=std::uint16_t(handle>>16))return AETERNUM_ERR_NOT_FOUND;
  return s.owner==owner?AETERNUM_OK:AETERNUM_ERR_DENIED;
 }
 void worker(std::uint32_t workerIndex) noexcept {
  std::uint64_t seen=0;
  {std::lock_guard lock(pool);ids[workerIndex]=std::this_thread::get_id();++started;ready.notify_one();}
  for(;;){
   Snapshot read;
   {std::unique_lock lock(pool);work.wait(lock,[&]{return poolStop||epoch!=seen;});if(poolStop)return;seen=epoch;read=snapshot;}
   try {
    if(options.control&&workerIndex==0){options.control->entered.store(true,std::memory_order_release);
     while(!options.control->release.load(std::memory_order_acquire)&&!cancel.load(std::memory_order_acquire))std::this_thread::yield();}
    if(options.failWorkerForTest==int(workerIndex))throw std::runtime_error("ECS worker test fault");
    auto begin=options.capacity*workerIndex/options.workers,end=options.capacity*(workerIndex+1)/options.workers;
    for(auto i=begin;i<end;++i){auto& d=deltas[i];d.apply=read.slots[i].active;if(!d.apply)continue;
     d={read.x[i],read.y[i],read.z[i],read.o[i],read.hp[i],read.maxHp[i],read.aura[i],true};
     // Production is an identity-delta stub, NOT combat/pathfinding.
     if(options.deterministicProbe){d.x+=float(read.diff);if(d.hp)--d.hp;d.aura^=1;}
     if(options.movementWorkload){d.x+=0.25f*float((read.diff%17)+1);d.y+=0.125f*float((i&15)+1);d.o+=0.01f;}
    }
   }catch(...){failed[workerIndex]=true;}
   ++passes[workerIndex];
   {std::lock_guard lock(pool);++done;if(done==options.workers)complete.notify_one();}
  }
 }
 void join(){
  {std::lock_guard lock(pool);poolStop=true;}work.notify_all();
  for(auto& thread:threads)if(thread.joinable())thread.join();
 }
 void shutdown() noexcept {
  cancel.store(true,std::memory_order_release);std::lock_guard lock(gate);if(stopped)return;
  join();stopped=true;
  for(auto& block:blocks)if(block){allocator.release(allocator.context,block);block=nullptr;}
 }
 ~Impl(){shutdown();}
};
Kernel::Kernel(std::unique_ptr<Impl> p):p_(std::move(p)){}
Kernel::~Kernel()=default;
std::unique_ptr<Kernel> Kernel::Create(Allocator allocator,Options options) noexcept {
 if(!allocator.allocate||!allocator.release||!options.capacity||options.capacity>16384||!options.workers||options.workers>16||options.failWorkerForTest>=int(options.workers))return nullptr;
 try {
  auto id=nextLifetime.load();
  while(id<=UINT32_MAX&&!nextLifetime.compare_exchange_weak(id,id+1)){}
  if(id>UINT32_MAX)return nullptr; // never wrap a published lifetime
  auto impl=std::make_unique<Impl>(allocator,std::move(options),std::uint32_t(id));impl->initialize();
  return std::unique_ptr<Kernel>(new Kernel(std::move(impl)));
 }catch(...){return nullptr;}
}
AeternumStatus Kernel::Register(std::uint64_t owner,AeternumEcsEntityState const& state,AeternumEcsEntityHandle& out) noexcept {
 auto& p=*p_;std::unique_lock lock(p.gate,std::try_to_lock);if(!lock.owns_lock())return AETERNUM_ERR_BUSY;
 if(p.stopped||p.cancel.load())return AETERNUM_ERR_NOT_READY;
 if(!owner||state.struct_size<sizeof(state)||(!state.guid.low&&!state.guid.high)||!std::isfinite(state.x)||!std::isfinite(state.y)||!std::isfinite(state.z)||!std::isfinite(state.o)||!state.max_hp||state.hp>state.max_hp)return AETERNUM_ERR_INVALID_ARGUMENT;
 auto b=p.bucket(state.guid,true);if(b!=none&&p.buckets[b].state==1)return AETERNUM_ERR_BUSY;
 if(p.freeHead==none||b==none)return AETERNUM_ERR_OVERFLOW;
 auto index=p.freeHead;auto& s=p.slots[index];p.freeHead=s.next;s.active=true;s.owner=owner;s.guid=state.guid;
 if(p.buckets[b].state==2)--p.tombstones;p.buckets[b]={state.guid,index,1};++p.count;
 p.transforms.X[index]=state.x;p.transforms.Y[index]=state.y;p.transforms.Z[index]=state.z;p.transforms.O[index]=state.o;
 p.vitals.HP[index]=state.hp;p.vitals.MaxHP[index]=state.max_hp;p.auras.Mask[index]=state.aura_mask;
 out=(std::uint64_t(p.lifetime)<<32)|(std::uint64_t(s.generation)<<16)|(index+1);return AETERNUM_OK;
}
AeternumStatus Kernel::Unregister(std::uint64_t owner,AeternumEcsEntityHandle handle) noexcept {
 auto& p=*p_;std::unique_lock lock(p.gate,std::try_to_lock);if(!lock.owns_lock())return AETERNUM_ERR_BUSY;if(p.stopped||p.cancel.load())return AETERNUM_ERR_NOT_READY;
 std::uint32_t i;auto status=p.decode(owner,handle,i);if(status)return status;
 auto& s=p.slots[i];auto b=p.bucket(s.guid,false);p.buckets[b].state=2;++p.tombstones;s.active=false;--p.count;
 if(s.generation!=UINT16_MAX){++s.generation;s.next=p.freeHead;p.freeHead=i;} // exhausted slots retire, never reuse stale generation
 p.compact();return AETERNUM_OK;
}
AeternumStatus Kernel::Read(std::uint64_t owner,AeternumEcsEntityHandle handle,AeternumEcsEntityState& out) noexcept {
 auto& p=*p_;std::unique_lock lock(p.gate,std::try_to_lock);if(!lock.owns_lock())return AETERNUM_ERR_BUSY;if(p.stopped||p.cancel.load())return AETERNUM_ERR_NOT_READY;
 if(out.struct_size<sizeof(out))return AETERNUM_ERR_INVALID_ARGUMENT;
 std::uint32_t i;auto status=p.decode(owner,handle,i);if(status)return status;
 out={out.struct_size,p.slots[i].guid,p.transforms.X[i],p.transforms.Y[i],p.transforms.Z[i],p.transforms.O[i],p.vitals.HP[i],p.vitals.MaxHP[i],p.auras.Mask[i]};return AETERNUM_OK;
}
AeternumStatus Kernel::Tick(std::uint32_t diff) noexcept {
 auto& p=*p_;std::unique_lock gate(p.gate,std::try_to_lock);if(!gate.owns_lock())return AETERNUM_ERR_BUSY;if(p.stopped||p.cancel.load())return AETERNUM_ERR_NOT_READY;
 try {
  {std::unique_lock lock(p.pool);p.snapshot={p.transforms.X,p.transforms.Y,p.transforms.Z,p.transforms.O,p.vitals.HP,p.vitals.MaxHP,p.auras.Mask,p.slots,diff};p.done=0;p.failed.fill(false);++p.epoch;p.work.notify_all();p.complete.wait(lock,[&]{return p.done==p.options.workers;});}
  if(p.cancel.load())return AETERNUM_ERR_NOT_READY;
  if(std::any_of(p.failed.begin(),p.failed.end(),[](bool failed){return failed;}))return AETERNUM_ERR_INTERNAL;
  // Strict S: all L readers completed; deterministic worker/slot order, caller only.
  for(std::uint32_t worker=0;worker<p.options.workers;++worker){auto begin=p.options.capacity*worker/p.options.workers,end=p.options.capacity*(worker+1)/p.options.workers;
   for(auto i=begin;i<end;++i){auto const& d=p.deltas[i];if(!d.apply)continue;
    p.transforms.X[i]=d.x;p.transforms.Y[i]=d.y;p.transforms.Z[i]=d.z;p.transforms.O[i]=d.o;p.vitals.HP[i]=d.hp;p.vitals.MaxHP[i]=d.maxHp;p.auras.Mask[i]=d.aura;++p.stats.applied;
   }
  }
  ++p.stats.ticks;p.stats.totalDiff+=diff;p.stats.coordinator=std::this_thread::get_id();return AETERNUM_OK;
 }catch(...){p.cancel.store(true);p.join();return AETERNUM_ERR_INTERNAL;}
}
AeternumStatus Kernel::Inspect(Stats& out) noexcept {
 auto& p=*p_;std::unique_lock gate(p.gate,std::try_to_lock);if(!gate.owns_lock())return AETERNUM_ERR_BUSY;
 out=p.stats;out.entities=p.count;out.workers=p.options.workers;out.workerJobs=0;out.distinctWorkers=0;
 for(unsigned i=0;i<p.options.workers;++i){out.workerJobs+=p.passes[i];bool unique=true;for(unsigned j=0;j<i;++j)if(p.ids[i]==p.ids[j])unique=false;if(unique)++out.distinctWorkers;}
 return AETERNUM_OK;
}
AeternumStatus Kernel::ExportTransforms(void* dst,std::size_t bytes) noexcept {
 auto& p=*p_;std::unique_lock gate(p.gate,std::try_to_lock);if(!gate.owns_lock())return AETERNUM_ERR_BUSY;if(p.stopped||p.cancel.load())return AETERNUM_ERR_NOT_READY;
 auto need=std::size_t(p.options.capacity)*4u*sizeof(float);if(!dst||bytes!=need||!p.transforms.X)return AETERNUM_ERR_INVALID_ARGUMENT;
 std::memcpy(dst,p.transforms.X,need);return AETERNUM_OK;
}
std::size_t Kernel::TransformBytes() const noexcept {return std::size_t(p_->options.capacity)*4u*sizeof(float);}
void Kernel::Shutdown() noexcept {p_->shutdown();}
std::uint32_t Kernel::Lifetime() const noexcept {return p_->lifetime;}
}
