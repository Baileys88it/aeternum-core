#define AETERNUM_ABI_EXPORTS 1
#include "aeternum/abi/AeternumABI.h"
#include "anticheat.h"
static AeternumHostFT const* g_host;
static int g_context;
static uint32_t const g_opcodes[3]={0x41002Eu,0x410000u,0x410006u};
static uint32_t AETERNUM_ABI_CALL ac_hook(void* ctx, AeternumPacketView const* view) {
    if(ctx!=&g_context || !AETERNUM_FT_HAS(view,AeternumPacketView,session_id))
        return AETERNUM_HOOK_DROP;
    if(view->direction!=AETERNUM_PACKET_DIR_CMSG) return AETERNUM_HOOK_PASS;
    return ac_packet(view->session_id,view->data,view->size);
}
static AeternumStatus AETERNUM_ABI_CALL ac_init(void* ctx) {
    uint32_t i; AeternumStatus first=AETERNUM_OK; (void)ctx;
    if(!AETERNUM_FT_HAS(g_host,AeternumHostFT,subscribe_packet) || !g_host->module_self || !g_host->subscribe_packet)
        return AETERNUM_ERR_UNSUPPORTED;
    ac_reset();
    for(i=0;i<3;++i) {
        AeternumStatus st=g_host->subscribe_packet(g_host->module_self(),AETERNUM_PACKET_DIR_CMSG,g_opcodes[i],ac_hook);
        if(first==AETERNUM_OK && st!=AETERNUM_OK) first=st;
    }
    return first;
}
static AeternumStatus AETERNUM_ABI_CALL ac_lifecycle(void* ctx) { (void)ctx; return AETERNUM_OK; }
AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(uint32_t abi,AeternumHostFT const* host,AeternumModuleEntryResult* out) {
    if(!host || !out || host->struct_size<AETERNUM_HOST_FT_V1_BASE_SIZE) return AETERNUM_ERR_INVALID_ARGUMENT;
    if(!aeternum_abi_compatible(AETERNUM_MODULE_ABI_VERSION,abi)) return AETERNUM_ERR_ABI_MISMATCH;
    g_host=host;
    out->info.id="mod_anticheat"; out->info.version="0.2.0"; out->info.api_range="1";
    out->info.abi_version=1; out->info.load_policy=AETERNUM_LOAD_POLICY_ON_BOOT_ONLY; out->info.critical=0;
    out->ft.struct_size=(uint32_t)sizeof(AeternumModuleFT);
    out->ft.on_load=ac_lifecycle; out->ft.on_init=ac_init; out->ft.on_start=ac_lifecycle;
    out->ft.on_stop=ac_lifecycle; out->ft.on_shutdown=ac_lifecycle; out->ft.metrics_observe=0;
    out->mod_ctx=&g_context; return AETERNUM_OK;
}
