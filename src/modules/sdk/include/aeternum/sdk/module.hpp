/*
 * Aeternum Module SDK — public module contract (v0.2 draft, M7).
 * NOT frozen. Static modules only (no dynamic loading in v0).
 */
#ifndef AETERNUM_SDK_MODULE_HPP
#define AETERNUM_SDK_MODULE_HPP

#include "aeternum/sdk/version.h"

#include <cstddef>

namespace aeternum::sdk
{
/** Lifecycle stages for static modules (host-driven). */
enum class LifecycleStage : unsigned char
{
    Discover = 0,
    Load,
    Init,
    Start,
    Stop,
    Shutdown
};

enum class ModuleRuntime : unsigned char
{
    Static = 0
    // Native / Wasm deferred — not in v0
};

struct ModuleIdentity
{
    char const* id = nullptr;       // stable id, e.g. "admin.monitoring"
    char const* version = nullptr;  // module semver string
    char const* sdkRange = nullptr; // documentation string, e.g. ">=0.2.0 <1.0.0"
};

/**
 * Public descriptor — filled by the module, consumed by the static host.
 * - dependencies: nullptr-terminated list of module ids (or nullptr if none)
 * - supportedProfiles: nullptr-terminated list; nullptr/empty means "all profiles"
 * - critical: failure during Load/Init/Start aborts world startup; else disable+log
 */
struct ModuleDescriptor
{
    ModuleIdentity identity{};
    ModuleRuntime runtime = ModuleRuntime::Static;
    bool critical = false;
    char const* const* dependencies = nullptr;
    char const* const* supportedProfiles = nullptr;
};

/** Result codes for lifecycle callbacks. */
enum class ModuleResult : unsigned char
{
    Ok = 0,
    Failed,
    Skipped
};

/**
 * Module instance interface (static). Host calls in order:
 * OnLoad → OnInit → OnStart … OnStop → OnShutdown
 */
class IModule
{
public:
    virtual ~IModule() = default;
    virtual ModuleDescriptor const& GetDescriptor() const = 0;
    virtual ModuleResult OnLoad() { return ModuleResult::Ok; }
    virtual ModuleResult OnInit() { return ModuleResult::Ok; }
    virtual ModuleResult OnStart() { return ModuleResult::Ok; }
    virtual ModuleResult OnStop() { return ModuleResult::Ok; }
    virtual ModuleResult OnShutdown() { return ModuleResult::Ok; }
};
}

#endif
