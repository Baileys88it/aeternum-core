#include "anticheat.h"
#include "aeternum/abi/AeternumABI.h"
/* Static BSS only. No allocator, hashing, modulo, pointer-as-ID or dynamic container. */
static AcSlot g_slots[AC_CAPACITY];
static uint32_t ac_u32(uint8_t const* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static float ac_float(uint8_t const* p) {
    union { uint32_t u; float f; } v; v.u=ac_u32(p); return v.f;
}
static int ac_finite(float f) {
    union { uint32_t u; float f; } v; v.f=f; return (v.u & 0x7f800000u)!=0x7f800000u;
}
void ac_reset(void) {
    uint32_t i; for(i=0;i<AC_CAPACITY;++i) g_slots[i].initialized=0;
}
int ac_snapshot(uint64_t id, AcSlot* out) {
    if(id>=AC_CAPACITY || !out) return 0;
    *out=g_slots[(uint32_t)id]; return 1;
}
uint32_t ac_check(uint64_t id, AcSample const* p) {
    AcSlot* s;
    if(id>=AC_CAPACITY || !p || !ac_finite(p->x) || !ac_finite(p->y) || !ac_finite(p->z))
        return AETERNUM_HOOK_DROP;
    s=&g_slots[(uint32_t)id];
    if(s->initialized) {
        double dx,dy,dz,dt,limit;
        /* Deliberately fail closed also on uint32 timestamp rollover. */
        if(p->timestamp<=s->last_timestamp) return AETERNUM_HOOK_DROP;
        dx=(double)p->x-s->last_x; dy=(double)p->y-s->last_y; dz=(double)p->z-s->last_z;
        dt=((double)p->timestamp-s->last_timestamp)/1000.0;
        limit=AC_MAX_SPEED*dt;
        /* Equivalent to distance/dt > MAX_SPEED, without sqrt/CRT imports. */
        if(dx*dx+dy*dy+dz*dz>limit*limit) return AETERNUM_HOOK_DROP;
    }
    s->last_x=p->x; s->last_y=p->y; s->last_z=p->z;
    s->last_timestamp=p->timestamp; s->initialized=1;
    return AETERNUM_HOOK_PASS;
}
int ac_parse(uint8_t const* p, uint32_t n, AcSample* out) {
    uint32_t mask,i,offset=2;
    if(!p || !out || n<2) return 0;
    mask=(uint32_t)p[0] | ((uint32_t)p[1]<<8);
    for(i=0;i<16;++i) offset+=(mask>>i)&1u;
    /* ObjectGuid.cpp: uint16 mask + popcount(mask) bytes.
       MovementPackets.cpp operator>>(MovementInfo): uint64 flags, uint32 time,
       PositionXYZO, pitch, stepUp, forceCount, moveIndex, gravity, optional bits.
       Only the verified fixed prefix is consumed; legacy handler validates the tail. */
    if(offset>n || n-offset<48u) return 0;
    out->timestamp=ac_u32(p+offset+8);
    out->x=ac_float(p+offset+12); out->y=ac_float(p+offset+16); out->z=ac_float(p+offset+20);
    return ac_finite(out->x) && ac_finite(out->y) && ac_finite(out->z);
}
uint32_t ac_packet(uint64_t id, uint8_t const* data, uint32_t size) {
    AcSample s;
    if(id>=AC_CAPACITY || !ac_parse(data,size,&s)) return AETERNUM_HOOK_DROP;
    return ac_check(id,&s);
}
