/*
 * Aeternum Module SDK — INTERNAL host hooks (v0.2).
 * Module authors must not include this header.
 */
#ifndef AETERNUM_SDK_INTERNAL_HOST_HOOKS_HPP
#define AETERNUM_SDK_INTERNAL_HOST_HOOKS_HPP

#include "aeternum/sdk/version.h"

namespace aeternum::sdk::internal
{
/** Opaque hooks reserved for future metrics/log injection. */
struct HostServices
{
    using LogFn = void (*)(char const* channel, char const* message);
    LogFn logInfo = nullptr;
    LogFn logWarn = nullptr;
    LogFn logError = nullptr;
};
}

#endif
