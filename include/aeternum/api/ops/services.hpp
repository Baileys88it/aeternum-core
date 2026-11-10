/*
 * Aeternum Engine public API — ops service locator (SDK 1.0).
 * Host/admin call through here; Trinity adapter Install()s concrete ports.
 */
#ifndef AETERNUM_API_OPS_SERVICES_HPP
#define AETERNUM_API_OPS_SERVICES_HPP

#include "aeternum/api/ops/metrics.hpp"
#include "aeternum/api/ops/runtime.hpp"

namespace aeternum::ops
{
struct HostServices
{
    IRuntimeStats* runtime = nullptr;
    IProcessControl* process = nullptr;
    IDatabasePing* database = nullptr;
    IScriptCatalog* scripts = nullptr;
    IMetricLocalQuery* metrics = nullptr;
};

inline HostServices& Services()
{
    static HostServices s;
    return s;
}

inline void Install(HostServices ports)
{
    Services() = ports;
}

using ConfigReloadHook = void (*)(bool reload);

inline ConfigReloadHook& ConfigReloadHookSlot()
{
    static ConfigReloadHook hook = nullptr;
    return hook;
}

inline void SetConfigReloadHook(ConfigReloadHook hook)
{
    ConfigReloadHookSlot() = hook;
}

inline void InvokeConfigReloadHook(bool reload)
{
    if (ConfigReloadHook h = ConfigReloadHookSlot())
        h(reload);
}
}

#endif