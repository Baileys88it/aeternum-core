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
 * Aeternum IE-M3 Blocco1 - F1 SEH trampoline (Windows). Pure C: the guarded frame owns no C++ object,
 * so __try/__except never has to unwind C++ state of the core.
 */
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <malloc.h> /* _resetstkoflw */

#include "aeternum/sdk/internal/native_guard.h"

/* MSVC C++ exception ("msc") raised with RaiseException by the module's own throw. */
#define AETERNUM_GUARD_MSVC_CPP_EXCEPTION 0xE06D7363u

static int aeternum_guard_code_containable(DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_OVERFLOW:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_UNDERFLOW:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_FLT_DENORMAL_OPERAND:
        case EXCEPTION_FLT_INEXACT_RESULT:
        case EXCEPTION_STACK_OVERFLOW:
            return 1;
        /* Never masked: EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP, EXCEPTION_GUARD_PAGE,
         * STATUS_HEAP_CORRUPTION (0xC0000374), STATUS_STACK_BUFFER_OVERRUN / fail-fast (0xC0000409),
         * EXCEPTION_NONCONTINUABLE_EXCEPTION, invalid handle, ... => EXCEPTION_CONTINUE_SEARCH. */
        default:
            return 0;
    }
}

static LONG aeternum_guard_filter(EXCEPTION_POINTERS const* ep, AeternumGuardImage const* image,
                                  AeternumGuardFault* fault)
{
    EXCEPTION_RECORD const* er;
    uintptr_t addr;
    int inside;

    if (!ep || !ep->ExceptionRecord || !image || !fault || image->base == 0 || image->end <= image->base)
        return EXCEPTION_CONTINUE_SEARCH;
    er = ep->ExceptionRecord;
    addr = (uintptr_t)er->ExceptionAddress;

    if (er->ExceptionCode == AETERNUM_GUARD_MSVC_CPP_EXCEPTION)
    {
        /* x64 MSVC EH: ExceptionInformation[3] = image base of the module that threw (D11 breach). */
        inside = (er->NumberParameters >= 4u && (uintptr_t)er->ExceptionInformation[3] == image->base) ? 1 : 0;
    }
    else
    {
        if (!aeternum_guard_code_containable(er->ExceptionCode))
            return EXCEPTION_CONTINUE_SEARCH;
        inside = (addr >= image->base && addr < image->end) ? 1 : 0;
    }
    if (!inside)
        return EXCEPTION_CONTINUE_SEARCH; /* fault in core code: never swallowed */

    fault->faulted = 1u;
    fault->code = (uint32_t)er->ExceptionCode;
    fault->address = addr;
    return EXCEPTION_EXECUTE_HANDLER;
}

static void aeternum_guard_reset(AeternumGuardFault* fault)
{
    fault->faulted = 0u;
    fault->code = 0u;
    fault->address = 0u;
}

int aeternum_guard_platform_supported(void)
{
#if defined(_M_X64) || defined(_M_ARM64)
    return 1;
#else
    return 0; /* x86 SEH ExceptionInformation layout differs; native runtime refuses to load */
#endif
}

AeternumStatus aeternum_guard_call_entry(AeternumGuardImage const* image, AeternumGuardEntryFn fn,
                                         uint32_t host_abi_version, AeternumHostFT const* host,
                                         AeternumModuleEntryResult* out, AeternumGuardFault* fault)
{
    AeternumStatus st = AETERNUM_ERR_INTERNAL;
    int stackOverflow = 0;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    aeternum_guard_reset(fault);
    if (!image || !fn || !out)
        return AETERNUM_ERR_INVALID_ARGUMENT;

    __try
    {
        st = fn(host_abi_version, host, out);
    }
    __except (aeternum_guard_filter(GetExceptionInformation(), image, fault))
    {
        st = AETERNUM_ERR_INTERNAL;
        stackOverflow = (fault->code == (uint32_t)EXCEPTION_STACK_OVERFLOW) ? 1 : 0;
    }
    if (stackOverflow)
        _resetstkoflw(); /* outside the __except block: re-arm the guard page */
    return st;
}

AeternumStatus aeternum_guard_call_ctx(AeternumGuardImage const* image, AeternumGuardCtxFn fn, void* mod_ctx,
                                       AeternumGuardFault* fault)
{
    AeternumStatus st = AETERNUM_ERR_INTERNAL;
    int stackOverflow = 0;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    aeternum_guard_reset(fault);
    if (!image || !fn)
        return AETERNUM_ERR_INVALID_ARGUMENT;

    __try
    {
        st = fn(mod_ctx);
    }
    __except (aeternum_guard_filter(GetExceptionInformation(), image, fault))
    {
        st = AETERNUM_ERR_INTERNAL;
        stackOverflow = (fault->code == (uint32_t)EXCEPTION_STACK_OVERFLOW) ? 1 : 0;
    }
    if (stackOverflow)
        _resetstkoflw();
    return st;
}

AeternumStatus aeternum_guard_call_hook(AeternumGuardImage const* image, AeternumPacketHookFn fn, void* mod_ctx,
                                        AeternumPacketView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumStatus st = AETERNUM_OK;
    uint32_t v = AETERNUM_HOOK_PASS;
    int stackOverflow = 0;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    aeternum_guard_reset(fault);
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
        return AETERNUM_ERR_INVALID_ARGUMENT;

    __try
    {
        v = fn(mod_ctx, view);
    }
    __except (aeternum_guard_filter(GetExceptionInformation(), image, fault))
    {
        st = AETERNUM_ERR_INTERNAL;
        v = AETERNUM_HOOK_PASS;
        stackOverflow = (fault->code == (uint32_t)EXCEPTION_STACK_OVERFLOW) ? 1 : 0;
    }
    if (stackOverflow)
        _resetstkoflw();
    if (verdict)
        *verdict = v;
    return st;
}

AeternumStatus aeternum_guard_call_combat(AeternumGuardImage const* image, AeternumCombatHookFn fn, void* mod_ctx,
                                        AeternumCombatView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumStatus st = AETERNUM_OK;
    uint32_t v = AETERNUM_HOOK_PASS;
    int stackOverflow = 0;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    aeternum_guard_reset(fault);
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
        return AETERNUM_ERR_INVALID_ARGUMENT;

    __try
    {
        v = fn(mod_ctx, view);
    }
    __except (aeternum_guard_filter(GetExceptionInformation(), image, fault))
    {
        st = AETERNUM_ERR_INTERNAL;
        v = AETERNUM_HOOK_PASS;
        stackOverflow = (fault->code == (uint32_t)EXCEPTION_STACK_OVERFLOW) ? 1 : 0;
    }
    if (stackOverflow)
        _resetstkoflw();
    if (verdict)
        *verdict = v;
    return st;
}

AeternumStatus aeternum_guard_call_spell(AeternumGuardImage const* image, AeternumSpellHookFn fn, void* mod_ctx,
                                        AeternumSpellView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumStatus st = AETERNUM_OK;
    uint32_t v = AETERNUM_HOOK_PASS;
    int stackOverflow = 0;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    aeternum_guard_reset(fault);
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
        return AETERNUM_ERR_INVALID_ARGUMENT;

    __try
    {
        v = fn(mod_ctx, view);
    }
    __except (aeternum_guard_filter(GetExceptionInformation(), image, fault))
    {
        st = AETERNUM_ERR_INTERNAL;
        v = AETERNUM_HOOK_PASS;
        stackOverflow = (fault->code == (uint32_t)EXCEPTION_STACK_OVERFLOW) ? 1 : 0;
    }
    if (stackOverflow)
        _resetstkoflw();
    if (verdict)
        *verdict = v;
    return st;
}

int aeternum_guard_enable_persistent(void)
{
    return 1; /* SEH is frame-based: nothing process-wide to install */
}

#else
/* ISO C forbids an empty translation unit. */
typedef int aeternum_native_guard_win_unused;
#endif /* _WIN32 */
