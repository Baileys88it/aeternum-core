/*
 * IE-M1 — thin adapter: aeternum::protocol contracts over Forever ExtraInserts/Remap.
 * Zero wire-behavior change: delegates to ClassicOpcodeRemap + ClientProfileInfo only.
 * HandlerOf is intentionally Unhandled until opcode-table binding (later milestone).
 */
#ifndef AETERNUM_FOREVER_PROTOCOL_API_ADAPTER_H
#define AETERNUM_FOREVER_PROTOCOL_API_ADAPTER_H

#include "Forever/ClassicOpcodeRemap.h"
#include "Forever/ClientProfile.h"
#include "aeternum/api/protocol/opcode.hpp"
#include "aeternum/api/protocol/profile.hpp"

#include <string>
#include <utility>
#include <vector>

namespace Forever
{
/**
 * IClientProfile backed by an in-memory ClientProfileInfo (loaded via TryLoadProfileExtraInserts).
 * ReloadFromDisk re-reads extra_inserts.conf from sourceRoot/profileId.
 */
class ProfileApiAdapter final : public aeternum::protocol::IClientProfile
{
public:
    ProfileApiAdapter() = default;

    ProfileApiAdapter(std::string sourceRoot, ClientProfileInfo info)
        : _sourceRoot(std::move(sourceRoot)), _info(std::move(info))
    {
    }

    static bool TryCreate(std::string const& sourceRoot, std::string const& profileId, ProfileApiAdapter& out)
    {
        ClientProfileInfo info;
        if (!TryLoadProfileExtraInserts(sourceRoot, profileId, info) || !info.Loaded)
            return false;
        out = ProfileApiAdapter(sourceRoot, std::move(info));
        return true;
    }

    std::string_view ProfileId() const override { return _info.ProfileId; }

    aeternum::protocol::BuildId Build() const override
    {
        return aeternum::protocol::BuildId{ _info.ClientBuild };
    }

    std::vector<std::uint32_t> ClientExtraInserts() const override
    {
        return std::vector<std::uint32_t>(_info.ClientExtraInserts.begin(), _info.ClientExtraInserts.end());
    }

    std::vector<std::uint32_t> ServerExtraInserts() const override
    {
        return std::vector<std::uint32_t>(_info.ServerExtraInserts.begin(), _info.ServerExtraInserts.end());
    }

    aeternum::Status ReloadFromDisk() override
    {
        ClientProfileInfo info;
        if (!TryLoadProfileExtraInserts(_sourceRoot, _info.ProfileId, info) || !info.Loaded)
            return aeternum::Status::NotReady;
        _info = std::move(info);
        return aeternum::Status::Ok;
    }

    ClientProfileInfo const& Info() const { return _info; }

private:
    std::string _sourceRoot;
    ClientProfileInfo _info;
};

/**
 * IOpcodeRegistry for ExtraInserts + main-group remap only (not full ClassicOpcodes tables).
 * Matches WireToCoreAfterClientInserts / CoreToWireAfterClientInserts bit-for-bit.
 */
class OpcodeRegistryAdapter final : public aeternum::protocol::IOpcodeRegistry
{
public:
    OpcodeRegistryAdapter(std::string profileId, std::uint32_t build, std::vector<uint32> clientInserts)
        : _profileId(std::move(profileId)), _build(build), _clientInserts(std::move(clientInserts))
    {
    }

    explicit OpcodeRegistryAdapter(ClientProfileInfo const& info)
        : OpcodeRegistryAdapter(info.ProfileId, info.ClientBuild, info.ClientExtraInserts)
    {
    }

    aeternum::protocol::BuildId Build() const override
    {
        return aeternum::protocol::BuildId{ _build };
    }

    std::string_view ProfileId() const override { return _profileId; }

    aeternum::Result<aeternum::protocol::CoreOpcode> WireToCore(aeternum::protocol::WireOpcode wire) const override
    {
        aeternum::Result<aeternum::protocol::CoreOpcode> r;
        uint32 const core = OpcodeRemap::WireToCoreAfterClientInserts(wire.raw, _clientInserts);
        if (core == OpcodeRemap::InvalidClassicOpcode)
        {
            r.status = aeternum::Status::NotFound;
            r.message = "wire opcode is an ExtraInsert hole";
            return r;
        }
        r.status = aeternum::Status::Ok;
        r.value = aeternum::protocol::CoreOpcode{ core };
        return r;
    }

    aeternum::Result<aeternum::protocol::WireOpcode> CoreToWire(aeternum::protocol::CoreOpcode core) const override
    {
        aeternum::Result<aeternum::protocol::WireOpcode> r;
        uint32 const wire = OpcodeRemap::CoreToWireAfterClientInserts(core.raw, _clientInserts);
        r.status = aeternum::Status::Ok;
        r.value = aeternum::protocol::WireOpcode{ wire };
        return r;
    }

    aeternum::protocol::HandlerStatus HandlerOf(aeternum::protocol::CoreOpcode) const override
    {
        // Opcode table / STATUS_* binding is intentionally out of scope for this thin adapter.
        return aeternum::protocol::HandlerStatus::Unhandled;
    }

private:
    std::string _profileId;
    std::uint32_t _build = 0;
    std::vector<uint32> _clientInserts;
};
}

#endif
