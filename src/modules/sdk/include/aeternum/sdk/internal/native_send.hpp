#pragma once
#include "aeternum/abi/AeternumABI.h"
#include <cstdint>
namespace aeternum::sdk::internal::native {
// Core-only, stack-scoped capability: installed under WorldSocket::_worldSessionLock.
// There is deliberately NO global session/raw-pointer lookup or saved lifetime capability.
using SubmitPacketFn = AeternumStatus (*)(void*,uint32_t,void const*,uint32_t) noexcept;
class SendTargetScope final {
public:
 SendTargetScope(uint32_t sessionId,void* target,SubmitPacketFn submit) noexcept;
 ~SendTargetScope();
 SendTargetScope(SendTargetScope const&)=delete;
 SendTargetScope& operator=(SendTargetScope const&)=delete;
 uint32_t sessionId; void* target; SubmitPacketFn submit;
private: SendTargetScope* previous_;
};
struct SendPacketStats {uint64_t claimed,disposed,accepted,rejected;};
SendPacketStats GetSendPacketStats() noexcept;
AeternumStatus AETERNUM_ABI_CALL AbiSendPacket(uint32_t sessionId,AeternumOwnedPacket* packet);
}
