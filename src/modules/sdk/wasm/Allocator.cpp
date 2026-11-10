#include "aeternum/sdk/internal/wasm_sandbox.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <limits>
namespace {
struct alignas(std::max_align_t) Header { std::size_t size; };
std::atomic_size_t blocks{0}, bytes{0}, peak{0};
std::atomic_llong failAfter{-1};
constexpr std::size_t Budget = 16 * 1024 * 1024; // aggregate wasm3 heap cap, not guest-visible
bool Denied() { auto n=failAfter.load(); if(n<0)return false; if(!n)return true; --failAfter; return false; }
}
extern "C" void* aeternum_wasm_calloc(std::size_t n,std::size_t s) {
 if(s && n>std::numeric_limits<std::size_t>::max()/s)return nullptr;
 auto size=n*s; if(size>Budget || bytes.load()>Budget-size || Denied())return nullptr;
 auto p=static_cast<Header*>(std::calloc(1,sizeof(Header)+size));if(!p)return nullptr;
 p->size=size; ++blocks;auto b=bytes.fetch_add(size)+size;auto high=peak.load();while(high<b&&!peak.compare_exchange_weak(high,b)){}
 return p+1;
}
extern "C" void aeternum_wasm_free(void* p) {
 if(!p)return;auto h=static_cast<Header*>(p)-1;bytes-=h->size;--blocks;std::free(h);
}
extern "C" void* aeternum_wasm_realloc(void* p,std::size_t size) {
 if(!p)return aeternum_wasm_calloc(1,size);
 auto old=static_cast<Header*>(p)-1;
 // Allocate-copy-free deliberately retains old memory on failure.
 auto q=aeternum_wasm_calloc(1,size);if(!q)return nullptr;
 std::memcpy(q,p,old->size<size?old->size:size);aeternum_wasm_free(p);return q;
}
namespace aeternum::sdk::internal::wasm {
AllocationStats Allocations(){return {blocks.load(),bytes.load(),peak.load()};}
void FailAllocationAfterForTest(long long n){failAfter=n;}
}
