/*
 * Aeternum Module SDK - static registration (public, v0.2 draft).
 */
#ifndef AETERNUM_SDK_REGISTRY_HPP
#define AETERNUM_SDK_REGISTRY_HPP

#include "aeternum/sdk/module.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace aeternum::sdk
{
/** Register a static module instance (call from translation unit static init or AddSC_). */
void RegisterModule(IModule* module);

/** Host entry points - wired from ScriptMgr WorldScript in the SDK host. */
void HostDiscoverAndStart();
void HostStopAndShutdown();
void HostNativeSyncPoint(); // global WorldScript::OnUpdate only

/** Test/host introspection */
std::size_t RegisteredModuleCount();

struct ModuleListEntry
{
    std::string id;
    std::string version;
    bool critical = false;
    bool started = false;
};

/** Snapshot of registered modules (and whether currently started). */
std::vector<ModuleListEntry> ListRegisteredModules();

/**
 * M1-Blocco3: FSM snapshot for the current boot cycle
 * (Discovered / Validated / Loaded / Active / Error). See MODULE_MANAGER_DESIGN.md.
 */
struct ModuleLifecycleEntry
{
    std::string id;
    bool critical = false;
    std::string state;
    std::string reason; // non-empty when state == Error
};

std::vector<ModuleLifecycleEntry> ListModuleLifecycle();
}

#endif