#include "aeternum/sdk/internal/native_send.hpp"
#include "aeternum/sdk/internal/native_host.hpp"
#include <atomic>
#include <cstring>
namespace aeternum::sdk::internal::native {
namespace {
thread_local SendTargetScope* current=nullptr;
thread_local bool submitting=false;
std::atomic<uint64_t> claimed{0},disposed{0},accepted{0},rejected{0};
uint64_t u64(uint8_t const* p){uint64_t v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(p[i])<<(8*i);return v;}
uint32_t u32(uint8_t const* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
struct Claimed {void* p;~Claimed(){HostDisposeClaimed(p);++disposed;}};
AeternumStatus reject(AeternumStatus status){++rejected;return status;}
}
SendTargetScope::SendTargetScope(uint32_t id,void* p,SubmitPacketFn fn) noexcept:sessionId(id),target(p),submit(fn),previous_(current){current=this;}
SendTargetScope::~SendTargetScope(){current=previous_;}
SendPacketStats GetSendPacketStats() noexcept{return {claimed.load(),disposed.load(),accepted.load(),rejected.load()};}
AeternumStatus AETERNUM_ABI_CALL AbiSendPacket(uint32_t id,AeternumOwnedPacket* packet){
 int slot=CurrentCrossingSlot();
 if(slot<0)return reject(AETERNUM_ERR_DENIED);
 auto state=GetSlotState(slot);
 // Match HostFree states: an in-flight callback must still surrender ownership if F3 starts stopping.
 if(state!=SlotState::Booting&&state!=SlotState::Active&&state!=SlotState::Stopping)return reject(AETERNUM_ERR_DENIED);
 AeternumProfiler::Scope timer(GetSlotProfiler(slot)); // nested in a hook: same identity, counted only once
 uint64_t allocation=0;
 // Registry + cookie/canary verification precede EVERY packet pointer dereference.
 if(!HostPeekOwnedBlock(slot,packet,&allocation)||allocation>UINT32_MAX||!HostClaimMutationBlock(slot,packet,static_cast<uint32_t>(allocation)))
  return reject(AETERNUM_ERR_DENIED);
 ++claimed;Claimed release{packet};
 if(GetSlotState(slot)!=SlotState::Active)return reject(AETERNUM_ERR_DENIED);
 if(allocation<sizeof(AeternumOwnedPacket))return reject(AETERNUM_ERR_INVALID_ARGUMENT);
 AeternumOwnedPacket header{};std::memcpy(&header,packet,sizeof(header));
 if(header.struct_size!=sizeof(header)||header.flags||header.payload_size!=21||allocation!=sizeof(header)+header.payload_size)
  return reject(AETERNUM_ERR_INVALID_ARGUMENT);
 if(header.opcode!=0x45003Eu)return reject(AETERNUM_ERR_UNSUPPORTED);
 auto data=reinterpret_cast<uint8_t const*>(packet)+sizeof(header);
 auto total=u64(data),left=u64(data+12);
 if(!total||total>60||!left||left>total||u32(data+8)!=0||data[20]!=0)return reject(AETERNUM_ERR_INVALID_ARGUMENT);
 if(!current||!current->target||!current->submit)return reject(AETERNUM_ERR_DENIED);
 if(id==UINT32_MAX||id!=current->sessionId)return reject(AETERNUM_ERR_NOT_FOUND);
 if(submitting)return reject(AETERNUM_ERR_BUSY);
 submitting=true;auto status=current->submit(current->target,header.opcode,data,header.payload_size);submitting=false;
 if(status==AETERNUM_OK){++accepted;return status;}return reject(status);
}
}
