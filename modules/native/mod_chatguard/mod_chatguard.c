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
 * Aeternum IE-M5 - mod_chatguard: native gameplay module (.dll / .so), C ABI v1 only.
 * Depends ONLY on include/aeternum/abi/AeternumABI.h (no core link, no STL, no exceptions, no CRT heap).
 * on_init subscribes (then the hook table is frozen):
 *  - CMSG_CHAT_MESSAGE_SAY (0x2C0023, logical /say): payload = int32 Language | Bits<11> TextLen | Bits<1> IsSecure
 *    | flush | Text[TextLen]. Text containing "DropMe" => AETERNUM_HOOK_DROP (never reaches the legacy handler).
 *    Any other text => F2 temp HostAlloc/HostFree inspect, AETERNUM_HOOK_PASS.
 *  - SMSG_CHAT (0x4A0001, server chat echo, legacy name SMSG_MESSAGECHAT): rudimentary parse of ChatPkt::Chat
 *    (uint8 SlashCmd, uint32 Language, 4 packed GUID128, 6 x 4-byte fields, Bits 11/11/5/7/12 + 5 flag bits, flush,
 *    strings). For SlashCmd == CHAT_MSG_SAY the ChatText is rewritten to "[M5] " + text in a HostAlloc block
 *    (12-bit length field updated), returned via view->mutation with AETERNUM_HOOK_MUTATE. Ownership of the block
 *    passes to the host (HostClaimMutationBlock / HostDisposeClaimed); the module never frees it. Any parse /
 *    bounds / slot problem => no mutation, AETERNUM_HOOK_PASS (never a malformed MUTATE).
 */
#ifndef AETERNUM_ABI_EXPORTS
#  define AETERNUM_ABI_EXPORTS 1
#endif
#include "aeternum/abi/AeternumABI.h"

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance;
    (void)reason;
    (void)reserved;
    return TRUE; /* trivial by contract (F1) */
}
#endif

#define CHATGUARD_ID "mod_chatguard"
/* Logical core opcodes (src/server/game/Server/Protocol/Opcodes.h), i.e. after M2 TranslateInbound. */
#define CHATGUARD_CMSG_CHAT_MESSAGE_SAY 0x2C0023u
#define CHATGUARD_SMSG_CHAT             0x4A0001u
#define CHATGUARD_CHAT_MSG_SAY          0x01u
#define CHATGUARD_TMP_MAX               4096u
#define CHATGUARD_TEXT_MAX_BITS12       4095u
#define CHATGUARD_SMSG_MAX              65536u

static char const g_dropToken[] = "DropMe";
static char const g_prefix[] = "[M5] ";
#define CHATGUARD_DROP_LEN   6u
/* Classic 1.60 inbound frame: 10 bytes precede Text (8 bytes, then Bits<11> TextLen | Bits<1> IsSecure | flush). */
#define CHATGUARD_CMSG_HDR   10u
#define CHATGUARD_PREFIX_LEN 5u

static AeternumHostFT const* g_host = NULL;
static int g_ctx = 0x4D35;

static uint32_t cg_strlen(char const* s)
{
    uint32_t n = 0u;
    while (s && s[n] != '\0' && n < 500u)
        ++n;
    return n;
}

static void cg_log(char const* text)
{
    AeternumStringView v;
    if (!g_host || !g_host->log_info || !text)
        return;
    v.data = text;
    v.len = cg_strlen(text);
    g_host->log_info(v);
}

/* Tiny bounded line builder (no CRT printf). */
typedef struct CgLine { char buf[512]; uint32_t n; } CgLine;

static void cg_init(CgLine* l)
{
    l->n = 0u;
    l->buf[0] = '\0';
}

static void cg_put(CgLine* l, char const* s)
{
    while (s && *s && l->n + 1u < sizeof(l->buf))
        l->buf[l->n++] = *s++;
    l->buf[l->n] = '\0';
}

/* Printable ASCII only, bounded (chat text is player input). */
static void cg_put_text(CgLine* l, unsigned char const* s, uint32_t len, uint32_t maxChars)
{
    uint32_t i;
    for (i = 0u; i < len && i < maxChars && l->n + 1u < sizeof(l->buf); ++i)
        l->buf[l->n++] = (s[i] >= 0x20u && s[i] < 0x7Fu) ? (char)s[i] : '?';
    if (len > maxChars)
        cg_put(l, "...");
    l->buf[l->n] = '\0';
}

static void cg_dec(CgLine* l, long value)
{
    char d[24];
    uint32_t k = 0u;
    unsigned long v = value < 0 ? (unsigned long)(-value) : (unsigned long)value;
    if (value < 0)
        cg_put(l, "-");
    do { d[k++] = (char)('0' + (v % 10u)); v /= 10u; } while (v != 0u && k < sizeof(d));
    while (k > 0u && l->n + 1u < sizeof(l->buf))
        l->buf[l->n++] = d[--k];
    l->buf[l->n] = '\0';
}

static int cg_has_hooks(void)
{
    return g_host && AETERNUM_FT_HAS(g_host, AeternumHostFT, subscribe_packet) && g_host->module_self
        && g_host->host_alloc && g_host->host_free && g_host->subscribe_packet;
}

/* MSB-first bit reader/writer matching ByteBuffer::ReadBit/WriteBit. */
static uint32_t cg_get_bits(unsigned char const* base, uint32_t bitPos, uint32_t count)
{
    uint32_t v = 0u, i;
    for (i = 0u; i < count; ++i)
    {
        uint32_t const b = bitPos + i;
        v = (v << 1) | (uint32_t)((base[b >> 3] >> (7u - (b & 7u))) & 1u);
    }
    return v;
}

static void cg_set_bits(unsigned char* base, uint32_t bitPos, uint32_t count, uint32_t value)
{
    uint32_t i;
    for (i = 0u; i < count; ++i)
    {
        uint32_t const b = bitPos + i;
        unsigned char const mask = (unsigned char)(1u << (7u - (b & 7u)));
        if ((value >> (count - 1u - i)) & 1u)
            base[b >> 3] = (unsigned char)(base[b >> 3] | mask);
        else
            base[b >> 3] = (unsigned char)(base[b >> 3] & (unsigned char)~mask);
    }
}

static int cg_contains(unsigned char const* s, uint32_t len, char const* tok, uint32_t tokLen)
{
    uint32_t i, j;
    if (tokLen == 0u || len < tokLen)
        return 0;
    for (i = 0u; i + tokLen <= len; ++i)
    {
        for (j = 0u; j < tokLen; ++j)
            if (s[i + j] != (unsigned char)tok[j])
                break;
        if (j == tokLen)
            return 1;
    }
    return 0;
}

static uint32_t cg_popcount16(uint32_t m)
{
    uint32_t c = 0u;
    m &= 0xFFFFu;
    while (m) { c += m & 1u; m >>= 1; }
    return c;
}

/* ---------------------------------------------------------------- CMSG_CHAT_MESSAGE_SAY (inbound) */
static uint32_t AETERNUM_ABI_CALL cg_chat_hook(void* ctx, AeternumPacketView const* view)
{
    AeternumModuleHandle self;
    uint32_t tmpSize, i, textLen = 0u;
    int parsed = 0;
    unsigned char* tmp;
    AeternumStatus freed = AETERNUM_OK;
    CgLine line;
    cg_init(&line);
    if (ctx != &g_ctx || !view || view->struct_size < AETERNUM_FT_FIELD_END(AeternumPacketView, session)
        || !cg_has_hooks())
        return AETERNUM_HOOK_PASS;

    if (view->direction == AETERNUM_PACKET_DIR_CMSG && view->data && view->size >= CHATGUARD_CMSG_HDR)
    {
        textLen = cg_get_bits(view->data + (CHATGUARD_CMSG_HDR - 2u), 0u, 11u);
        parsed = (CHATGUARD_CMSG_HDR + textLen == view->size) ? 1 : 0;
        if (!parsed && view->size > CHATGUARD_CMSG_HDR) /* Text = remainder after the 10-byte header */
        {
            textLen = view->size - CHATGUARD_CMSG_HDR;
            parsed = 1;
        }
    }

    if (parsed && cg_contains(view->data + CHATGUARD_CMSG_HDR, textLen, g_dropToken, CHATGUARD_DROP_LEN))
    {
        cg_put(&line, "[mod_chatguard] CMSG_CHAT_MESSAGE_SAY opcode=0x2C0023 dir=inbound size=");
        cg_dec(&line, (long)view->size);
        cg_put(&line, " bytes verdict=DROP reason=DropMe text_len=");
        cg_dec(&line, (long)textLen);
        cg_log(line.buf);
        return AETERNUM_HOOK_DROP;
    }

    self = g_host->module_self();
    tmpSize = view->size == 0u ? 1u : (view->size > CHATGUARD_TMP_MAX ? CHATGUARD_TMP_MAX : view->size);
    tmp = (unsigned char*)g_host->host_alloc(self, (uint64_t)tmpSize);
    if (tmp)
    {
        for (i = 0u; i < tmpSize; ++i) /* touch the whole block (F2 cookie/tail check on free) */
            tmp[i] = (unsigned char)((view->data && i < view->size) ? view->data[i] : 0u);
        freed = g_host->host_free(self, tmp);
    }

    cg_put(&line, "[mod_chatguard] CMSG_CHAT_MESSAGE_SAY opcode=0x2C0023 dir=");
    cg_put(&line, view->direction == AETERNUM_PACKET_DIR_CMSG ? "inbound" : "outbound");
    cg_put(&line, " size=");
    cg_dec(&line, (long)view->size);
    cg_put(&line, " bytes host_alloc=");
    cg_put(&line, tmp ? "ok(" : "null(");
    cg_dec(&line, (long)tmpSize);
    cg_put(&line, ") host_free=");
    cg_dec(&line, (long)freed);
    cg_put(&line, parsed ? " verdict=PASS" : " verdict=PASS parse=unrecognized");
    cg_log(line.buf);
    return AETERNUM_HOOK_PASS;
}

/* ---------------------------------------------------------------- SMSG_CHAT (outbound) */
static void cg_smsg_log(uint32_t size, char const* tail)
{
    CgLine line;
    cg_init(&line);
    cg_put(&line, "[mod_chatguard] SMSG_CHAT opcode=0x4A0001 dir=outbound size=");
    cg_dec(&line, (long)size);
    cg_put(&line, " bytes ");
    cg_put(&line, tail);
    cg_log(line.buf);
}

static uint32_t AETERNUM_ABI_CALL cg_smsg_chat_hook(void* ctx, AeternumPacketView const* view)
{
    unsigned char const* d;
    uint32_t size, off, g, bitsOff, senderLen, targetLen, prefixLen, chanLen, textLen, textOff, textEnd;
    uint32_t newSize, i, w;
    unsigned char slash;
    unsigned char* out;
    AeternumPacketMutation* mut;
    AeternumModuleHandle self;
    CgLine line;

    if (ctx != &g_ctx || !view || view->struct_size < AETERNUM_FT_FIELD_END(AeternumPacketView, session)
        || !cg_has_hooks() || view->direction != AETERNUM_PACKET_DIR_SMSG || !view->data)
        return AETERNUM_HOOK_PASS;
    d = view->data;
    size = view->size;
    if (size < 5u || size > CHATGUARD_SMSG_MAX)
    {
        cg_smsg_log(size, "verdict=PASS parse=short");
        return AETERNUM_HOOK_PASS;
    }
    slash = d[0];
    off = 5u; /* uint8 SlashCmd + uint32 Language */
    for (g = 0u; g < 4u; ++g) /* SenderGUID, SenderGuildGUID, SenderWowAccount, TargetGUID (packed GUID128) */
    {
        uint32_t mask;
        if (off + 2u > size)
        {
            cg_smsg_log(size, "verdict=PASS parse=guid_oob");
            return AETERNUM_HOOK_PASS;
        }
        mask = (uint32_t)d[off] | ((uint32_t)d[off + 1u] << 8);
        off += 2u + cg_popcount16(mask);
    }
    off += 24u; /* TargetVirtualAddress, SenderVirtualAddress, AchievementID, ChatFlags, DisplayTime, SpellID */
    bitsOff = off;
    off += 7u;  /* 11+11+5+7+12 + 5 flag bits = 51 bits, flushed */
    if (off > size)
    {
        cg_smsg_log(size, "verdict=PASS parse=bits_oob");
        return AETERNUM_HOOK_PASS;
    }
    senderLen = cg_get_bits(d + bitsOff, 0u, 11u);
    targetLen = cg_get_bits(d + bitsOff, 11u, 11u);
    prefixLen = cg_get_bits(d + bitsOff, 22u, 5u);
    chanLen   = cg_get_bits(d + bitsOff, 27u, 7u);
    textLen   = cg_get_bits(d + bitsOff, 34u, 12u);
    textOff = off + senderLen + targetLen + prefixLen + chanLen;
    textEnd = textOff + textLen;
    if (textOff > size || textEnd > size)
    {
        cg_smsg_log(size, "verdict=PASS parse=text_oob");
        return AETERNUM_HOOK_PASS;
    }

    if (slash != CHATGUARD_CHAT_MSG_SAY)
    {
        cg_init(&line);
        cg_put(&line, "verdict=PASS chat_type=");
        cg_dec(&line, (long)slash);
        cg_put(&line, " (not SAY, no mutation)");
        cg_smsg_log(size, line.buf);
        return AETERNUM_HOOK_PASS;
    }
    if (view->struct_size < AETERNUM_FT_FIELD_END(AeternumPacketView, mutation) || !view->mutation
        || view->mutation->struct_size < AETERNUM_FT_FIELD_END(AeternumPacketMutation, size))
    {
        cg_smsg_log(size, "verdict=PASS reason=no_mutation_slot");
        return AETERNUM_HOOK_PASS;
    }
    if (textLen + CHATGUARD_PREFIX_LEN > CHATGUARD_TEXT_MAX_BITS12)
    {
        cg_smsg_log(size, "verdict=PASS reason=text_too_long");
        return AETERNUM_HOOK_PASS;
    }
    if (textLen >= CHATGUARD_PREFIX_LEN && cg_contains(d + textOff, CHATGUARD_PREFIX_LEN, g_prefix, CHATGUARD_PREFIX_LEN))
    {
        cg_smsg_log(size, "verdict=PASS reason=already_prefixed");
        return AETERNUM_HOOK_PASS;
    }

    newSize = size + CHATGUARD_PREFIX_LEN;
    self = g_host->module_self();
    out = (unsigned char*)g_host->host_alloc(self, (uint64_t)newSize);
    if (!out)
    {
        cg_smsg_log(size, "host_alloc=null verdict=PASS reason=alloc_refused");
        return AETERNUM_HOOK_PASS;
    }
    w = 0u;
    for (i = 0u; i < textOff; ++i)
        out[w++] = d[i];
    cg_set_bits(out + bitsOff, 34u, 12u, textLen + CHATGUARD_PREFIX_LEN);
    for (i = 0u; i < CHATGUARD_PREFIX_LEN; ++i)
        out[w++] = (unsigned char)g_prefix[i];
    for (i = textOff; i < size; ++i) /* original text + trailing optionals (BroadcastTextID/ChannelGUID/...) */
        out[w++] = d[i];

    if (w != newSize || cg_get_bits(out + bitsOff, 34u, 12u) != textLen + CHATGUARD_PREFIX_LEN)
    {
        AeternumStatus const fr = g_host->host_free(self, out); /* never hand a malformed block to the host */
        cg_init(&line);
        cg_put(&line, "verdict=PASS reason=self_check_failed host_free=");
        cg_dec(&line, (long)fr);
        cg_smsg_log(size, line.buf);
        return AETERNUM_HOOK_PASS;
    }

    mut = view->mutation;
    mut->data = out;
    mut->size = newSize;

    cg_init(&line);
    cg_put(&line, "host_alloc=ok(");
    cg_dec(&line, (long)newSize);
    cg_put(&line, ") host_free=claimed_by_host verdict=MUTATE new_size=");
    cg_dec(&line, (long)newSize);
    cg_put(&line, " text_len=");
    cg_dec(&line, (long)textLen);
    cg_put(&line, "->");
    cg_dec(&line, (long)(textLen + CHATGUARD_PREFIX_LEN));
    cg_put(&line, " from=");
    cg_put_text(&line, d + textOff, textLen, 64u);
    cg_put(&line, " to=");
    cg_put(&line, g_prefix);
    cg_put_text(&line, d + textOff, textLen, 64u);
    cg_smsg_log(size, line.buf);
    return AETERNUM_HOOK_MUTATE;
}

/* ---------------------------------------------------------------- lifecycle */
static AeternumStatus AETERNUM_ABI_CALL cg_on_load(void* ctx)
{
    (void)ctx;
    cg_log("[mod_chatguard] on_load");
    return AETERNUM_OK;
}

static AeternumStatus AETERNUM_ABI_CALL cg_on_init(void* ctx)
{
    AeternumStatus st, st2;
    CgLine line;
    (void)ctx;
    if (!cg_has_hooks())
    {
        cg_log("[mod_chatguard] on_init: host has no packet hook API");
        return AETERNUM_ERR_UNSUPPORTED;
    }
    st = g_host->subscribe_packet(g_host->module_self(), AETERNUM_PACKET_DIR_CMSG,
                                  CHATGUARD_CMSG_CHAT_MESSAGE_SAY, cg_chat_hook);
    cg_init(&line);
    cg_put(&line, "[mod_chatguard] on_init: subscribe CMSG_CHAT_MESSAGE_SAY opcode=0x2C0023 status=");
    cg_dec(&line, (long)st);
    cg_log(line.buf);
    if (st != AETERNUM_OK)
        return st;

    st2 = g_host->subscribe_packet(g_host->module_self(), AETERNUM_PACKET_DIR_SMSG,
                                   CHATGUARD_SMSG_CHAT, cg_smsg_chat_hook);
    cg_init(&line);
    cg_put(&line, "[mod_chatguard] on_init: subscribe SMSG_CHAT opcode=0x4A0001 status=");
    cg_dec(&line, (long)st2);
    cg_log(line.buf);
    return st2;
}

static AeternumStatus AETERNUM_ABI_CALL cg_on_start(void* ctx)
{
    (void)ctx;
    cg_log("[mod_chatguard] on_start");
    return AETERNUM_OK;
}

static AeternumStatus AETERNUM_ABI_CALL cg_on_stop(void* ctx)
{
    (void)ctx;
    cg_log("[mod_chatguard] on_stop");
    return AETERNUM_OK;
}

static AeternumStatus AETERNUM_ABI_CALL cg_on_shutdown(void* ctx)
{
    (void)ctx;
    cg_log("[mod_chatguard] on_shutdown");
    return AETERNUM_OK;
}

AETERNUM_ABI_EXPORT AeternumStatus AETERNUM_ABI_CALL aeternum_module_entry(
    uint32_t host_abi_version,
    AeternumHostFT const* host,
    AeternumModuleEntryResult* out)
{
    if (!host || !out || host->struct_size < AETERNUM_HOST_FT_V1_BASE_SIZE)
        return AETERNUM_ERR_INVALID_ARGUMENT;
    if (!aeternum_abi_compatible(AETERNUM_MODULE_ABI_VERSION, host_abi_version))
        return AETERNUM_ERR_ABI_MISMATCH;
    g_host = host;

    out->info.id = CHATGUARD_ID;
    out->info.version = "0.2.1";
    out->info.api_range = "1";
    out->info.abi_version = AETERNUM_MODULE_ABI_VERSION;
    out->info.load_policy = AETERNUM_LOAD_POLICY_ON_BOOT_ONLY;
    out->info.critical = 0u;
    out->ft.struct_size = (uint32_t)sizeof(AeternumModuleFT);
    out->ft.on_load = cg_on_load;
    out->ft.on_init = cg_on_init;
    out->ft.on_start = cg_on_start;
    out->ft.on_stop = cg_on_stop;
    out->ft.on_shutdown = cg_on_shutdown;
    out->ft.metrics_observe = NULL;
    out->mod_ctx = &g_ctx;
    return AETERNUM_OK;
}