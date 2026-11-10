/*
 * IE-M1 step 3 — wire IOpcodeRegistry / IClientProfile into Forever session path.
 * Zero wire change: ExtraInserts removal still uses OpcodeRemap with the same
 * EffectiveClientExtraInserts lists; the adapter is the process-wide seat.
 */
#ifndef AETERNUM_FOREVER_PROTOCOL_SESSION_BRIDGE_H
#define AETERNUM_FOREVER_PROTOCOL_SESSION_BRIDGE_H

#include "Define.h"

#include <cstdint>
#include <string>
#include <vector>

namespace aeternum::protocol { class IOpcodeRegistry; class IClientProfile; }

namespace Forever
{
class OpcodeRegistryAdapter;
class ProfileApiAdapter;

namespace ProtocolSession
{
/** Conf: Classic.ProtocolApiAdapter.Enable (default 1 — Remap/adapter parity proven by Catch2). */
bool Enabled();

/** Build/refresh adapters from EffectiveClientExtraInserts + active profile. Safe to call after ApplyActiveProfileDataAtStartup. */
void InitAtStartup();

/** Tear down (world shutdown). */
void Shutdown();

/**
 * Session-path ExtraInserts removal. When Enabled(), routes through the process
 * OpcodeRegistryAdapter insert list (same vectors as EffectiveClientExtraInserts).
 * Bit-identical to Forever::OpcodeRemap::RemoveClientExtraInserts.
 */
uint32 RemoveClientExtraInserts(uint32 classicOpcode);

/** Optional accessors for tests / future ops (may be null if disabled). */
aeternum::protocol::IOpcodeRegistry const* Registry();
aeternum::protocol::IClientProfile const* Profile();

std::vector<uint32> const& ClientInsertsSnapshot();
std::string const& ProfileIdSnapshot();
}
}

#endif