/*
 * Aeternum Forever — pure Classic ExtraInserts remap helpers (M4 / IE-M1).
 * Shared by ClassicOpcodes.cpp, ProtocolApiAdapter, and Catch2 gates. No conf/singletons.
 */
#ifndef AETERNUM_FOREVER_CLASSIC_OPCODE_REMAP_H
#define AETERNUM_FOREVER_CLASSIC_OPCODE_REMAP_H

#include "Define.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Forever::OpcodeRemap
{
inline constexpr uint32 InvalidClassicOpcode = 0xFFFFFFFFu;

inline constexpr uint32 OpcodeGroup(uint32 opcode)
{
    return opcode >> 16;
}

inline constexpr uint32 WithOpcodeGroup(uint32 opcode, uint32 group)
{
    return (group << 16) | (opcode & 0xFFFF);
}

/** Mirror of ClassicOpcodes RemoveClientExtraInserts (inserts sorted ascending). */
inline uint32 RemoveClientExtraInserts(uint32 classicOpcode, std::vector<uint32> const& inserts)
{
    uint32 below = 0;
    for (uint32 insert : inserts)
    {
        if (OpcodeGroup(insert) != OpcodeGroup(classicOpcode))
            continue;
        if (insert == classicOpcode)
            return InvalidClassicOpcode;
        if (insert < classicOpcode)
            ++below;
    }
    return classicOpcode - below;
}

/**
 * Reverse of RemoveClientExtraInserts: 70707-style number -> wire/new-build number.
 * Inserts are wire numbers (same convention as production conf/profile lists).
 */
inline uint32 AddClientExtraInserts(uint32 classicOpcode, std::vector<uint32> inserts)
{
    std::sort(inserts.begin(), inserts.end());
    for (uint32 insert : inserts)
    {
        if (OpcodeGroup(insert) != OpcodeGroup(classicOpcode))
            continue;
        if (insert <= classicOpcode)
            ++classicOpcode;
    }
    return classicOpcode;
}

/** After ExtraGroupShift path: main client group 0x44 maps to retail/core 0x43 (group - 1). */
inline uint32 ClientMainGroupToCore(uint32 classicOpcode)
{
    uint32 const group = OpcodeGroup(classicOpcode);
    if (group == 0x44)
        return WithOpcodeGroup(classicOpcode, 0x43);
    return classicOpcode;
}

/** Reverse of ClientMainGroupToCore for the Forever main client group. */
inline uint32 CoreMainGroupToClient(uint32 coreOpcode)
{
    uint32 const group = OpcodeGroup(coreOpcode);
    if (group == 0x43)
        return WithOpcodeGroup(coreOpcode, 0x44);
    return coreOpcode;
}

inline uint32 WireToCoreAfterClientInserts(uint32 wire, std::vector<uint32> inserts)
{
    std::sort(inserts.begin(), inserts.end());
    uint32 v = RemoveClientExtraInserts(wire, inserts);
    if (v == InvalidClassicOpcode)
        return InvalidClassicOpcode;
    return ClientMainGroupToCore(v);
}

/** Inverse of WireToCoreAfterClientInserts for ExtraInserts + main-group shift only. */
inline uint32 CoreToWireAfterClientInserts(uint32 core, std::vector<uint32> inserts)
{
    uint32 v = CoreMainGroupToClient(core);
    return AddClientExtraInserts(v, std::move(inserts));
}
}

#endif
