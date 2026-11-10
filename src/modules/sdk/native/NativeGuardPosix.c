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
 * Aeternum IE-M3 Blocco1 - F1 signal trampoline (Linux). Pure C.
 *
 * - Thread-local sigjmp_buf armed ONLY for the duration of one module call.
 * - SIGSEGV/SIGBUS/SIGFPE/SIGILL handlers (SA_SIGINFO | SA_ONSTACK) installed for the call and the previous
 *   dispositions restored afterwards (boot path, single thread). IE-M3 Blocco2: before packet hooks are published
 *   aeternum_guard_enable_persistent() installs the handlers ONCE process-wide (hooks run on several threads);
 *   from then on a call only arms its thread-local state. A per-thread sigaltstack is created on the first guarded
 *   call of that thread (one 64 KiB allocation per thread, never per packet) so a stack overflow can be handled.
 * - The handler recovers ONLY when this thread is armed AND the faulting PC lies inside the module image.
 *   Anything else is forwarded to the previous disposition (core crash path unchanged).
 */
#if !defined(_WIN32)

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "aeternum/sdk/internal/native_guard.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
#  define AETERNUM_GUARD_POSIX_SUPPORTED 1
#else
#  define AETERNUM_GUARD_POSIX_SUPPORTED 0
#endif

#define AETERNUM_GUARD_NSIG 4
#define AETERNUM_GUARD_ALTSTACK_BYTES (64u * 1024u)

static int const k_guard_signals[AETERNUM_GUARD_NSIG] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL };

/* Previous dispositions for the duration of the guarded call (boot path: one call at a time). */
static struct sigaction g_guard_prev[AETERNUM_GUARD_NSIG];

static __thread sigjmp_buf* t_guard_jmp;
static __thread AeternumGuardImage const* t_guard_image;
static __thread AeternumGuardFault* t_guard_fault;
static __thread volatile sig_atomic_t t_guard_armed;
static __thread void* t_guard_altstack;
static volatile sig_atomic_t g_guard_persistent = 0;

static uintptr_t aeternum_guard_pc(void* ucv)
{
#if AETERNUM_GUARD_POSIX_SUPPORTED
    ucontext_t const* uc = (ucontext_t const*)ucv;
    if (!uc)
        return 0u;
#  if defined(__x86_64__)
    return (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#  else
    return (uintptr_t)uc->uc_mcontext.pc;
#  endif
#else
    (void)ucv;
    return 0u;
#endif
}

static void aeternum_guard_forward(int sig, siginfo_t* si, void* uc)
{
    int i;
    for (i = 0; i < AETERNUM_GUARD_NSIG; ++i)
    {
        if (k_guard_signals[i] != sig)
            continue;
        if ((g_guard_prev[i].sa_flags & SA_SIGINFO) && g_guard_prev[i].sa_sigaction)
        {
            g_guard_prev[i].sa_sigaction(sig, si, uc);
            return;
        }
        if (g_guard_prev[i].sa_handler == SIG_IGN)
            return;
        if (g_guard_prev[i].sa_handler != SIG_DFL && g_guard_prev[i].sa_handler)
        {
            g_guard_prev[i].sa_handler(sig);
            return;
        }
        break;
    }
    /* Default action: reset and return; the faulting instruction re-executes and takes the default path. */
    signal(sig, SIG_DFL);
}

static void aeternum_guard_handler(int sig, siginfo_t* si, void* uc)
{
    uintptr_t const pc = aeternum_guard_pc(uc);
    if (t_guard_armed && t_guard_jmp && t_guard_image && t_guard_fault
        && pc >= t_guard_image->base && pc < t_guard_image->end)
    {
        t_guard_armed = 0;
        t_guard_fault->faulted = 1u;
        t_guard_fault->code = (uint32_t)sig;
        t_guard_fault->address = pc;
        siglongjmp(*t_guard_jmp, 1);
    }
    aeternum_guard_forward(sig, si, uc); /* fault in core code: never swallowed */
}

static int aeternum_guard_ensure_altstack(void)
{
    stack_t cur;
    stack_t ss;
    if (sigaltstack(NULL, &cur) == 0 && !(cur.ss_flags & SS_DISABLE))
        return 1; /* thread already has one (e.g. installed by the core or the test runner) */
    if (!t_guard_altstack)
    {
        t_guard_altstack = malloc(AETERNUM_GUARD_ALTSTACK_BYTES); /* per thread, kept for the thread lifetime */
        if (!t_guard_altstack)
            return 0;
    }
    memset(&ss, 0, sizeof(ss));
    ss.ss_sp = t_guard_altstack;
    ss.ss_size = AETERNUM_GUARD_ALTSTACK_BYTES;
    ss.ss_flags = 0;
    return sigaltstack(&ss, NULL) == 0 ? 1 : 0;
}

static int aeternum_guard_install(void)
{
    int i;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = aeternum_guard_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    for (i = 0; i < AETERNUM_GUARD_NSIG; ++i)
    {
        if (sigaction(k_guard_signals[i], &sa, &g_guard_prev[i]) != 0)
        {
            while (--i >= 0)
                sigaction(k_guard_signals[i], &g_guard_prev[i], NULL);
            return 0;
        }
    }
    return 1;
}

static void aeternum_guard_uninstall(void)
{
    int i;
    for (i = 0; i < AETERNUM_GUARD_NSIG; ++i)
        sigaction(k_guard_signals[i], &g_guard_prev[i], NULL);
}

int aeternum_guard_platform_supported(void)
{
    return AETERNUM_GUARD_POSIX_SUPPORTED;
}

int aeternum_guard_enable_persistent(void)
{
    if (!AETERNUM_GUARD_POSIX_SUPPORTED)
        return 0;
    if (g_guard_persistent)
        return 1;
    if (!aeternum_guard_install())
        return 0;
    g_guard_persistent = 1;
    return 1;
}

typedef struct AeternumGuardCall {
    int kind; /* 0 = entry, 1 = ctx, 2 = packet hook */
    AeternumGuardEntryFn entry;
    AeternumGuardCtxFn ctxFn;
    AeternumPacketHookFn hookFn;
    AeternumPacketView const* view;
    AeternumCombatHookFn combatFn;
    AeternumCombatView const* combatView;
    AeternumSpellHookFn spellFn;
    AeternumSpellView const* spellView;
    uint32_t verdict;
    uint32_t host_abi_version;
    AeternumHostFT const* host;
    AeternumModuleEntryResult* out;
    void* mod_ctx;
} AeternumGuardCall;

static AeternumStatus aeternum_guard_run(AeternumGuardImage const* image, AeternumGuardCall* call,
                                         AeternumGuardFault* fault)
{
    sigjmp_buf jb;
    AeternumStatus volatile st = AETERNUM_ERR_INTERNAL;
    sigjmp_buf* volatile prevJmp = t_guard_jmp;
    AeternumGuardImage const* volatile prevImage = t_guard_image;
    AeternumGuardFault* volatile prevFault = t_guard_fault;
    sig_atomic_t const prevArmed = t_guard_armed;

    fault->faulted = 0u;
    fault->code = 0u;
    fault->address = 0u;
    if (!AETERNUM_GUARD_POSIX_SUPPORTED)
        return AETERNUM_ERR_UNSUPPORTED;
    int const perCall = g_guard_persistent ? 0 : 1;
    if (!aeternum_guard_ensure_altstack() || (perCall && !aeternum_guard_install()))
        return AETERNUM_ERR_INTERNAL;

    t_guard_jmp = &jb;
    t_guard_image = image;
    t_guard_fault = fault;
    if (sigsetjmp(jb, 1) == 0)
    {
        t_guard_armed = 1;
        if (call->kind == 0)
            st = call->entry(call->host_abi_version, call->host, call->out);
        else if (call->kind == 1)
            st = call->ctxFn(call->mod_ctx);
        else
        {
            if (call->kind == 2)
                call->verdict = call->hookFn(call->mod_ctx, call->view);
            else if (call->kind == 3)
                call->verdict = call->combatFn(call->mod_ctx, call->combatView);
            else
                call->verdict = call->spellFn(call->mod_ctx, call->spellView);
            st = AETERNUM_OK;
        }
        t_guard_armed = 0;
    }
    else
    {
        st = AETERNUM_ERR_INTERNAL; /* contained fault: fault->faulted == 1 */
    }
    t_guard_armed = prevArmed;
    t_guard_jmp = prevJmp;
    t_guard_image = prevImage;
    t_guard_fault = prevFault;
    if (perCall)
        aeternum_guard_uninstall();
    return st;
}

AeternumStatus aeternum_guard_call_entry(AeternumGuardImage const* image, AeternumGuardEntryFn fn,
                                         uint32_t host_abi_version, AeternumHostFT const* host,
                                         AeternumModuleEntryResult* out, AeternumGuardFault* fault)
{
    AeternumGuardCall call;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (!image || !fn || !out)
    {
        fault->faulted = 0u;
        return AETERNUM_ERR_INVALID_ARGUMENT;
    }
    memset(&call, 0, sizeof(call));
    call.kind = 0;
    call.entry = fn;
    call.host_abi_version = host_abi_version;
    call.host = host;
    call.out = out;
    return aeternum_guard_run(image, &call, fault);
}

AeternumStatus aeternum_guard_call_ctx(AeternumGuardImage const* image, AeternumGuardCtxFn fn, void* mod_ctx,
                                       AeternumGuardFault* fault)
{
    AeternumGuardCall call;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (!image || !fn)
    {
        fault->faulted = 0u;
        return AETERNUM_ERR_INVALID_ARGUMENT;
    }
    memset(&call, 0, sizeof(call));
    call.kind = 1;
    call.ctxFn = fn;
    call.mod_ctx = mod_ctx;
    return aeternum_guard_run(image, &call, fault);
}

AeternumStatus aeternum_guard_call_hook(AeternumGuardImage const* image, AeternumPacketHookFn fn, void* mod_ctx,
                                        AeternumPacketView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumGuardCall call;
    AeternumStatus st;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
    {
        fault->faulted = 0u;
        return AETERNUM_ERR_INVALID_ARGUMENT;
    }
    memset(&call, 0, sizeof(call));
    call.kind = 2;
    call.hookFn = fn;
    call.view = view;
    call.mod_ctx = mod_ctx;
    call.verdict = AETERNUM_HOOK_PASS;
    st = aeternum_guard_run(image, &call, fault);
    if (verdict)
        *verdict = fault->faulted ? (uint32_t)AETERNUM_HOOK_PASS : call.verdict;
    return st;
}

AeternumStatus aeternum_guard_call_combat(AeternumGuardImage const* image, AeternumCombatHookFn fn, void* mod_ctx,
                                        AeternumCombatView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumGuardCall call;
    AeternumStatus st;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
    {
        fault->faulted = 0u;
        return AETERNUM_ERR_INVALID_ARGUMENT;
    }
    memset(&call, 0, sizeof(call));
    call.kind = 3;
    call.combatFn = fn;
    call.combatView = view;
    call.mod_ctx = mod_ctx;
    call.verdict = AETERNUM_HOOK_PASS;
    st = aeternum_guard_run(image, &call, fault);
    if (verdict)
        *verdict = fault->faulted ? (uint32_t)AETERNUM_HOOK_PASS : call.verdict;
    return st;
}

AeternumStatus aeternum_guard_call_spell(AeternumGuardImage const* image, AeternumSpellHookFn fn, void* mod_ctx,
                                        AeternumSpellView const* view, uint32_t* verdict, AeternumGuardFault* fault)
{
    AeternumGuardCall call;
    AeternumStatus st;
    if (!fault)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (verdict)
        *verdict = AETERNUM_HOOK_PASS;
    if (!image || !fn || !view)
    {
        fault->faulted = 0u;
        return AETERNUM_ERR_INVALID_ARGUMENT;
    }
    memset(&call, 0, sizeof(call));
    call.kind = 4;
    call.spellFn = fn;
    call.spellView = view;
    call.mod_ctx = mod_ctx;
    call.verdict = AETERNUM_HOOK_PASS;
    st = aeternum_guard_run(image, &call, fault);
    if (verdict)
        *verdict = fault->faulted ? (uint32_t)AETERNUM_HOOK_PASS : call.verdict;
    return st;
}

#else
typedef int aeternum_native_guard_posix_unused;
#endif /* !_WIN32 */
