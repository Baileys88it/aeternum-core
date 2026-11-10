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
 * Aeternum IE-M3 Blocco1 - INTERNAL F1 fault guard for core -> native module crossings (pure C).
 *
 * The trampolines live in C translation units (no C++ objects to unwind in the guarded frame):
 *   Windows: native/NativeGuardWin.c   - __try/__except, filter accepts only containable codes
 *            (AV, int/flt div0, int overflow, illegal/privileged insn, bounds, misalignment, in-page,
 *            stack overflow, MSVC C++ exception thrown BY the module image) whose address lies in the
 *            module image. BREAKPOINT / single-step / fail-fast / heap corruption / GS are never masked.
 *   POSIX:   native/NativeGuardPosix.c - thread-local sigsetjmp armed only during the call,
 *            SIGSEGV/SIGBUS/SIGFPE/SIGILL on a sigaltstack, PC checked against the module image.
 * A fault whose address is outside the image (core code) is NOT swallowed: normal crash path.
 * C++ InvokeNoexcept (abi_guard.hpp, D11) alone does not catch an access violation.
 *
 * Blocco1: boot path (entry + on_load/on_init/on_start + on_stop/on_shutdown).
 * Blocco2: packet hooks (aeternum_guard_call_hook) on world / map / network threads. POSIX installs the signal
 * handlers once (aeternum_guard_enable_persistent) before hooks are published; Windows SEH is per-frame.
 * Module authors must not include this header.
 */
#ifndef AETERNUM_SDK_INTERNAL_NATIVE_GUARD_H
#define AETERNUM_SDK_INTERNAL_NATIVE_GUARD_H

#include "aeternum/abi/AeternumABI.h"

#include <stddef.h>
#include <stdint.h>

AETERNUM_EXTERN_C_BEGIN

/** [base, end) of the mapped module image. A fault is contained only if its PC/address is inside. */
typedef struct AeternumGuardImage {
    uintptr_t base;
    uintptr_t end;
} AeternumGuardImage;

/** Filled by the guard. faulted=1 => the call was aborted by a contained fault. */
typedef struct AeternumGuardFault {
    uint32_t  faulted;
    uint32_t  code;    /* Windows: NTSTATUS exception code; POSIX: signal number */
    uintptr_t address; /* faulting instruction address (inside the image when faulted=1) */
} AeternumGuardFault;

typedef AeternumStatus (AETERNUM_ABI_CALL *AeternumGuardEntryFn)(
    uint32_t host_abi_version, AeternumHostFT const* host, AeternumModuleEntryResult* out);
typedef AeternumStatus (AETERNUM_ABI_CALL *AeternumGuardCtxFn)(void* mod_ctx);

/** 1 when this build has a working trampoline for the platform (Windows x64 SEH, Linux x86_64/aarch64). */
int aeternum_guard_platform_supported(void);

/**
 * Guarded call of aeternum_module_entry. Returns the module status, or AETERNUM_ERR_INTERNAL with
 * fault->faulted=1 when a containable fault inside the image aborted the call.
 */
AeternumStatus aeternum_guard_call_entry(AeternumGuardImage const* image, AeternumGuardEntryFn fn,
                                         uint32_t host_abi_version, AeternumHostFT const* host,
                                         AeternumModuleEntryResult* out, AeternumGuardFault* fault);

/** Guarded call of one AeternumModuleFT lifecycle slot (on_load/on_init/on_start/on_stop/on_shutdown). */
AeternumStatus aeternum_guard_call_ctx(AeternumGuardImage const* image, AeternumGuardCtxFn fn, void* mod_ctx,
                                       AeternumGuardFault* fault);

/**
 * IE-M3 Blocco2: guarded call of one packet hook. *verdict receives the hook result (AETERNUM_HOOK_PASS when the
 * call faulted). Returns AETERNUM_OK, or AETERNUM_ERR_INTERNAL with fault->faulted=1 on a contained fault.
 */
AeternumStatus aeternum_guard_call_hook(AeternumGuardImage const* image, AeternumPacketHookFn fn, void* mod_ctx,
                                        AeternumPacketView const* view, uint32_t* verdict, AeternumGuardFault* fault);

AeternumStatus aeternum_guard_call_combat(AeternumGuardImage const* image, AeternumCombatHookFn fn, void* mod_ctx,
    AeternumCombatView const* view, uint32_t* verdict, AeternumGuardFault* fault);
AeternumStatus aeternum_guard_call_spell(AeternumGuardImage const* image, AeternumSpellHookFn fn, void* mod_ctx,
    AeternumSpellView const* view, uint32_t* verdict, AeternumGuardFault* fault);

/** POSIX: install the fault handlers process-wide once (multi-threaded hot path). Windows: no-op. 1 = ok. */
int aeternum_guard_enable_persistent(void);

AETERNUM_EXTERN_C_END

#endif /* AETERNUM_SDK_INTERNAL_NATIVE_GUARD_H */
