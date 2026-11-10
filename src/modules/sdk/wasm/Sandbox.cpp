#include "aeternum/sdk/internal/wasm_sandbox.hpp"
#include "wasm3.h"
#include "aeternum/sdk/internal/profiler.hpp"
#include "m3_env.h" // pinned implementation details only for fail-closed import/start validation
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <limits>
#include "aeternum/sdk/internal/wasm_abort.hpp"
namespace aeternum::sdk::internal::wasm {
namespace {
struct AbortControl {
 std::string identity; std::uint64_t generation;
 std::atomic<bool> cancel{false}; bool running=false; // running is registry-mutex protected
 AbortControl(std::string id,std::uint64_t gen):identity(std::move(id)),generation(gen){}
};
struct Registry {
 std::mutex mutex; std::array<std::weak_ptr<AbortControl>,64> slots;
 std::uint64_t nextGeneration=1;
};
Registry& Controls(){static Registry r;return r;}
std::shared_ptr<AbortControl> RegisterControl(std::string const& identity) {
 auto& r=Controls();std::lock_guard<std::mutex> lock(r.mutex);
 std::size_t free=r.slots.size();
 for(std::size_t i=0;i<r.slots.size();++i) {
  auto c=r.slots[i].lock();if(c && c->identity==identity)return {};
  if(!c && free==r.slots.size())free=i;
 }
 if(free==r.slots.size() || r.nextGeneration==std::numeric_limits<std::uint64_t>::max())return {};
 auto c=std::make_shared<AbortControl>(identity,r.nextGeneration++);r.slots[free]=c;return c;
}
bool Cancelled(void* context) {return static_cast<AbortControl*>(context)->cancel.load(std::memory_order_acquire);}
struct Invocation {
 std::shared_ptr<AbortControl> c;
 explicit Invocation(std::shared_ptr<AbortControl> control):c(std::move(control)) {
  std::lock_guard<std::mutex> lock(Controls().mutex);
  c->cancel.store(false,std::memory_order_release);c->running=true;
 }
 ~Invocation(){std::lock_guard<std::mutex> lock(Controls().mutex);c->running=false;}
};
}
std::vector<ModuleState> SnapshotModules() {
 auto& r=Controls();std::lock_guard<std::mutex> lock(r.mutex);std::vector<ModuleState> result;
 for(auto const& slot:r.slots)if(auto c=slot.lock())result.push_back({c->identity,c->generation,c->running,c->cancel.load(std::memory_order_acquire)});
 return result;
}
AbortResult RequestAbort(std::string const& module,std::uint64_t generation) {
 auto& r=Controls();std::lock_guard<std::mutex> lock(r.mutex);
 for(auto const& slot:r.slots)if(auto c=slot.lock();c && c->identity==module) {
  if(c->generation!=generation)return AbortResult::StaleGeneration;
  if(!c->running)return AbortResult::Idle;
  if(c->cancel.exchange(true,std::memory_order_acq_rel))return AbortResult::AlreadyRequested;
  return AbortResult::Accepted;
 }
 return AbortResult::Missing;
}
struct Sandbox::Impl {
 IM3Environment env=nullptr; IM3Runtime runtime=nullptr; IM3Function bound=nullptr;
 std::uint64_t fuelLimit=1000000, fuelUsed=0;
 std::vector<unsigned char> bytes; Log log; std::string error; bool loaded=false;
 std::shared_ptr<AbortControl> abort; // outlives every VM use of its dispatch callback
 static m3ApiRawFunction(HostLog) {
  (void)_ctx;(void)_mem;
  auto self=static_cast<Impl*>(m3_GetUserData(runtime));
  uint32_t offset=static_cast<uint32_t>(*_sp);
  uint32_t size=0;auto memory=m3_GetMemory(runtime,&size,0);
  if(!memory || !offset || offset>=size)return "invalid guest log pointer";
  auto count=std::min<uint32_t>(256,size-offset);
  auto end=static_cast<unsigned char const*>(std::memchr(memory+offset,0,count));
  if(!end)return "unterminated or oversized guest log";
  try {if(self->log)self->log(std::string(reinterpret_cast<char*>(memory+offset),end-(memory+offset)));}
  catch(...){return "host log failure";}
  return m3Err_none;
 }
 void release() noexcept {
  if(runtime)m3_FreeRuntime(runtime);runtime=nullptr;
  if(env)m3_FreeEnvironment(env);env=nullptr;
  abort.reset();bound=nullptr;
  std::vector<unsigned char>().swap(bytes);log={};loaded=false;
 }
 bool fail(char const* e){error=e?e:"unknown WASM error";release();return false;}
 ~Impl(){release();}
};
Sandbox::Sandbox():impl_(std::make_unique<Impl>()){}
Sandbox::~Sandbox()=default;
void Sandbox::Reset() noexcept {impl_->release();impl_->error.clear();}
std::string const& Sandbox::Error() const{return impl_->error;}
bool Sandbox::Ready() const{return impl_->runtime!=nullptr;}
std::uint64_t Sandbox::FuelUsed() const{return impl_->fuelUsed;}
bool Sandbox::Initialize(Log log,std::uint64_t fuel){
 Reset();impl_->fuelUsed=0;
 if(!fuel || fuel>10000000)return impl_->fail("WASM fuel limit must be 1..10000000");
 impl_->fuelLimit=fuel;impl_->log=std::move(log);impl_->env=m3_NewEnvironment();
 if(!impl_->env)return impl_->fail("WASM environment allocation failed");
 impl_->runtime=m3_NewRuntime(impl_->env,64*1024,impl_.get());
 if(!impl_->runtime)return impl_->fail("WASM stack/runtime allocation failed");
 return true;
}
bool Sandbox::LoadImage(std::uint8_t const* data,std::size_t size,std::string identity){
 auto& s=*impl_;
 if(!s.runtime)return s.fail("WASM environment not initialized");
 if(s.loaded)return s.fail("duplicate WASM load");
 if(!data || size<8 || size>256*1024)return s.fail("WASM file size rejected");
 if(identity.empty() || identity.size()>1024 || identity.find('\0')!=std::string::npos)return s.fail("WASM identity rejected");
 try {
 s.bytes.assign(data,data+size);
 IM3Module module=nullptr;auto result=m3_ParseModule(s.env,&module,s.bytes.data(),static_cast<uint32_t>(s.bytes.size()));
 if(result)return s.fail(result);
 auto reject=[&](char const* reason){m3_FreeModule(module);return s.fail(reason);};
 if(module->startFunction>=0)return reject("WASM start section forbidden; use on_init");
 if(module->memoryImported || module->memoryInfo.initPages>16 || module->memoryInfo.maxPages>16)
  return reject("WASM memory exceeds sandbox policy");
 for(uint32_t i=0;i<module->numGlobals;++i)if(module->globals[i].imported)return reject("WASM global import forbidden");
 if(module->numFuncImports!=1)return reject("WASM import allowlist requires one host log");
 auto const& imp=module->functions[0].import;
 if(!imp.moduleUtf8||!imp.fieldUtf8||std::strcmp(imp.moduleUtf8,"aeternum")||std::strcmp(imp.fieldUtf8,"aeternum_host_log"))
  return reject("WASM import forbidden");
 result=m3_LoadModule(s.runtime,module);if(result)return reject(result);
 s.loaded=true;
 result=m3_LinkRawFunction(module,"aeternum","aeternum_host_log","v(i)",&Impl::HostLog);
 if(result)return s.fail(result);
 s.bound=nullptr;
 IM3Function init=nullptr;result=m3_FindFunction(&init,s.runtime,"on_init");
 if(result)return s.fail(result);
 if(m3_GetArgCount(init)||m3_GetRetCount(init))return s.fail("on_init must be void(void)");
 auto* metric=AeternumProfiler::Instance().Register("wasm",identity.c_str());
 s.abort=RegisterControl(identity);
 if(!s.abort)return s.fail("WASM module identity already live or abort registry full");
 s.runtime->aeternumCancelContext=s.abort.get();s.runtime->aeternumCancelCheck=&Cancelled;
 s.runtime->aeternumFuel=s.fuelLimit;
 { Invocation invocation(s.abort); AeternumProfiler::Scope timer(metric); result=m3_CallV(init); }
 s.fuelUsed=s.fuelLimit-s.runtime->aeternumFuel;
 if(result)return s.fail(result);
 return true;
 }catch(...){return s.fail("WASM host load exception");}
}
bool Sandbox::LoadAndRun(std::filesystem::path const& path){
 auto& s=*impl_;
 if(!s.runtime)return s.fail("WASM environment not initialized");
 if(s.loaded)return s.fail("duplicate WASM load");
 try {
 std::ifstream file(path,std::ios::binary|std::ios::ate);
 if(!file)return s.fail("WASM file unavailable");
 auto length=file.tellg();if(length<=0||length>256*1024)return s.fail("WASM file size rejected");
 std::vector<unsigned char> bytes(static_cast<std::size_t>(length));file.seekg(0);
 if(!file.read(reinterpret_cast<char*>(bytes.data()),length))return s.fail("WASM file read failed");
 auto utf8=std::filesystem::weakly_canonical(path).generic_u8string();
 std::string identity(reinterpret_cast<char const*>(utf8.data()),utf8.size());
 return LoadImage(bytes.data(),bytes.size(),std::move(identity));
 }catch(...){return s.fail("WASM host load exception");}
}
bool Sandbox::BindI32Export(char const* exported){
 auto& s=*impl_;
 if(!s.runtime || !s.loaded || !exported || !*exported)return s.fail("WASM export unavailable");
 IM3Function fn=nullptr;auto status=m3_FindFunction(&fn,s.runtime,exported);
 if(status)return s.fail(status);
 if(m3_GetArgCount(fn)!=1 || m3_GetRetCount(fn)!=1)return s.fail("export must be i32(i32)");
 s.bound=fn;return true;
}
bool Sandbox::CallI32(char const* exported,std::int32_t argument,std::int32_t& result){
 auto& s=*impl_;
 if(!s.runtime || !s.loaded || !s.abort || !exported || !*exported)return s.fail("WASM export unavailable");
 try {
 IM3Function fn=nullptr;auto status=m3_FindFunction(&fn,s.runtime,exported);
 if(status)return s.fail(status);
 if(m3_GetArgCount(fn)!=1 || m3_GetRetCount(fn)!=1)return s.fail("export must be i32(i32)");
 auto* metric=AeternumProfiler::Instance().Register("wasm",exported);
 s.runtime->aeternumFuel=s.fuelLimit;
 { Invocation invocation(s.abort); AeternumProfiler::Scope timer(metric); status=m3_CallV(fn,argument); }
 s.fuelUsed=s.fuelLimit-s.runtime->aeternumFuel;
 if(status)return s.fail(status);
 std::int32_t out=0;status=m3_GetResultsV(fn,&out);if(status)return s.fail(status);
 result=out;return true;
 }catch(...){return s.fail("WASM host call exception");}
}
}
