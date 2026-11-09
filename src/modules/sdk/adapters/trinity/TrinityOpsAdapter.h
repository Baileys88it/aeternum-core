/*
 * TrinityCore adapter for aeternum/api ops (IE-M1 debt clearance).
 * ONLY this seat may include World.h / ScriptMgr.h / DatabaseEnv.h / Metric*.
 * Host (ModuleHost) and admin_monitoring must use aeternum/api/ops/* instead.
 */
#ifndef AETERNUM_SDK_ADAPTERS_TRINITY_OPS_ADAPTER_H
#define AETERNUM_SDK_ADAPTERS_TRINITY_OPS_ADAPTER_H

namespace aeternum::ops::trinity
{
/** Install HostServices ports + register WorldScript hooks (SDK host lifecycle). */
void InstallAndRegisterHostScripts();
}

#endif