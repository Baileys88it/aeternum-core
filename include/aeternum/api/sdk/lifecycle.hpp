/*
 * Aeternum Engine public API — module lifecycle (SDK 1.0 frozen).
 * Real consumer: src/modules/sdk ModuleHost + admin_monitoring module.
 * Existing draft headers under src/modules/sdk/include remain draft until host is Trinity-free.
 */
#ifndef AETERNUM_API_SDK_LIFECYCLE_HPP
#define AETERNUM_API_SDK_LIFECYCLE_HPP

#include "aeternum/api/status.hpp"

namespace aeternum::api_sdk
{
/** Frozen 1.0 version for the aeternum/api surface (distinct from draft modules/sdk). */
inline constexpr int kApiVersionMajor = 1;
inline constexpr int kApiVersionMinor = 0;
inline constexpr int kApiVersionPatch = 0;

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
};

struct ModuleIdentity
{
    char const* id = nullptr;
    char const* version = nullptr;
    char const* sdkRange = nullptr; // e.g. ">=1.0.0 <2.0.0"
};

struct ModuleDescriptor
{
    ModuleIdentity identity{};
    ModuleRuntime runtime = ModuleRuntime::Static;
    bool critical = false;
    char const* const* dependencies = nullptr;
    char const* const* supportedProfiles = nullptr;
};

enum class ModuleResult : unsigned char
{
    Ok = 0,
    Failed,
    Skipped
};

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
