/* M6 local prototype. No core includes, no heap; PacketView session_id integration. */
#ifndef M6_ANTICHEAT_H
#define M6_ANTICHEAT_H
#include <stdint.h>
#include <stddef.h>
#define AC_CAPACITY 65536u
#define AC_MAX_SPEED 100.0 /* yards / second; blatant movement only */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct AcSlot {
    float last_x, last_y, last_z;
    uint32_t last_timestamp;
    uint32_t initialized;
} AcSlot;
typedef struct AcSample { float x, y, z; uint32_t timestamp; } AcSample;
/* Returns ABI verdict: PASS=0, DROP=1. Caller serializes accesses per session. */
uint32_t ac_check(uint64_t session_id, AcSample const* sample);
uint32_t ac_packet(uint64_t session_id, uint8_t const* data, uint32_t size);
int ac_parse(uint8_t const* data, uint32_t size, AcSample* out);
void ac_reset(void); /* boot/test only, never concurrent with callbacks */
int ac_snapshot(uint64_t session_id, AcSlot* out);
#ifdef __cplusplus
}
#endif
#endif
