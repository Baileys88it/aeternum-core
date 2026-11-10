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
 * Aeternum Engine - Module C ABI (M1; Host FT grown by struct_size in IE-M3; PacketView grown in IE-M5 Blocco1). Pure C11 / C++20.
 * Path: include/aeternum/abi/AeternumABI.h
 * Contract: docs/independent_engine/architecture/modules/DYNAMIC_LOADING_AND_HOT_RELOAD.md section 9.1
 * Decisions: D2 (C boundary), D3 (no STL across DLL), D11 (no exception cross-boundary; see abi_guard.hpp)
 */
#ifndef AETERNUM_ABI_AETERNUM_ABI_H
#define AETERNUM_ABI_AETERNUM_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#  define AETERNUM_EXTERN_C_BEGIN extern "C" {
#  define AETERNUM_EXTERN_C_END   }
#else
#  define AETERNUM_EXTERN_C_BEGIN
#  define AETERNUM_EXTERN_C_END
#endif

#ifndef AETERNUM_ABI_CALL
#  if defined(_MSC_VER)
#    define AETERNUM_ABI_CALL __cdecl
#  else
#    define AETERNUM_ABI_CALL
#  endif
#endif

#ifndef AETERNUM_ABI_EXPORT
#  if defined(AETERNUM_ABI_STATIC)
#    define AETERNUM_ABI_EXPORT
#  elif defined(_WIN32) || defined(__CYGWIN__)
#    if defined(AETERNUM_ABI_EXPORTS)
#      define AETERNUM_ABI_EXPORT __declspec(dllexport)
#    else
#      define AETERNUM_ABI_EXPORT __declspec(dllimport)
#    endif
#  elif defined(__GNUC__) || defined(__clang__)
#    define AETERNUM_ABI_EXPORT __attribute__((visibility("default")))
#  else
#    define AETERNUM_ABI_EXPORT
#  endif
#endif

#if !defined(AETERNUM_ABI_STATIC) && !defined(AETERNUM_ABI_EXPORTS)
#  undef AETERNUM_ABI_EXPORT
#  define AETERNUM_ABI_EXPORT
#endif

AETERNUM_EXTERN_C_BEGIN

#define AETERNUM_MODULE_ABI_VERSION 1u

typedef int32_t AeternumStatus;

enum {
    AETERNUM_OK                    = 0,
    AETERNUM_ERR_INVALID_ARGUMENT  = 1,
    AETERNUM_ERR_NOT_FOUND         = 2,
    AETERNUM_ERR_NOT_READY         = 3,
    AETERNUM_ERR_DENIED            = 4,
    AETERNUM_ERR_UNSUPPORTED       = 5,
    AETERNUM_ERR_INTERNAL          = 6,
    AETERNUM_ERR_BUSY              = 7,
    AETERNUM_ERR_OVERFLOW          = 8,
    AETERNUM_ERR_ABI_MISMATCH      = 100,
    AETERNUM_ERR_MANIFEST_INVALID  = 101,
    AETERNUM_ERR_INTEGRITY         = 102
};

typedef struct AeternumPlayer_Opaque*  AeternumPlayerHandle;
typedef struct AeternumSession_Opaque* AeternumSessionHandle;
typedef struct AeternumModule_Opaque*  AeternumModuleHandle;
typedef struct AeternumMap_Opaque*     AeternumMapHandle;

typedef struct AeternumStringView {
    char const* data;
    uint32_t    len;
} AeternumStringView;

typedef struct AeternumOutBuffer {
    char*    data;
    uint32_t capacity;
    uint32_t length;
} AeternumOutBuffer;

enum {
    AETERNUM_LOAD_POLICY_ON_BOOT_ONLY    = 0,
    AETERNUM_LOAD_POLICY_RUNTIME_RESTART = 1,
    AETERNUM_LOAD_POLICY_HOT_RELOADABLE  = 2
};

typedef struct AeternumModuleInfo {
    char const* id;
    char const* version;
    char const* api_range;
    uint32_t    abi_version;
    uint32_t    load_policy;
    uint32_t    critical;
} AeternumModuleInfo;

typedef struct AeternumModuleFT {
    uint32_t struct_size;
    AeternumStatus (AETERNUM_ABI_CALL *on_load)(void* mod_ctx);
    AeternumStatus (AETERNUM_ABI_CALL *on_init)(void* mod_ctx);
    AeternumStatus (AETERNUM_ABI_CALL *on_start)(void* mod_ctx);
    AeternumStatus (AETERNUM_ABI_CALL *on_stop)(void* mod_ctx);
    AeternumStatus (AETERNUM_ABI_CALL *on_shutdown)(void* mod_ctx);
    AeternumStatus (AETERNUM_ABI_CALL *metrics_observe)(void* mod_ctx, AeternumStringView name, double value);
} AeternumModuleFT;

/*
 * IE-M3 Blocco2 - packet hooks (C ABI, POD only). Opcodes are LOGICAL core opcodes (after the M2 ProtocolAdapter
 * TranslateInbound / before TranslateOutbound), never wire ids. The view and its bytes are valid only for the call.
 */
enum {
    AETERNUM_PACKET_DIR_CMSG = 0, /* client -> server, before QueuePacket / legacy core handler             */
    AETERNUM_PACKET_DIR_SMSG = 1  /* server -> client, before TranslateOutbound (SendPacket)                */
};

/* Hook verdict (ABI v1). IE-M5 Blocco1: CMSG DROP is honored (packet never reaches the legacy handler).
 * SMSG MUTATE replaces the outbound payload via a HostAlloc buffer recorded in view->mutation (Policy F2).
 * SMSG DROP remains ignored (no cancel capability); CMSG MUTATE is malformed and fail-closes the module. */
enum {
    AETERNUM_HOOK_PASS   = 0,
    AETERNUM_HOOK_DROP   = 1,
    AETERNUM_HOOK_MUTATE = 2,
    AETERNUM_OVERRIDE_DAMAGE = 3, /* combat only; host stack mutation buffer */
    AETERNUM_CANCEL_SPELL    = 4  /* spell prepare only */
};

/* M5: host-owned OUT slot for SMSG mutation. Module fills data/size when returning AETERNUM_HOOK_MUTATE.
 * data MUST be a live host_alloc block owned by the calling module with cookie size == size. */
typedef struct AeternumPacketMutation {
    uint32_t       struct_size;
    uint8_t*       data;   /* OUT: HostAlloc user pointer (ownership stays with the module until host claims) */
    uint32_t       size;   /* OUT: payload bytes; MUST equal the HostAlloc cookie size */
} AeternumPacketMutation;

typedef struct AeternumPacketView {
    uint32_t              struct_size;
    uint32_t              direction;  /* AETERNUM_PACKET_DIR_* */
    uint32_t              opcode;     /* logical opcode */
    uint32_t              size;       /* payload bytes at data */
    uint8_t const*        data;       /* read-only payload, NOT owned, valid only during the call */
    AeternumSessionHandle session;    /* opaque; may be NULL */
    /* ABI v1 growth (IE-M5 Blocco1, struct_size): host-owned mutation slot. Non-NULL only for SMSG when the
     * host struct_size includes this field. Modules MUST test AETERNUM_FT_HAS-style size before writing. */
    AeternumPacketMutation* mutation;
    /* ABI v1 append (M6): process-unique WorldSession lifetime ID, never reused in this process.
     * Guard with struct_size before reading. UINT32_MAX = unavailable (e.g. outbound without session).
     * IDs are NOT account IDs, NOT bounded to a module table; modules MUST bounds-check. */
    uint32_t session_id;
} AeternumPacketView;

typedef uint32_t (AETERNUM_ABI_CALL *AeternumPacketHookFn)(void* mod_ctx, AeternumPacketView const* view);

/* M7: core gameplay intercepts. All input fields are read-only for the duration of the call.
 * ObjectGuid is 128 bits in this core: id = raw low word, id_high = raw high word (NOT a counter).
 * No opaque Unit/Spell pointer or module heap crosses this boundary. Guard fields with struct_size. */
typedef struct AeternumDamageMutation {
    uint32_t struct_size;
    uint32_t override_damage; /* OUT, host stack; consumed ONLY on AETERNUM_OVERRIDE_DAMAGE */
} AeternumDamageMutation;

typedef struct AeternumCombatView {
    uint32_t struct_size;
    uint64_t attacker_id;
    uint64_t attacker_id_high;
    uint64_t victim_id;
    uint64_t victim_id_high;
    uint32_t spell_id;    /* 0 for non-spell damage */
    uint32_t base_damage; /* entry value before native scaling/scripts; identical for every subscriber */
    AeternumDamageMutation* mutation; /* host-owned, valid ONLY during this callback */
} AeternumCombatView;

typedef struct AeternumSpellView {
    uint32_t struct_size;
    uint64_t attacker_id;
    uint64_t attacker_id_high;
    uint64_t victim_id;
    uint64_t victim_id_high;
    uint32_t spell_id;
    uint32_t base_damage; /* 0 at prepare: final hit damage has NOT been calculated yet */
} AeternumSpellView;

typedef uint32_t (AETERNUM_ABI_CALL *AeternumCombatHookFn)(void* mod_ctx, AeternumCombatView const* view);
typedef uint32_t (AETERNUM_ABI_CALL *AeternumSpellHookFn)(void* mod_ctx, AeternumSpellView const* view);

/* M9: boot-only static RAM writes. No raw cache pointers cross the C boundary.
 * Creature HP is rounded, unranked Classic health at reference level 1 (not a live unit HP).
 * Changing it changes the per-entry Classic multiplier consumed at every spawn/level.
 * Weapon override is absolute min/max damage per hit at ALL item levels, not a bonus stat.
 * The only supported item-stat selector is attack delay in milliseconds, NOT a DB2 stat index. */
enum { AETERNUM_ITEM_STAT_DELAY_MS = 1 };
#define AETERNUM_DATA_MAX_HP 1000000u
#define AETERNUM_DATA_MAX_WEAPON_DAMAGE 1000000.0f
typedef struct AeternumCreatureHPView {
    uint32_t struct_size;
    uint32_t reference_level;
    uint32_t base_hp;
} AeternumCreatureHPView;
typedef struct AeternumWeaponDamageView {
    uint32_t struct_size;
    float min_damage;
    float max_damage;
} AeternumWeaponDamageView;
/* Fortification: default-difficulty SpellInfo recovery field (category remains
 * independent), and unique UNGROUPED direct-item creature-loot chance only.
 * Same OnDataLoaded/thread window as M9; no mid-combat mutation or raw pointers. */
#define AETERNUM_DATA_MAX_SPELL_RECOVERY_MS 86400000u
typedef struct AeternumSpellRecoveryView {
    uint32_t struct_size;
    uint32_t recovery_ms;
    uint32_t category_recovery_ms;
} AeternumSpellRecoveryView;
typedef struct AeternumLootChanceView {
    uint32_t struct_size;
    float chance; /* finite percent, 0..100; grouped/reference/currency excluded */
} AeternumLootChanceView;
typedef AeternumStatus (AETERNUM_ABI_CALL *AeternumDataLoadedFn)(void* mod_ctx);

/* M11: contiguous HostAlloc envelope. Payload starts at struct_size; current header must be exactly 16.
 * No session/core/socket pointer crosses this interface. opcode is LOGICAL, not client wire. */
typedef struct AeternumOwnedPacket {
    uint32_t struct_size;
    uint32_t opcode;
    uint32_t payload_size;
    uint32_t flags; /* reserved, must be zero */
} AeternumOwnedPacket;

/* M14: values only; original GUID is 128 bits, never a truncated counter.
 * Entity handles are kernel-lifetime + slot-generation tagged, not pointers.
 * Register/read/unregister require a current Active module crossing and an
 * explicitly ECS-owned map/instance. BUSY during L/S; no mutable array export. */
typedef uint64_t AeternumEcsEntityHandle;
typedef struct AeternumGuid128 { uint64_t low, high; } AeternumGuid128;
typedef struct AeternumEcsEntityState {
    uint32_t struct_size;
    AeternumGuid128 guid;
    float x, y, z, o;
    uint32_t hp, max_hp;
    uint64_t aura_mask;
} AeternumEcsEntityState;
typedef struct AeternumHostFT {
    uint32_t struct_size;
    void (AETERNUM_ABI_CALL *log_info)(AeternumStringView msg);
    AeternumStatus (AETERNUM_ABI_CALL *db_async_query)(
        AeternumStringView sql,
        void (AETERNUM_ABI_CALL *cb)(void* user, AeternumStatus status, void const* rows, uint32_t nbytes),
        void* user);
    /*
     * ABI v1 growth (IE-M3 Blocco1, maintainer decision Q1): fields are only ever APPENDED below and are
     * guarded by struct_size. AETERNUM_MODULE_ABI_VERSION stays 1. A module MUST test
     * AETERNUM_FT_HAS(host, AeternumHostFT, <field>) before calling an appended field.
     */
    void (AETERNUM_ABI_CALL *log_error)(AeternumStringView msg);
    /* 1 if any native module faulted in this process (contained fault => process "tainted"), else 0. */
    uint32_t (AETERNUM_ABI_CALL *runtime_tainted)(void);
    /*
     * ABI v1 growth (IE-M3 Blocco2). Every call below is valid ONLY inside a host -> module crossing
     * (entry, lifecycle slot, packet hook) on the thread the host called the module on; outside one it fails
     * closed (NULL / AETERNUM_ERR_DENIED). `module` MUST be the value returned by module_self().
     */
    /* Opaque handle of the module currently being called by the host (NULL outside a crossing). */
    AeternumModuleHandle (AETERNUM_ABI_CALL *module_self)(void);
    /* Policy F2: host-owned heap with per-block cookie (module id + generation + size). Capped; NULL on refusal. */
    void* (AETERNUM_ABI_CALL *host_alloc)(AeternumModuleHandle module, uint64_t size);
    /* Frees ONLY a live host_alloc block owned by `module`. Orphan / foreign / stale / double free => not freed,
     * logged, module quarantined (Error), AETERNUM_ERR_DENIED returned. NULL => AETERNUM_OK (no-op). */
    AeternumStatus (AETERNUM_ABI_CALL *host_free)(AeternumModuleHandle module, void* ptr);
    /* Packet hook subscription: ONLY during on_init. AETERNUM_ERR_DENIED after on_init (table frozen), for
     * socket-level opcodes (movement subscriptions permitted since M6); AETERNUM_ERR_NOT_FOUND for an opcode with no dense slot;
     * AETERNUM_ERR_OVERFLOW when the opcode already has the maximum number of hooks. */
    AeternumStatus (AETERNUM_ABI_CALL *subscribe_packet)(AeternumModuleHandle module, uint32_t direction,
                                                         uint32_t opcode, AeternumPacketHookFn fn);
    /* M7 append: same on_init-only, image validation, publish/freeze and bounded table contract.
     * Damage: PASS ignores OUT; last successful OVERRIDE wins, every input retains original base_damage.
     * Spell: first CANCEL stops dispatch and native preparation (normal failure cleanup still runs).
     * Fault: quarantine + taint/60s shutdown, faulting OUT ignored; native default continues. */
    AeternumStatus (AETERNUM_ABI_CALL *subscribe_combat)(AeternumModuleHandle module, AeternumCombatHookFn fn);
    AeternumStatus (AETERNUM_ABI_CALL *subscribe_spell)(AeternumModuleHandle module, AeternumSpellHookFn fn);
    /* M9 append: subscribe only in on_init; getters/setters ONLY inside the single post-load callback
     * on the boot thread. Outside => DENIED; missing entry => NOT_FOUND; non-Classic HP/non-weapon =>
     * UNSUPPORTED; invalid value => INVALID_ARGUMENT/OVERFLOW. No DB/file writes, no module allocation. */
    AeternumStatus (AETERNUM_ABI_CALL *subscribe_data_loaded)(AeternumModuleHandle module, AeternumDataLoadedFn fn);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_get_creature_base_hp)(uint32_t entry, AeternumCreatureHPView* out);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_set_creature_base_hp)(uint32_t entry, uint32_t new_hp);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_get_item_stat)(uint32_t item_id, uint32_t stat_type, int32_t* out);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_set_item_stat)(uint32_t item_id, uint32_t stat_type, int32_t value);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_get_weapon_damage)(uint32_t item_id, AeternumWeaponDamageView* out);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_set_weapon_damage)(uint32_t item_id, float min_damage, float max_damage);
    /* M11 append: only synchronous inbound hook context for this exact lifetime session_id.
     * Send is Active-only. Valid owned envelopes in Booting/Active/Stopping crossings are consumed,
     * including malformed payload, stale session, closed socket and rate refusal. Orphan/foreign/
     * corrupt blocks, quarantined states or calls outside a crossing are NOT consumed. Never reuse after claim.
     * Allowlist: SMSG_START_TIMER, 21-byte native countdown, no text/GUID, 1..60 seconds.
     * Bypasses gameplay/session/outbound hooks, NOT mandatory socket MPSC/encryption/async queues.
     * OK means copied into transport queue, not client delivery. Max one accepted timer per socket. */
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_send_packet)(uint32_t session_id, AeternumOwnedPacket* packet);
    /* M12 append, ABI v1 unchanged: check AETERNUM_FT_HAS before use.
     * JSON snapshot (all module metrics), fresh NUL-terminated HostAlloc owned by `module`.
     * Only a valid current crossing may call. NULL on refusal/allocation failure.
     * Caller MUST host_free(module, result) during a valid crossing; never CRT free.
     * Each successful call owns an independent buffer, stable until host_free/clean unload.
     * No TLS string pointer; dump is not itself profiled. Live atomic samples are approximate.
     * Counts: outermost timed invocation per module/thread (nested send inside hook counts once).
     * Totals include synchronous host calls; VM loading/standalone dumps are not timed.
     * Aggregate across reloads; contained faults count as attempted executions.
     * WASM identity uses canonical module path; native identity uses validated module id.
     * 256 identities/process, max identity1024 bytes; RegistrationDrops reports refusals.
     */
    char* (AETERNUM_ABI_CALL *aeternum_get_telemetry_dump)(AeternumModuleHandle module);
    /* Fortification append: check AETERNUM_FT_HAS; borrowed OUT values only.
     * No allocation/ownership transfer; outputs untouched on rejection. */
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_get_spell_recovery)(uint32_t spell_id, AeternumSpellRecoveryView* out);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_set_spell_recovery)(uint32_t spell_id, uint32_t recovery_ms);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_get_creature_loot_chance)(uint32_t loot_id, uint32_t item_id, AeternumLootChanceView* out);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_set_creature_loot_chance)(uint32_t loot_id, uint32_t item_id, float chance);
    /* M8 append, ABI v1. Host-owned zero-initialized POD, one block/module,
     * schema!=0/version!=0/bytes1..65536. First acquisition during Booting only;
     * schema/version/size must match exactly thereafter (no implicit migration).
     * Valid ONLY within a host crossing; do not host_free/CRT-free, send, or retain
     * beyond module lifetime. Host preserves bytes across clean reload, releases
     * at final clean shutdown. Caller synchronizes concurrent POD mutations.
     * No DLL heap/static/raw function pointers belong in this block. */
    void* (AETERNUM_ABI_CALL *aeternum_persistent_state)(AeternumModuleHandle module,
        uint64_t schema, uint32_t version, uint32_t bytes);
    /* M14 append, ABI v1: outputs untouched on refusal. No pointers retained.
     * GUID lookup average O(1) open hashing, bounded O(capacity) worst case. */
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_ecs_register_entity)(AeternumModuleHandle, uint32_t map_id, uint32_t instance_id, AeternumEcsEntityState const*, AeternumEcsEntityHandle*);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_ecs_unregister_entity)(AeternumModuleHandle, uint32_t map_id, uint32_t instance_id, AeternumEcsEntityHandle);
    AeternumStatus (AETERNUM_ABI_CALL *aeternum_ecs_read_entity)(AeternumModuleHandle, uint32_t map_id, uint32_t instance_id, AeternumEcsEntityHandle, AeternumEcsEntityState*);
} AeternumHostFT;

/* struct_size versioning helpers (C, no STL). FIELD_END = first byte after <field>. */
#define AETERNUM_FT_FIELD_END(type, field) (offsetof(type, field) + sizeof(((type*)0)->field))
#define AETERNUM_FT_HAS(ft_ptr, type, field) \
    ((ft_ptr) != NULL && (size_t)((ft_ptr)->struct_size) >= AETERNUM_FT_FIELD_END(type, field))
/* Smallest struct_size the host accepts for each table (the M1 v1 layout). */
#define AETERNUM_HOST_FT_V1_BASE_SIZE   AETERNUM_FT_FIELD_END(AeternumHostFT, db_async_query)
#define AETERNUM_MODULE_FT_V1_BASE_SIZE AETERNUM_FT_FIELD_END(AeternumModuleFT, metrics_observe)
/* Upper sanity bound for a struct_size reported across the ABI. */
#define AETERNUM_FT_MAX_STRUCT_SIZE     4096u

typedef struct AeternumModuleEntryResult {
    AeternumModuleInfo info;
    AeternumModuleFT   ft;
    void*              mod_ctx;
} AeternumModuleEntryResult;

AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(
    uint32_t host_abi_version,
    AeternumHostFT const* host,
    AeternumModuleEntryResult* out);

static inline int aeternum_abi_compatible(uint32_t module_abi, uint32_t host_abi)
{
    return module_abi == host_abi ? 1 : 0;
}

AETERNUM_EXTERN_C_END

#if defined(__cplusplus)
static_assert(sizeof(AeternumStatus) == 4, "AeternumStatus must be int32");
static_assert(sizeof(AeternumPlayerHandle) == sizeof(void*), "opaque handle must be pointer-sized");
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(AeternumStatus) == 4, "AeternumStatus must be int32");
_Static_assert(sizeof(AeternumPlayerHandle) == sizeof(void*), "opaque handle must be pointer-sized");
#endif

#endif /* AETERNUM_ABI_AETERNUM_ABI_H */