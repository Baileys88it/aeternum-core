#include "aeternum/sdk/internal/meshing.hpp"
#include <charconv>
#include <sstream>
#include "SpellMgr.h"
#include "aeternum/sdk/internal/native_ecs.hpp"
#include "LootMgr.h"
#include "aeternum/api/data_loaded.hpp"
#include "ObjectMgr.h"
#include "Log.h"
/*
 * TrinityCore adapter implementing SDK 1.0 ops facades.
 */
#include "TrinityOpsAdapter.h"

#include "aeternum/api/ops/services.hpp"
#include "aeternum/sdk/registry.hpp"
#include "aeternum/sdk/internal/manifest_gate.hpp"

#include "Config.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Metric.h"
#include "MetricLocalStore.h"
#include "ScriptMgr.h"
#include "World.h"
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
#include "aeternum/sdk/internal/native_host.hpp"
#include "aeternum/sdk/internal/data_hooks.hpp"
#endif

#include "aeternum/sdk/internal/telemetry_http.hpp"
#include "aeternum/sdk/internal/profiler.hpp"
#include "aeternum/sdk/internal/wasm_abort.hpp"
#include "AdminTokenResolve.h"
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace aeternum::ops::trinity
{
namespace
{
class TrinityRuntimeStats final : public IRuntimeStats
{
public:
    std::uint64_t UptimeSeconds() const override { return GameTime::GetUptime(); }
    std::uint32_t ActiveSessionCount() const override { return sWorld->GetActiveSessionCount(); }
    std::uint32_t QueuedSessionCount() const override { return sWorld->GetQueuedSessionCount(); }
    std::uint32_t PlayerCount() const override { return sWorld->GetPlayerCount(); }
};

class TrinityProcessControl final : public IProcessControl
{
public:
    void StopNow(ProcessExit code) override
    {
        World::StopNow(static_cast<uint8>(code));
    }

    void SendServerMessage(std::string_view message) override
    {
        sWorld->SendServerMessage(SERVER_MSG_STRING, std::string(message));
    }

    void RequestShutdown(std::uint32_t delaySeconds, std::string_view reason) override
    {
        sWorld->ShutdownServ(delaySeconds, 0, SHUTDOWN_EXIT_CODE, std::string(reason));
    }
};

class TrinityDatabasePing final : public IDatabasePing
{
public:
    bool PingWorld() override
    {
        try { if (auto r = WorldDatabase.Query("SELECT 1")) return true; }
        catch (...) {}
        return false;
    }
    bool PingCharacter() override
    {
        try { if (auto r = CharacterDatabase.Query("SELECT 1")) return true; }
        catch (...) {}
        return false;
    }
    bool PingLogin() override
    {
        try { if (auto r = LoginDatabase.Query("SELECT 1")) return true; }
        catch (...) {}
        return false;
    }
};

class TrinityScriptCatalog final : public IScriptCatalog
{
public:
    std::uint32_t ScriptCount() const override { return sScriptMgr->GetScriptCount(); }
};

class TrinityMetricLocalQuery final : public IMetricLocalQuery
{
public:
    bool LocalSinkEnabled() const override { return sMetric->IsLocalSinkEnabled(); }
    std::uint64_t LocalStoreBytes() const override { return sMetric->GetLocalStoreBytes(); }
    std::uint64_t LocalBudgetBytes() const override { return sMetric->GetLocalBudgetBytes(); }
    std::uint32_t LocalRetentionDays() const override { return sMetric->GetLocalRetentionDays(); }
    std::size_t LocalMaxSeries() const override { return sMetric->GetLocalMaxSeries(); }
    std::uint32_t LocalFileCount() const override { return sMetric->GetLocalFileCount(); }
    std::string LocalDirectory() const override { return sMetric->GetLocalDirectory().generic_string(); }
    bool DenseScrapeEnabled() const override { return sMetric->IsDenseScrapeEnabled(); }
    std::uint64_t DenseSampleCount() const override { return sMetric->GetDenseSampleCount(); }
    std::uint64_t DenseCpuNanos() const override { return sMetric->GetDenseCpuNanos(); }

    std::vector<MetricSeriesSnapshot> LoadSeriesFile(std::string_view path, std::size_t topN) const override
    {
        std::vector<MetricSeriesSnapshot> out;
        auto snaps = Trinity::MetricLocal::LocalStore::LoadFile(std::filesystem::path(std::string(path)));
        if (topN > 0)
            snaps = Trinity::MetricLocal::TakeTopNBySum(snaps, topN);
        out.reserve(snaps.size());
        for (auto const& s : snaps)
        {
            MetricSeriesSnapshot m;
            m.name = s.Key.Name;
            m.labels = s.Key.Labels;
            m.count = s.Stats.Count;
            m.sum = s.Stats.Sum;
            m.min = s.Stats.Min;
            m.max = s.Stats.Max;
            out.push_back(std::move(m));
        }
        return out;
    }
};

TrinityRuntimeStats g_runtime;
TrinityProcessControl g_process;
TrinityDatabasePing g_database;
TrinityScriptCatalog g_scripts;
TrinityMetricLocalQuery g_metrics;

// M1-Blocco2 (D9): manifest gate options, read once at startup. Keys are optional (quiet defaults).
//   Aeternum.Modules.ManifestDir     = "" => <directory of worldserver.conf>/modules
//   Aeternum.Modules.RequireManifest = 0  => missing manifest = legacy load + WARN; 1 => missing = rejected
//   Aeternum.Modules.NativeRuntime   = 0  => IE-M3 native .dll/.so loader off (only read if built with
//                                           AETERNUM_NATIVE_MODULE_RUNTIME=ON, CMake default ON since Blocco3)
aeternum::sdk::internal::ManifestBootOptions ResolveManifestBootOptions()
{
    aeternum::sdk::internal::ManifestBootOptions o;
    o.manifestDir = sConfigMgr->GetStringDefault("Aeternum.Modules.ManifestDir", "", true);
    if (o.manifestDir.empty())
        o.manifestDir = (std::filesystem::path(sConfigMgr->GetFilename()).parent_path() / "modules").string();
    o.requireManifest = sConfigMgr->GetBoolDefault("Aeternum.Modules.RequireManifest", false, true);
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    // IE-M3 Blocco3: OS-level native loader, second switch (config). Default 0 even when compiled in (empty = no-op).
    o.nativeRuntime = sConfigMgr->GetBoolDefault("Aeternum.Modules.NativeRuntime", false, true);
    o.nativeHotReload = sConfigMgr->GetBoolDefault("Aeternum.Modules.HotReload", false, true);
#endif
    return o;
}

#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
std::unique_ptr<Aeternum::Ecs::MapDriver> ResolveOwnedEcsMap(uint32 map,uint32 instance)
{
    // Explicit opt-in AND one exact map/instance; no wildcard/global amputation.
    if(!sConfigMgr->GetBoolDefault("Aeternum.ECS.Enabled",false)) return nullptr;
    auto ownedMap=sConfigMgr->GetIntDefault("Aeternum.ECS.OwnedMapId",-1);
    auto ownedInstance=sConfigMgr->GetIntDefault("Aeternum.ECS.OwnedInstanceId",-1);
    if(ownedMap<0 || ownedInstance<0 || uint32(ownedMap)!=map || uint32(ownedInstance)!=instance) return nullptr;
    auto capacity=sConfigMgr->GetIntDefault("Aeternum.ECS.Capacity",1024);
    auto workers=sConfigMgr->GetIntDefault("Aeternum.ECS.Workers",16);
    if(capacity<1||capacity>4096||workers<1||workers>16) return nullptr;
    aeternum::ecs::Options options;options.capacity=uint32(capacity);options.workers=uint32(workers);
    static_assert(sizeof(ObjectGuid)==sizeof(AeternumGuid128),"ECS GUID must preserve all 128 bits");
    return aeternum::sdk::internal::native::CreateOwnedEcsMap(map,instance,options);
}
#endif
// Called by Aeternum::OnDataLoaded after script registration, before gameplay workers.
void BootStaticDataOverrides()
{
    aeternum::sdk::HostDiscoverAndStart(ResolveManifestBootOptions());
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    using namespace aeternum::sdk::internal::native;
    DataBackend backend{
        [](uint32 id,AeternumCreatureHPView* out){return sObjectMgr->AeternumGetCreatureHP(id,out);},
        [](uint32 id,uint32 hp){return sObjectMgr->AeternumSetCreatureHP(id,hp);},
        [](uint32 id,uint32 type,int32* out){return sObjectMgr->AeternumGetItemStat(id,type,out);},
        [](uint32 id,uint32 type,int32 value){return sObjectMgr->AeternumSetItemStat(id,type,value);},
        [](uint32 id,AeternumWeaponDamageView* out){return sObjectMgr->AeternumGetWeaponDamage(id,out);},
        [](uint32 id,float lo,float hi){return sObjectMgr->AeternumSetWeaponDamage(id,lo,hi);},
        [](uint32 id,AeternumSpellRecoveryView* out){return sSpellMgr->AeternumGetSpellRecovery(id,out);},
        [](uint32 id,uint32 ms){return sSpellMgr->AeternumSetSpellRecovery(id,ms);},
        [](uint32 id,uint32 item,AeternumLootChanceView* out){return LootTemplates_Creature.AeternumGetItemChance(id,item,out);},
        [](uint32 id,uint32 item,float chance){return LootTemplates_Creature.AeternumSetItemChance(id,item,chance);}
    };
    auto status=ConfigureDataBackend(backend);
    if (status==AETERNUM_OK) status=DispatchDataLoaded();
    TC_LOG_INFO("module.sdk.native","M9 OnDataLoaded boot event status={}",status);
    if(EnableEcsForBoot()) Aeternum::Ecs::factory.store(&ResolveOwnedEcsMap,std::memory_order_release);
#endif
}

aeternum::sdk::internal::telemetry::Server& TelemetryServer() {
 // Construct shared data before the server: static teardown stops/joins HTTP
 // before profiler/abort-registry destruction, including failed boot paths.
 static bool const dataReady=[] {
  (void)aeternum::sdk::internal::AeternumProfiler::Instance();
  (void)aeternum::sdk::internal::wasm::SnapshotModules();return true;
 }();
 (void)dataReady;static aeternum::sdk::internal::telemetry::Server server;return server;
}
std::string TelemetryAdminToken(bool op) {
 auto key=op?"Admin.OperatorToken":"Admin.Token";
 auto envKey=op?"Admin.OperatorTokenFromEnv":"Admin.TokenFromEnv";
 auto fallback=op?AdminMonitoring::kDefaultAdminOperatorTokenEnvVar:AdminMonitoring::kDefaultAdminTokenEnvVar;
 auto name=AdminMonitoring::EffectiveTokenEnvVarName(sConfigMgr->GetStringDefault(envKey,fallback),fallback);
 std::string env;
#if defined(_WIN32)
 char* value=nullptr;std::size_t size=0;
 if(_dupenv_s(&value,&size,name.c_str())==0 && value)env=value;
 std::free(value);
#else
 if(auto value=std::getenv(name.c_str()))env=value;
#endif
 auto resolved=AdminMonitoring::ResolveAdminToken(sConfigMgr->GetStringDefault(key,""),env);
 if(resolved.confTokenNonEmpty)TC_LOG_WARN("module.sdk.telemetry","Admin token configured in plaintext; prefer existing Admin.*TokenFromEnv. Value never logged.");
 return resolved.value;
}
void StartTelemetryHttp() {
 // Opt-in only, no automatic reload/rebind or production configuration edits.
 if(!sConfigMgr->GetBoolDefault("Aeternum.TelemetryHttp.Enable",false))return;
 auto port=sConfigMgr->GetIntDefault("Aeternum.TelemetryHttp.Port",0);
 if(port<1 || port>65535){TC_LOG_ERROR("module.sdk.telemetry","Telemetry HTTP refused: invalid port");return;}
 aeternum::sdk::internal::telemetry::Config config;config.enabled=true;config.port=static_cast<std::uint16_t>(port);
 config.bind=sConfigMgr->GetStringDefault("Aeternum.TelemetryHttp.BindIP","127.0.0.1");
 config.readToken=TelemetryAdminToken(false);config.operatorToken=TelemetryAdminToken(true);
 if(!TelemetryServer().Start(std::move(config)))TC_LOG_ERROR("module.sdk.telemetry","Telemetry HTTP refused: loopback bind/auth/listener unavailable; token never logged");
}
aeternum::sdk::internal::meshing::Server& EdgeServer() {
 static aeternum::sdk::internal::meshing::Server server;return server;
}
void StartEdge() {
 // M15: L7 TCP termination is aeternum_gateway.exe (IOCP). Worldserver does not bind the edge.
 if(sConfigMgr->GetBoolDefault("Aeternum.Edge.Enable",false))
  TC_LOG_WARN("module.sdk.edge","Aeternum.Edge.Enable ignored: transport edge is aeternum_gateway (M15), not worldserver");
}

class ModuleSdkWorldScript : public WorldScript
{
public:
    ModuleSdkWorldScript() : WorldScript("AeternumModuleSdkWorldScript") { }
    void OnStartup() override { StartEdge(); StartTelemetryHttp(); aeternum::sdk::HostDiscoverAndStart(ResolveManifestBootOptions()); }
    void OnShutdown() override { EdgeServer().Stop(); TelemetryServer().Stop(); aeternum::sdk::HostStopAndShutdown(); }
    void OnConfigLoad(bool reload) override { aeternum::ops::InvokeConfigReloadHook(reload); }
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    // IE-M3 Blocco2 (policy F6 / maintainer Q6): a contained native fault taints the process and arms a one-shot 60 s
    // clean shutdown. Issued here, on the world thread, through the existing IProcessControl::RequestShutdown
    // (World::ShutdownServ) - never TerminateProcess. Idle cost: one atomic load per world tick.
    void OnUpdate(uint32 /*diff*/) override
    {
        aeternum::sdk::HostNativeSyncPoint();
        aeternum::sdk::internal::native::PumpGracefulShutdown(aeternum::ops::Services().process);
    }
#endif
};

bool g_installed = false;
} // anon

void InstallAndRegisterHostScripts()
{
    if (!g_installed)
    {
        HostServices ports;
        ports.runtime = &g_runtime;
        ports.process = &g_process;
        ports.database = &g_database;
        ports.scripts = &g_scripts;
        ports.metrics = &g_metrics;
        Install(ports);
        g_installed = true;
    }
    Aeternum::DataLoadedBootHook = &BootStaticDataOverrides;
    new ModuleSdkWorldScript();
}
} // namespace aeternum::ops::trinity