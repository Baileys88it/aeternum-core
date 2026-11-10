/*
 * Aeternum Module SDK - static host (M7). Discovers registered IModules,
 * plans order/compat, runs DISCOVER->LOAD->INIT->START and reverse stop.
 * M1-Blocco3: ModuleManager FSM tracks Discovered->Validated->Loaded->Active->Error.
 * IE-M3 Blocco1: optional OS-level native loader (native_runtime.hpp), compiled in only with
 * CMake AETERNUM_NATIVE_MODULE_RUNTIME=ON (default ON since Blocco3) and enabled by Aeternum.Modules.NativeRuntime=1.
 * Static IModule path unchanged. No IModule -> C FT bridge (maintainer Q7: permanently forbidden).
 */
#include "aeternum/sdk/registry.hpp"
#include "aeternum/sdk/internal/static_runtime.hpp"
#include "aeternum/sdk/internal/manifest_gate.hpp"
#include "aeternum/sdk/internal/module_manager.hpp"
#include "Forever/ClientProfile.h"
#include "Log.h"
#include "aeternum/api/ops/services.hpp"
#include "TrinityOpsAdapter.h"
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
#include "aeternum/sdk/internal/native_runtime.hpp"
#endif

#include "aeternum/sdk/internal/wasm_sandbox.hpp"
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace aeternum::sdk
{
namespace
{
std::mutex g_mutex;
std::vector<IModule*> g_modules;
std::vector<std::string> g_startedOrder;
bool g_running = false;
internal::wasm::Sandbox g_wasm;
internal::ModuleManager g_manager;
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
internal::native::NativeModuleRuntime g_native;
bool g_nativeShutdownDeferred=false;

void NativeHostLog(internal::native::NativeLogLevel level, char const* message)
{
    try
    {
        switch (level)
        {
            case internal::native::NativeLogLevel::Error: TC_LOG_ERROR("module.sdk.native", "{}", message); break;
            case internal::native::NativeLogLevel::Warn:  TC_LOG_WARN("module.sdk.native", "{}", message); break;
            default:                                      TC_LOG_INFO("module.sdk.native", "{}", message); break;
        }
    }
    catch (...)
    {
        // D11: never let a logging failure unwind into module code.
    }
}
#endif

char const* StageName(LifecycleStage s)
{
    switch (s)
    {
        case LifecycleStage::Discover: return "DISCOVER";
        case LifecycleStage::Load: return "LOAD";
        case LifecycleStage::Init: return "INIT";
        case LifecycleStage::Start: return "START";
        case LifecycleStage::Stop: return "STOP";
        case LifecycleStage::Shutdown: return "SHUTDOWN";
        default: return "?";
    }
}

internal::PlannedModule ToPlanned(IModule* m)
{
    auto const& d = m->GetDescriptor();
    internal::PlannedModule p;
    p.id = d.identity.id ? d.identity.id : "";
    p.critical = d.critical;
    if (d.dependencies)
        for (char const* const* dep = d.dependencies; *dep; ++dep)
            p.dependencies.emplace_back(*dep);
    if (d.supportedProfiles)
        for (char const* const* pr = d.supportedProfiles; *pr; ++pr)
            p.supportedProfiles.emplace_back(*pr);
    return p;
}

IModule* FindById(std::string const& id)
{
    for (IModule* m : g_modules)
        if (m && m->GetDescriptor().identity.id && id == m->GetDescriptor().identity.id)
            return m;
    return nullptr;
}

bool Invoke(IModule* m, LifecycleStage stage)
{
    ModuleResult r = ModuleResult::Ok;
    switch (stage)
    {
        case LifecycleStage::Load: r = m->OnLoad(); break;
        case LifecycleStage::Init: r = m->OnInit(); break;
        case LifecycleStage::Start: r = m->OnStart(); break;
        case LifecycleStage::Stop: r = m->OnStop(); break;
        case LifecycleStage::Shutdown: r = m->OnShutdown(); break;
        default: break;
    }
    char const* id = m->GetDescriptor().identity.id ? m->GetDescriptor().identity.id : "?";
    if (r == ModuleResult::Failed)
    {
        TC_LOG_ERROR("module.sdk", "Module {} {} FAILED", id, StageName(stage));
        return false;
    }
    TC_LOG_WARN("module.sdk", "Module {} {} ok", id, StageName(stage));
    return true;
}

} // anon

void RegisterModule(IModule* module)
{
    if (!module)
        return;
    std::lock_guard<std::mutex> g(g_mutex);
    for (IModule* existing : g_modules)
        if (existing == module)
            return;
    g_modules.push_back(module);
}

std::size_t RegisteredModuleCount()
{
    std::lock_guard<std::mutex> g(g_mutex);
    return g_modules.size();
}

std::vector<ModuleListEntry> ListRegisteredModules()
{
    std::lock_guard<std::mutex> g(g_mutex);
    std::vector<ModuleListEntry> out;
    out.reserve(g_modules.size());
    for (IModule* m : g_modules)
    {
        if (!m)
            continue;
        auto const& d = m->GetDescriptor();
        ModuleListEntry e;
        e.id = d.identity.id ? d.identity.id : "";
        e.version = d.identity.version ? d.identity.version : "";
        e.critical = d.critical;
        e.started = false;
        for (auto const& sid : g_startedOrder)
            if (sid == e.id) { e.started = true; break; }
        out.push_back(std::move(e));
    }
    return out;
}

std::vector<ModuleLifecycleEntry> ListModuleLifecycle()
{
    std::lock_guard<std::mutex> g(g_mutex);
    std::vector<ModuleLifecycleEntry> out;
    out.reserve(g_manager.Size());
    for (auto const& r : g_manager.Records())
    {
        ModuleLifecycleEntry e;
        e.id = r.id;
        e.critical = r.critical;
        e.state = internal::ModuleFsmStateName(r.state);
        e.reason = r.reason;
        out.push_back(std::move(e));
    }
    return out;
}

namespace
{
char const* ManifestLoadPolicyName(ManifestLoadPolicy policy)
{
    switch (policy)
    {
        case ManifestLoadPolicy::OnBootOnly: return "on-boot-only";
        case ManifestLoadPolicy::RuntimeRestart: return "runtime-restart";
        case ManifestLoadPolicy::HotReloadable: return "hot-reloadable";
        default: return "unknown";
    }
}
}

void HostDiscoverAndStart()
{
    HostDiscoverAndStart(internal::ManifestBootOptions{});
}

void HostDiscoverAndStart(internal::ManifestBootOptions const& options)
{
    std::lock_guard<std::mutex> g(g_mutex);
    if (g_running)
        return;

    g_manager.Clear();

    TC_LOG_WARN("module.sdk", "Module SDK host DISCOVER ({} registered, sdk {}.{}.{} draft)",
        uint32(g_modules.size()),
        AETERNUM_SDK_VERSION_MAJOR, AETERNUM_SDK_VERSION_MINOR, AETERNUM_SDK_VERSION_PATCH);

    std::vector<internal::PlannedModule> planned;
    planned.reserve(g_modules.size());
    for (IModule* m : g_modules)
    {
        internal::PlannedModule p = ToPlanned(m);
        // FSM: registry sighting => Discovered (dup id refused; keep planner input anyway)
        if (!p.id.empty())
            g_manager.Discover(p.id, p.critical);
        planned.push_back(std::move(p));
    }

    std::filesystem::path const manifestDir = options.manifestDir.empty()
        ? std::filesystem::path("modules") : std::filesystem::path(options.manifestDir);

    // M1-Blocco2 (D9): module.manifest.json gate, boot-only. Fail-closed on invalid manifests.
    // With no registered modules (vanilla) the loop is empty and boot is unaffected.
    if (!planned.empty())
    {

        // Log levels (category "module.sdk.manifest", worldserver.conf.dist: Logger.module.sdk.manifest=3,Console Server):
        // per-module OK = DEBUG, legacy (no manifest) = WARN, rejected = ERROR, one INFO summary line per boot.
        // Visible at INFO without enabling the whole module.sdk category on the console.
        uint32 accepted = 0, legacy = 0, rejected = 0;
        std::vector<internal::ManifestGateResult> gate;
        gate.reserve(planned.size());
        for (auto const& p : planned)
        {
            gate.push_back(internal::EvaluateModuleManifestAt(manifestDir, p.id, options.requireManifest));
            auto const& r = gate.back();
            switch (r.outcome)
            {
                case internal::ManifestGateOutcome::Accepted:
                    ++accepted;
                    TC_LOG_DEBUG("module.sdk.manifest", "Module {} manifest OK (abiVersion {}, loadPolicy {}, sha256 {})",
                        p.id, r.manifest.abiVersion, ManifestLoadPolicyName(r.manifest.loadPolicy),
                        r.manifest.hasSha256 ? "verified" : "absent");
                    break;
                case internal::ManifestGateOutcome::LegacyNoManifest:
                    ++legacy;
                    TC_LOG_WARN("module.sdk.manifest", "Module {} has no module.manifest.json - legacy static load (M1 transitional)", p.id);
                    break;
                default:
                    ++rejected;
                    TC_LOG_ERROR("module.sdk.manifest", "Module {} REJECTED by manifest gate: {} (status {})",
                        p.id, r.reason, uint32(r.status));
                    break;
            }
        }
        TC_LOG_INFO("module.sdk.manifest", "Module manifest gate: {} accepted, {} legacy, {} rejected (dir '{}', requireManifest={})",
            accepted, legacy, rejected, manifestDir.generic_string(), options.requireManifest);

        std::string gateAbort;
        if (internal::ApplyManifestGate(planned, gate, gateAbort))
        {
            internal::ApplyGateResultsToManager(g_manager, planned, gate);
            TC_LOG_ERROR("module.sdk.manifest", "Module SDK aborting startup: {}", gateAbort);
            if (auto* p = aeternum::ops::Services().process) p->StopNow(aeternum::ops::ProcessExit::Error);
            return;
        }
        internal::ApplyGateResultsToManager(g_manager, planned, gate);
    }

    std::string activeProfile = Forever::GetConfiguredActiveProfileId();
    auto plan = internal::PlanStaticModules(std::move(planned), activeProfile);
    internal::ApplyPlanToManager(g_manager, plan);

    for (auto const& m : plan.modules)
        if (!m.enabled)
            TC_LOG_WARN("module.sdk", "Module {} disabled ({})", m.id, m.disableReason);

    if (plan.abortStartup)
    {
        TC_LOG_ERROR("module.sdk", "Module SDK aborting startup: {}", plan.message);
        if (auto* p = aeternum::ops::Services().process) p->StopNow(aeternum::ops::ProcessExit::Error);
        return;
    }

    g_startedOrder.clear();
    for (auto const& id : plan.startOrder)
    {
        IModule* m = FindById(id);
        if (!m)
            continue;
        bool critical = m->GetDescriptor().critical;
        if (!Invoke(m, LifecycleStage::Load) || !Invoke(m, LifecycleStage::Init))
        {
            g_manager.MarkError(id, "lifecycle_load_or_init_failed");
            if (critical)
            {
                TC_LOG_ERROR("module.sdk", "Critical module {} failed during start sequence - aborting", id);
                for (auto it = g_startedOrder.rbegin(); it != g_startedOrder.rend(); ++it)
                    if (IModule* s = FindById(*it))
                    {
                        Invoke(s, LifecycleStage::Stop);
                        Invoke(s, LifecycleStage::Shutdown);
                        g_manager.MarkError(*it, "rolled_back_after_critical_failure");
                    }
                g_startedOrder.clear();
                if (auto* p = aeternum::ops::Services().process) p->StopNow(aeternum::ops::ProcessExit::Error);
                return;
            }
            TC_LOG_WARN("module.sdk", "Non-critical module {} disabled after failure", id);
            continue;
        }
        // Validated -> Loaded after OnLoad+OnInit
        g_manager.MarkLoaded(id);

        if (!Invoke(m, LifecycleStage::Start))
        {
            g_manager.MarkError(id, "lifecycle_start_failed");
            if (critical)
            {
                TC_LOG_ERROR("module.sdk", "Critical module {} failed during start sequence - aborting", id);
                for (auto it = g_startedOrder.rbegin(); it != g_startedOrder.rend(); ++it)
                    if (IModule* s = FindById(*it))
                    {
                        Invoke(s, LifecycleStage::Stop);
                        Invoke(s, LifecycleStage::Shutdown);
                        g_manager.MarkError(*it, "rolled_back_after_critical_failure");
                    }
                g_startedOrder.clear();
                if (auto* p = aeternum::ops::Services().process) p->StopNow(aeternum::ops::ProcessExit::Error);
                return;
            }
            TC_LOG_WARN("module.sdk", "Non-critical module {} disabled after failure", id);
            continue;
        }
        // Loaded -> Active after OnStart
        g_manager.MarkActive(id);
        g_startedOrder.push_back(id);
    }

#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    // IE-M3 Blocco1: native .dll/.so modules, after the static ones (no dependency graph for native yet).
    // Fail-closed per module; never FATAL; only a critical native module aborts boot (M1 semantics).
    if (options.nativeRuntime)
    {
        internal::native::NativeRuntimeOptions nopts;
        nopts.manifestDir = manifestDir;
        nopts.hotReload = options.nativeHotReload;
        nopts.log = &NativeHostLog;
        internal::native::NativeBootResult nres;
        try
        {
            nres = g_native.Boot(g_manager, nopts);
        }
        catch (...)
        {
            nres = internal::native::NativeBootResult{};
            TC_LOG_ERROR("module.sdk.native", "Native module runtime: unexpected exception at boot - native modules skipped");
        }
        TC_LOG_INFO("module.sdk.native", "Native module runtime: {} discovered, {} active, {} rejected/error (dir '{}'){}",
            nres.discovered, nres.active, nres.failed, manifestDir.generic_string(),
            internal::native::IsNativeRuntimeTainted() ? " - process TAINTED" : "");
        if (nres.abortStartup)
        {
            TC_LOG_ERROR("module.sdk", "Module SDK aborting startup: {}", nres.message);
            g_native.Shutdown();
            for (auto it = g_startedOrder.rbegin(); it != g_startedOrder.rend(); ++it)
                if (IModule* s = FindById(*it))
                {
                    Invoke(s, LifecycleStage::Stop);
                    Invoke(s, LifecycleStage::Shutdown);
                    g_manager.MarkError(*it, "rolled_back_after_critical_failure");
                }
            g_startedOrder.clear();
            if (auto* p = aeternum::ops::Services().process) p->StopNow(aeternum::ops::ProcessExit::Error);
            return;
        }
    }
#endif

    // M10: VM allocation belongs to SDK boot, before world worker concurrency.
    // Optional explicitly named sample only. No WASI/native host pointers.
    if (!g_wasm.Initialize([](std::string const& text) { TC_LOG_INFO("module.sdk.wasm", "{}", text); }))
        TC_LOG_ERROR("module.sdk.wasm", "WASM init failed: {}", g_wasm.Error());
    else
    {
        auto sample = manifestDir / "wasm" / "mod_test.wasm";
        std::error_code ec;
        if (std::filesystem::exists(sample, ec) && !g_wasm.LoadAndRun(sample))
            TC_LOG_ERROR("module.sdk.wasm", "WASM sample rejected: {}", g_wasm.Error());
        else
            TC_LOG_INFO("module.sdk.wasm", "WASM VM initialized (stack=65536, maxLinearMemory=1048576, sample={})", std::filesystem::exists(sample, ec));
    }
    g_running = true;
    TC_LOG_WARN("module.sdk", "Module SDK START complete ({} modules running, FSM active={}, error={})",
        uint32(g_startedOrder.size()),
        uint32(g_manager.CountInState(internal::ModuleFsmState::Active)),
        uint32(g_manager.CountInState(internal::ModuleFsmState::Error)));
}

void HostStopAndShutdown()
{
    std::lock_guard<std::mutex> g(g_mutex);
    if (!g_running && g_startedOrder.empty())
        return;

#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    // Natives started last => stop first. FreeLibrary only for clean modules (no fault, in_flight == 0).
    try { g_native.Shutdown(); } catch (...) { }
    if(g_native.ShutdownPending()) {
        g_nativeShutdownDeferred=true;
        TC_LOG_WARN("module.sdk.native","SDK shutdown deferred until native root leases drain (no blocking tick wait)");
        return; // keep manager/host lifetimes intact; next global sync point completes teardown
    }
    g_nativeShutdownDeferred=false;
#endif

    g_wasm.Reset();
    auto stopOrder = internal::ReverseOrder(g_startedOrder);
    for (auto const& id : stopOrder)
        if (IModule* m = FindById(id))
        {
            Invoke(m, LifecycleStage::Stop);
            Invoke(m, LifecycleStage::Shutdown);
        }
    g_startedOrder.clear();
    g_running = false;
    g_manager.Clear();
    TC_LOG_WARN("module.sdk", "Module SDK SHUTDOWN complete");
}

void HostNativeSyncPoint() {
#if defined(AETERNUM_NATIVE_MODULE_RUNTIME)
    bool finishShutdown=false;
    {
        std::unique_lock<std::mutex> lock(g_mutex,std::try_to_lock);
        if(lock.owns_lock() && g_running) {
            try {g_native.SyncPoint();}
            catch(...) {internal::native::CancelNativePause();TC_LOG_ERROR("module.sdk.native","Native reload failed; affected module remains inactive");}
            finishShutdown=g_nativeShutdownDeferred && !g_native.ShutdownPending();
        }
    }
    if(finishShutdown)HostStopAndShutdown();
#endif
}

void RegisterSdkWorldScript()
{
    aeternum::ops::trinity::InstallAndRegisterHostScripts();
}
} // namespace aeternum::sdk

void AddSC_aeternum_module_sdk()
{
    aeternum::sdk::RegisterSdkWorldScript();
}