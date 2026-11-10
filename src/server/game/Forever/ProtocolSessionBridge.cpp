/*
 * IE-M1 step 3 — ProtocolSession bridge implementation.
 */
#include "Forever/ProtocolSessionBridge.h"
#include "Forever/ProtocolApiAdapter.h"
#include "Forever/ClassicOpcodeRemap.h"
#include "Forever/ClientProfile.h"
#include "Config.h"
#include "Log.h"

#include <memory>
#include <mutex>

namespace Forever::ProtocolSession
{
namespace
{
std::mutex g_mutex;
bool g_enabled = false;
bool g_inited = false;
std::unique_ptr<OpcodeRegistryAdapter> g_registry;
std::unique_ptr<ProfileApiAdapter> g_profile;
std::vector<uint32> g_clientInserts;
std::string g_profileId;
}

bool Enabled()
{
    return g_enabled && g_registry != nullptr;
}

void Shutdown()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_registry.reset();
    g_profile.reset();
    g_clientInserts.clear();
    g_profileId.clear();
    g_enabled = false;
    g_inited = false;
}

void InitAtStartup()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    // Default ON: Catch2 goldens prove OpcodeRegistryAdapter == OpcodeRemap bit-for-bit.
    bool const want = sConfigMgr->GetBoolDefault("Classic.ProtocolApiAdapter.Enable", true);
    g_enabled = false;
    g_registry.reset();
    g_profile.reset();
    g_clientInserts = EffectiveClientExtraInserts();
    g_profileId = GetConfiguredActiveProfileId();
    if (g_profileId.empty())
        g_profileId = "legacy-extra-inserts";

    uint32 build = 0;
    std::string root = ResolveProfileDataRoot();
    if (!GetConfiguredActiveProfileId().empty() && !root.empty())
    {
        ProfileApiAdapter profile;
        if (ProfileApiAdapter::TryCreate(root, GetConfiguredActiveProfileId(), profile))
        {
            g_profile = std::make_unique<ProfileApiAdapter>(std::move(profile));
            build = g_profile->Info().ClientBuild;
            // Prefer profile adapter inserts when loaded (should match Effective*).
            auto fromProfile = g_profile->ClientExtraInserts();
            if (!fromProfile.empty())
                g_clientInserts.assign(fromProfile.begin(), fromProfile.end());
        }
    }

    g_registry = std::make_unique<OpcodeRegistryAdapter>(g_profileId, build, g_clientInserts);
    g_enabled = want;
    g_inited = true;

    TC_LOG_INFO("server.loading",
        "Forever ProtocolApiAdapter session bridge: enable={} profile='{}' client_inserts={} (zero wire change via Remap)",
        g_enabled ? 1 : 0, g_profileId, uint32(g_clientInserts.size()));
}

uint32 RemoveClientExtraInserts(uint32 classicOpcode)
{
    // Always use the same Remap helper; inserts come from the bridge snapshot when inited,
    // else fall back to Effective* (startup races / tests).
    std::vector<uint32> const& inserts = g_inited ? g_clientInserts : EffectiveClientExtraInserts();
    return OpcodeRemap::RemoveClientExtraInserts(classicOpcode, inserts);
}

aeternum::protocol::IOpcodeRegistry const* Registry()
{
    return g_registry.get();
}

aeternum::protocol::IClientProfile const* Profile()
{
    return g_profile.get();
}

std::vector<uint32> const& ClientInsertsSnapshot()
{
    return g_clientInserts;
}

std::string const& ProfileIdSnapshot()
{
    return g_profileId;
}
}