/* M11: native countdown only, no text field; /played request is passed unchanged. */
#include "aeternum/abi/AeternumABI.h"
#include <stdio.h>
static AeternumHostFT const* host;
static void le64(uint8_t* p,uint64_t n){unsigned i;for(i=0;i<8;++i)p[i]=(uint8_t)(n>>(8u*i));}
static uint32_t AETERNUM_ABI_CALL trigger(void* ctx,AeternumPacketView const* view){
 AeternumOwnedPacket* packet;uint8_t* data;uint32_t wire;AeternumStatus status;(void)ctx;
 /* ReadDataHandler reads (but retains) the 4-byte client wire opcode in WorldPacket storage.
  * The hook gets data()/size(), not data()+rpos(): exactly wire4 + played flag1.
  * The core reads one bit and ignores the other seven padding bits. No chat string/NUL exists. */
 if(!AETERNUM_FT_HAS(view,AeternumPacketView,session_id)||view->session_id==UINT32_MAX||view->direction!=AETERNUM_PACKET_DIR_CMSG||view->opcode!=0x3D014Cu||view->size!=5||!view->data)return AETERNUM_HOOK_PASS;
 wire=(uint32_t)view->data[0]|((uint32_t)view->data[1]<<8)|((uint32_t)view->data[2]<<16)|((uint32_t)view->data[3]<<24);
 /* Both aliases are SHA-verified in the pinned 1.60.1.70790 profile. */
 if(wire!=0x3E014Cu&&wire!=0x3E014Eu)return AETERNUM_HOOK_PASS;
 packet=(AeternumOwnedPacket*)host->host_alloc(host->module_self(),sizeof(AeternumOwnedPacket)+21u);
 if(!packet)return AETERNUM_HOOK_PASS;
 packet->struct_size=(uint32_t)sizeof(*packet);packet->opcode=0x45003Eu;packet->payload_size=21;packet->flags=0;
 data=(uint8_t*)(packet+1);le64(data,30);data[8]=0;data[9]=0;data[10]=0;data[11]=0;le64(data+12,30);data[20]=0;
 /* Valid owned envelope is always consumed, including one-shot rate refusal. */
 {
  static char const text[]="TRIGGER: /played intercettato, iniezione timer in corso";
  AeternumStringView message={text,(uint32_t)(sizeof(text)-1u)};
  host->log_info(message);
  status=host->aeternum_send_packet(view->session_id,packet);
 }
 {
  char text[64];int n=snprintf(text,sizeof(text),"TIMER_SEND: status=%u",(unsigned)status);
  if(n>0&&(unsigned)n<sizeof(text)){AeternumStringView message={text,(uint32_t)n};host->log_info(message);}
 }
 return AETERNUM_HOOK_PASS;
}
static AeternumStatus AETERNUM_ABI_CALL init(void* ctx){(void)ctx;return host->subscribe_packet(host->module_self(),AETERNUM_PACKET_DIR_CMSG,0x3D014Cu,trigger);}
AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(uint32_t abi,AeternumHostFT const* h,AeternumModuleEntryResult* out){
 if(!h||!out)return AETERNUM_ERR_INVALID_ARGUMENT;
 if(!aeternum_abi_compatible(AETERNUM_MODULE_ABI_VERSION,abi))return AETERNUM_ERR_ABI_MISMATCH;
 if(!AETERNUM_FT_HAS(h,AeternumHostFT,aeternum_send_packet)||!h->aeternum_send_packet||!h->module_self||!h->host_alloc||!h->subscribe_packet||!h->log_info)return AETERNUM_ERR_UNSUPPORTED;
 host=h;out->info.id="mod_holographic_ui";out->info.version="0.1.1";out->info.api_range="1";out->info.abi_version=1;out->info.load_policy=AETERNUM_LOAD_POLICY_ON_BOOT_ONLY;out->info.critical=0;
 out->ft.struct_size=(uint32_t)sizeof(out->ft);out->ft.on_load=0;out->ft.on_init=init;out->ft.on_start=0;out->ft.on_stop=0;out->ft.on_shutdown=0;out->ft.metrics_observe=0;out->mod_ctx=0;
 return AETERNUM_OK;
}
