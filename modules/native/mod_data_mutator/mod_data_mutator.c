/* M9: NPC 1984 has a checked-in creature_classic_stats row.
 * Live build is HP-only. Weapon 9000002 is enabled ONLY in the separately compiled test fixture image.
 * HP semantics: triple rounded unranked Classic level-1 reference HP (not current unit health).
 * Missing/unsupported targets are logged and skipped; invalid host/ABI fails closed. No heap. */
#include "aeternum/abi/AeternumABI.h"
static AeternumHostFT const* host;
static AeternumStatus AETERNUM_ABI_CALL data_loaded(void* ctx)
{
    AeternumCreatureHPView hp={sizeof(hp),0,0};
    AeternumStatus status;
    (void)ctx;
    status=host->aeternum_get_creature_base_hp(1984,&hp);
    if (status==AETERNUM_OK) {
        if (hp.base_hp>AETERNUM_DATA_MAX_HP/3u) return AETERNUM_ERR_OVERFLOW;
        status=host->aeternum_set_creature_base_hp(1984,hp.base_hp*3u);
        if (status!=AETERNUM_OK) return status;
    } else if (status!=AETERNUM_ERR_NOT_FOUND && status!=AETERNUM_ERR_UNSUPPORTED) return status;
    else { static char const message[]="[mod_data_mutator] NPC 1984 absent/non-Classic; skipped"; AeternumStringView msg={message,sizeof(message)-1u}; host->log_info(msg); }
#if defined(AETERNUM_DATA_MUTATOR_FIXTURE_WEAPON)
    /* Test-only image: never enable this flag in the deployed HP-only module. */
    status=host->aeternum_set_weapon_damage(9000002,20.0f,40.0f);
    if (status==AETERNUM_ERR_NOT_FOUND || status==AETERNUM_ERR_UNSUPPORTED) {
        static char const message[]="[mod_data_mutator] weapon 9000002 fixture absent; skipped";
        AeternumStringView msg={message,sizeof(message)-1u}; host->log_info(msg); return AETERNUM_OK;
    }
    return status;
#else
    return AETERNUM_OK; /* Live QA is HP-only, even if fixture id 9000002 unexpectedly exists. */
#endif
}
static AeternumStatus AETERNUM_ABI_CALL init(void* ctx)
{ (void)ctx; return host->subscribe_data_loaded(host->module_self(),data_loaded); }
static AeternumStatus AETERNUM_ABI_CALL ok(void* ctx) { (void)ctx; return AETERNUM_OK; }
AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(
    uint32_t abi,AeternumHostFT const* h,AeternumModuleEntryResult* out)
{
    if (abi!=AETERNUM_MODULE_ABI_VERSION) return AETERNUM_ERR_ABI_MISMATCH;
    if (!out || !AETERNUM_FT_HAS(h,AeternumHostFT,aeternum_set_weapon_damage)
        || !h->module_self || !h->subscribe_data_loaded || !h->aeternum_get_creature_base_hp
        || !h->aeternum_set_creature_base_hp || !h->aeternum_set_weapon_damage || !h->log_info)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    host=h;
    out->info.id="mod_data_mutator"; out->info.version="0.2.0"; out->info.api_range="^1.0.0";
    out->info.abi_version=1; out->info.load_policy=AETERNUM_LOAD_POLICY_ON_BOOT_ONLY; out->info.critical=0;
    out->ft.struct_size=sizeof(out->ft); out->ft.on_load=ok; out->ft.on_init=init;
    out->ft.on_start=ok; out->ft.on_stop=ok; out->ft.on_shutdown=ok; out->ft.metrics_observe=0;
    out->mod_ctx=0; return AETERNUM_OK;
}
