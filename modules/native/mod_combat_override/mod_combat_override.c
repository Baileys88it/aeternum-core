/* Aeternum M7 QA: stack-buffer override, event-only telemetry, no module heap or PASS logging. */
#include "aeternum/abi/AeternumABI.h"
static AeternumHostFT const* g_host;
/* Fixed stack formatting: no printf/locale allocation and no persistent event state. */
static char* text(char* out, char const* in)
{
    while (*in) *out++ = *in++;
    return out;
}
static char* number(char* out, uint32_t value)
{
    char digits[10]; unsigned n = 0;
    do { digits[n++] = (char)('0' + value % 10u); value /= 10u; } while (value);
    while (n) *out++ = digits[--n];
    return out;
}
static char* guid_word(char* out, uint64_t value)
{
    static char const hex[] = "0123456789ABCDEF";
    unsigned n;
    for (n = 0; n < 16u; ++n) *out++ = hex[(value >> (60u - n * 4u)) & 15u];
    return out;
}
static void log_override(AeternumCombatView const* view)
{
    char buffer[320]; char* out = buffer; AeternumStringView message;
    if (!g_host->log_info) return;
    out = text(out, "[mod_combat_override] combat verdict=OVERRIDE_DAMAGE raw=");
    out = number(out, view->base_damage);
    out = text(out, " override="); out = number(out, view->mutation->override_damage);
    out = text(out, " spell_id="); out = number(out, view->spell_id);
    out = text(out, " attacker=0x"); out = guid_word(out, view->attacker_id_high); out = guid_word(out, view->attacker_id);
    out = text(out, " victim=0x"); out = guid_word(out, view->victim_id_high); out = guid_word(out, view->victim_id);
    *out = 0; message.data = buffer; message.len = (uint32_t)(out - buffer);
    g_host->log_info(message);
}
static uint32_t AETERNUM_ABI_CALL damage_hook(void* ctx, AeternumCombatView const* view)
{
    (void)ctx;
    if (!AETERNUM_FT_HAS(view, AeternumCombatView, mutation) ||
        !AETERNUM_FT_HAS(view->mutation, AeternumDamageMutation, override_damage))
        return AETERNUM_HOOK_PASS;
    if (view->base_damage > 5000u)
    {
        view->mutation->override_damage = view->base_damage / 2u;
        log_override(view);
        return AETERNUM_OVERRIDE_DAMAGE;
    }
    return AETERNUM_HOOK_PASS;
}
/* QA selected spell: Healing Touch 5185, observed in Roky's active known-spell list and action slot 2.
 * Evidence: isolated PKT login Player-70-0000000A, 2026-11-06 22:06:18 Europe/Rome. */
#define QA_CANCEL_SPELL_ID 5185u
static uint32_t AETERNUM_ABI_CALL spell_hook(void* ctx, AeternumSpellView const* view)
{
    (void)ctx;
    if (!AETERNUM_FT_HAS(view, AeternumSpellView, base_damage)) return AETERNUM_HOOK_PASS;
    if (view->spell_id == QA_CANCEL_SPELL_ID)
    {
        if (g_host->log_info)
        {
            char buffer[320]; char* out = buffer; AeternumStringView message;
            out = text(out, "[mod_combat_override] spell verdict=CANCEL_SPELL spell_id=");
            out = number(out, view->spell_id);
            out = text(out, " attacker=0x"); out = guid_word(out, view->attacker_id_high); out = guid_word(out, view->attacker_id);
            out = text(out, " victim=0x"); out = guid_word(out, view->victim_id_high); out = guid_word(out, view->victim_id);
            *out = 0; message.data = buffer; message.len = (uint32_t)(out - buffer);
            g_host->log_info(message);
        }
        return AETERNUM_CANCEL_SPELL;
    }
    return AETERNUM_HOOK_PASS;
}
static AeternumStatus AETERNUM_ABI_CALL init(void* ctx)
{
    (void)ctx;
    AeternumStatus status = g_host->subscribe_combat(g_host->module_self(), damage_hook);
    if (status != AETERNUM_OK) return status;
    return g_host->subscribe_spell(g_host->module_self(), spell_hook);
}
AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(
    uint32_t version, AeternumHostFT const* host, AeternumModuleEntryResult* out)
{
    if (!out || !host) return AETERNUM_ERR_INVALID_ARGUMENT;
    if (!aeternum_abi_compatible(AETERNUM_MODULE_ABI_VERSION, version)) return AETERNUM_ERR_ABI_MISMATCH;
    if (!AETERNUM_FT_HAS(host, AeternumHostFT, subscribe_spell) || !host->subscribe_combat || !host->subscribe_spell || !host->module_self)
        return AETERNUM_ERR_UNSUPPORTED;
    g_host = host;
    out->info.id = "mod_combat_override";
    out->info.version = "0.2.0";
    out->info.api_range = "^1.0.0";
    out->info.abi_version = AETERNUM_MODULE_ABI_VERSION;
    out->info.load_policy = AETERNUM_LOAD_POLICY_ON_BOOT_ONLY;
    out->info.critical = 0;
    out->ft.struct_size = sizeof(AeternumModuleFT);
    out->ft.on_load = 0; out->ft.on_init = init; out->ft.on_start = 0;
    out->ft.on_stop = 0; out->ft.on_shutdown = 0; out->ft.metrics_observe = 0;
    out->mod_ctx = 0;
    return AETERNUM_OK;
}
