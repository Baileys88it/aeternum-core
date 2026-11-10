#include "aeternum/sdk/internal/profiler.hpp"
#include <iomanip>
#include <locale>
#include <sstream>
namespace aeternum::sdk::internal {
AeternumProfiler& AeternumProfiler::Instance() noexcept { static AeternumProfiler p; return p; }
AeternumProfiler::Record* AeternumProfiler::Register(char const* kind, char const* id) noexcept {
 try {
  if (!kind || !id) { ++registrationDrops_; return nullptr; }
  std::string key = std::string(kind) + "/" + id;
  if (key.size()>1024) { ++registrationDrops_; return nullptr; }
  std::lock_guard<std::mutex> lock(registration_);
  for (std::size_t i=0;i<size_;++i) if (records_[i].id==key) return &records_[i];
  if (size_==records_.size()) { ++registrationDrops_; return nullptr; }
  auto& r=records_[size_]; r.id=std::move(key); ++size_; return &r;
 } catch (...) { ++registrationDrops_; return nullptr; }
}
namespace {
void Quote(std::ostream& out, std::string const& text) {
 static char const hex[]="0123456789abcdef";
 out << '"';
 for (unsigned char c:text) {
  if (c=='"' || c=='\\') out << '\\' << char(c);
  else if (c<32) out << "\\u00" << hex[c>>4] << hex[c&15];
  else out << char(c);
 }
 out << '"';
}
}
std::string AeternumProfiler::Dump() {
 std::lock_guard<std::mutex> lock(registration_);
 std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(17);
 out << "{\"ClockIsSteady\":" << (Clock::is_steady?"true":"false")
     << ",\"LiveSnapshotApproximate\":true,\"RegistrationDrops\":" << registrationDrops_.load()
     << ",\"Modules\":[";
 for (std::size_t i=0;i<size_;++i) {
  auto const& r=records_[i]; if(i)out << ',';
  out << "{\"ModuleId\":"; Quote(out,r.id);
  auto ticks=r.ticks.load(std::memory_order_relaxed);
  auto count=r.count.load(std::memory_order_relaxed);
  long double micros=static_cast<long double>(ticks)*Clock::period::num*1000000.0L/Clock::period::den;
  out << ",\"ExecutionCount\":" << count << ",\"TotalTimeMicroseconds\":" << micros << '}';
 }
 out << "]}"; return out.str();
}
void AeternumProfiler::ResetCountersForTest() noexcept {
 std::lock_guard<std::mutex> lock(registration_);
 for(std::size_t i=0;i<size_;++i){records_[i].count.store(0);records_[i].ticks.store(0);}
 registrationDrops_.store(0);
}
thread_local AeternumProfiler::Scope* AeternumProfiler::Scope::top_=nullptr;
AeternumProfiler::Scope::Scope(Record* record) noexcept :record_(record),previous_(top_) {
 top_=this;
 if(!record_)return;
 for(auto* p=previous_;p;p=p->previous_)if(p->record_==record_)return;
 owns_=true; start_=Clock::now();
}
AeternumProfiler::Scope::~Scope() noexcept {
 if(owns_) {
  auto elapsed=(Clock::now()-start_).count();
  if(elapsed>0)record_->ticks.fetch_add(static_cast<std::uint64_t>(elapsed),std::memory_order_relaxed);
  record_->count.fetch_add(1,std::memory_order_relaxed);
 }
 top_=previous_;
}
}
