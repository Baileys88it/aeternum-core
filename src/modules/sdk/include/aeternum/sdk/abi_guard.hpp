/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Aeternum SDK - D11 C ABI exception guard (C++ only).
 * Every C-boundary wrapper must catch std::exception and (...) and return AeternumStatus.
 * Never let a C++ exception cross the module boundary.
 */
#ifndef AETERNUM_SDK_ABI_GUARD_HPP
#define AETERNUM_SDK_ABI_GUARD_HPP

#include "aeternum/abi/AeternumABI.h"

#include <exception>
#include <utility>

namespace aeternum::sdk
{

inline AeternumStatus AbiGuardCatchUnknown() noexcept
{
    return AETERNUM_ERR_INTERNAL;
}

inline AeternumStatus AbiGuardCatchStd(std::exception const&) noexcept
{
    /* Do not log ex.what() here: callers may attach a host logger without secrets. */
    return AETERNUM_ERR_INTERNAL;
}

/**
 * Invoke a noexcept-facing C ABI callable that may throw inside C++.
 * Returns AETERNUM_ERR_INTERNAL on any exception; never rethrows.
 */
template <class Fn>
AeternumStatus InvokeNoexcept(Fn&& fn) noexcept
{
    try
    {
        return static_cast<AeternumStatus>(std::forward<Fn>(fn)());
    }
    catch (std::exception const& ex)
    {
        return AbiGuardCatchStd(ex);
    }
    catch (...)
    {
        return AbiGuardCatchUnknown();
    }
}

} // namespace aeternum::sdk

/* Macro form for hand-written extern "C" wrappers (D11). */
#define AETERNUM_C_API_BEGIN try {
#define AETERNUM_C_API_END \
    } catch (std::exception const&) { \
        return AETERNUM_ERR_INTERNAL; \
    } catch (...) { \
        return AETERNUM_ERR_INTERNAL; \
    }

#endif /* AETERNUM_SDK_ABI_GUARD_HPP */