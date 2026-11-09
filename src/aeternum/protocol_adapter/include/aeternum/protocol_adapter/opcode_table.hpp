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
 * Aeternum IE-M2 Blocco1 - dense preallocated opcode translation table (hot-path half).
 *
 * Structural rules (enforced by contrib/aeternum/ci/check_adapter_no_map.ps1):
 *  - no associative containers anywhere in the adapter;
 *  - this header: no heap (no new/malloc/vector/string), fixed-capacity arrays only.
 *
 * Dense scheme for 24-bit opcodes 0xGGIIII (group byte + 16-bit index):
 *  level 1: GroupSpan[256] indexed by GG -> { base, minIndex, count } (built at boot from the data)
 *  level 2: OpcodeEntry slots[kMaxSlotsPerMap]; slot = base + (II - minIndex) if (II - minIndex) < count
 *  Lookup = 1 range check + 2 array reads + 1 valid-bit test: O(1), branch-light, zero allocation.
 *  Keys > 0xFFFFFF (e.g. 0xFFFFFFFF hole marker) are rejected before indexing (out of range).
 *
 * Golden rule (IE-M2): ignored_known -> drop + counter, ZERO logging / I/O / heap.
 *                      unknown       -> drop + counter + rate-limited ERROR through an injected sink.
 * Blocco3: inbound wired (shadow.hpp). Blocco4: outbound SMSG logical -> wire uses smsgLogicalToWire
 * (exposed as SmsgAdapterTable(), same boot-loaded dense array - no second copy) via outbound.hpp.
 * Blocco5: SMSG send policy lives in the same entries: kFlagBlocked = never sent, kFlagStub = send the fixed
 * placeholder payload (StubPayload, opcodes.json smsg_stub_payloads). Replaces ClassicOpcodes block/stub helpers.
 */
#ifndef AETERNUM_PROTOCOL_ADAPTER_OPCODE_TABLE_HPP
#define AETERNUM_PROTOCOL_ADAPTER_OPCODE_TABLE_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace aeternum::protocol_adapter
{
inline constexpr std::uint32_t kOpcodeKeyMax = 0x00FFFFFFu;  // 0xGGIIII
inline constexpr std::uint32_t kOpcodeGroups = 256u;
inline constexpr std::uint32_t kMaxSlotsPerMap = 4096u;     // 70790 needs <= 2012 per map (see M2_BLOCK1.md)
inline constexpr std::uint32_t kInvalidOpcode = 0xFFFFFFFFu;
inline constexpr std::uint32_t kMaxStubPayloads = 16u;      // Blocco5: fixed capacity (70790 uses 2)
inline constexpr std::uint32_t kMaxStubPayloadBytes = 64u;  // Blocco5: largest placeholder payload (70790 uses 4)

/** Entry flags (mirror of gen_opcode_table.py F_*) + internal valid bit. */
enum OpcodeFlag : std::uint16_t
{
    kFlagHandled      = 0x0001,
    kFlagCritical     = 0x0002,
    kFlagBlocked      = 0x0004,
    kFlagStub         = 0x0008,
    kFlagOverride     = 0x0010,
    kFlagAlias        = 0x0020,
    kFlagIgnoredKnown = 0x0040,
    kFlagValid        = 0x8000
};

/** Per-slot classification (unknown = no slot). */
enum class OpcodeClass : std::uint8_t
{
    Unknown = 0,
    Handled = 1,        // core has an active handler / socket-level processing
    KnownUnhandled = 2, // named in the core table but no handler (STATUS_UNHANDLED / IGNORED) - core decides as today
    IgnoredKnown = 3    // profile ignored_known: drop + counter, never log
};

enum class OpcodeDecision : std::uint8_t
{
    Dispatch = 0,
    Drop = 1
};

struct OpcodeEntry
{
    std::uint32_t key = 0;    // lookup key (wire or logical, depending on the map)
    std::uint32_t value = 0;  // translated id
    std::uint16_t flags = 0;
    std::uint8_t status = 0;  // aeternum::protocol::HandlerStatus numeric value
    OpcodeClass cls = OpcodeClass::Unknown;
};
static_assert(sizeof(OpcodeEntry) == 12, "OpcodeEntry layout");

/** IE-M2 Blocco5: fixed placeholder payload for one kFlagStub SMSG. Filled at boot by the loader, read-only afterwards. */
struct StubPayload
{
    std::uint32_t logical = 0;
    std::uint32_t size = 0;
    std::array<std::uint8_t, kMaxStubPayloadBytes> bytes{};
};

struct GroupSpan
{
    std::uint32_t base = 0;
    std::uint32_t minIndex = 0;
    std::uint32_t count = 0;  // 0 = group absent
};

/**
 * One direction of translation (key -> value). Capacity fixed at compile time.
 * Build (boot only, single thread): Reset -> NoteKey* -> Layout -> Insert*.
 * Read (any thread): Find / Hit.
 */
class DenseOpcodeMap
{
public:
    DenseOpcodeMap() noexcept { Reset(); }
    DenseOpcodeMap(DenseOpcodeMap const&) = delete;
    DenseOpcodeMap& operator=(DenseOpcodeMap const&) = delete;

    void Reset() noexcept
    {
        for (std::uint32_t g = 0; g < kOpcodeGroups; ++g)
        {
            _groups[g] = GroupSpan{};
            _buildMin[g] = 0xFFFFu;
            _buildMax[g] = 0u;
            _buildSeen[g] = false;
        }
        for (std::uint32_t i = 0; i < kMaxSlotsPerMap; ++i)
        {
            _slots[i] = OpcodeEntry{};
            _hits[i].store(0, std::memory_order_relaxed);
        }
        _slotsUsed = 0;
        _entries = 0;
        _laidOut = false;
    }

    /** Phase 1: extend the span of the key's group. */
    bool NoteKey(std::uint32_t key) noexcept
    {
        if (_laidOut || key > kOpcodeKeyMax)
            return false;
        std::uint32_t const g = key >> 16;
        std::uint32_t const idx = key & 0xFFFFu;
        _buildSeen[g] = true;
        if (idx < _buildMin[g]) _buildMin[g] = idx;
        if (idx > _buildMax[g]) _buildMax[g] = idx;
        return true;
    }

    /** Phase 2: assign contiguous slot ranges per group. False if capacity is exceeded. */
    bool Layout() noexcept
    {
        if (_laidOut)
            return false;
        std::uint32_t used = 0;
        for (std::uint32_t g = 0; g < kOpcodeGroups; ++g)
        {
            if (!_buildSeen[g])
                continue;
            std::uint32_t const count = _buildMax[g] - _buildMin[g] + 1u;
            if (count > kMaxSlotsPerMap - used)
                return false;
            _groups[g] = GroupSpan{ used, _buildMin[g], count };
            used += count;
        }
        _slotsUsed = used;
        _laidOut = true;
        return true;
    }

    enum class InsertResult : std::uint8_t { Ok, NotLaidOut, OutOfRange, Duplicate };

    /** Phase 3: place an entry (key must have been noted). */
    InsertResult Insert(OpcodeEntry const& e) noexcept
    {
        if (!_laidOut)
            return InsertResult::NotLaidOut;
        std::int32_t const slot = SlotOf(e.key);
        if (slot < 0)
            return InsertResult::OutOfRange;
        OpcodeEntry& dst = _slots[static_cast<std::uint32_t>(slot)];
        if (dst.flags & kFlagValid)
            return InsertResult::Duplicate;
        dst = e;
        dst.flags = static_cast<std::uint16_t>(dst.flags | kFlagValid);
        ++_entries;
        return InsertResult::Ok;
    }

    /** O(1), noexcept, zero allocation. nullptr = not found / out of range. */
    OpcodeEntry const* Find(std::uint32_t key) const noexcept
    {
        std::int32_t const slot = SlotOf(key);
        if (slot < 0)
            return nullptr;
        OpcodeEntry const& e = _slots[static_cast<std::uint32_t>(slot)];
        return (e.flags & kFlagValid) ? &e : nullptr;
    }

    /** Relaxed per-slot hit counter; returns the new value. */
    std::uint32_t Hit(OpcodeEntry const* e) noexcept
    {
        std::size_t const i = static_cast<std::size_t>(e - _slots.data());
        return _hits[i].fetch_add(1, std::memory_order_relaxed) + 1u;
    }

    std::uint32_t Hits(std::uint32_t key) const noexcept
    {
        std::int32_t const slot = SlotOf(key);
        return slot < 0 ? 0u : _hits[static_cast<std::uint32_t>(slot)].load(std::memory_order_relaxed);
    }

    std::uint32_t Entries() const noexcept { return _entries; }
    std::uint32_t SlotsUsed() const noexcept { return _slotsUsed; }
    bool LaidOut() const noexcept { return _laidOut; }

    /** Boot-only / test helper: visit every valid entry in slot order. Not for the packet path. */
    template <typename Visitor>
    void ForEachEntry(Visitor&& visit) const
    {
        for (std::uint32_t i = 0; i < _slotsUsed; ++i)
            if (_slots[i].flags & kFlagValid)
                visit(_slots[i]);
    }

private:
    std::int32_t SlotOf(std::uint32_t key) const noexcept
    {
        if (key > kOpcodeKeyMax)
            return -1;
        GroupSpan const& g = _groups[key >> 16];
        std::uint32_t const off = (key & 0xFFFFu) - g.minIndex;  // wraps when idx < minIndex
        if (off >= g.count)
            return -1;
        return static_cast<std::int32_t>(g.base + off);
    }

    std::array<GroupSpan, kOpcodeGroups> _groups{};
    std::array<OpcodeEntry, kMaxSlotsPerMap> _slots{};
    std::array<std::atomic<std::uint32_t>, kMaxSlotsPerMap> _hits{};
    // build-time scratch (boot only)
    std::array<std::uint32_t, kOpcodeGroups> _buildMin{};
    std::array<std::uint32_t, kOpcodeGroups> _buildMax{};
    std::array<bool, kOpcodeGroups> _buildSeen{};
    std::uint32_t _slotsUsed = 0;
    std::uint32_t _entries = 0;
    bool _laidOut = false;
};

/** Unknown-opcode rate limiting (profile opcodes/unknown_policy.json "unknown"). */
struct UnknownPolicy
{
    std::uint32_t windowMs = 10000;
    std::uint32_t thresholdPerWindow = 50;  // <= threshold unknowns in a window: no log at all
    std::uint32_t maxErrorsPerWindow = 1;   // above threshold: at most this many ERROR emissions per window
};

enum class OpcodeDirection : std::uint8_t { ClientWire = 0, ClientLogical = 1, ServerLogical = 2 };

/**
 * ERROR sink (plain function pointer: no std::function, no heap). Called only above the threshold,
 * at most maxErrorsPerWindow times per window. The host decides how to log (TC_LOG_ERROR later).
 */
using UnknownErrorSink = void (*)(void* ctx, std::uint32_t opcode, OpcodeDirection dir,
                                  std::uint32_t countInWindow, std::uint64_t unknownTotal) noexcept;

struct OpcodeClassification
{
    OpcodeClass cls = OpcodeClass::Unknown;
    OpcodeDecision decision = OpcodeDecision::Drop;
    std::uint32_t translated = kInvalidOpcode;  // logical id (from wire) or wire id (from logical)
    std::uint16_t flags = 0;
    std::uint8_t status = 0;
};

/**
 * Rate-limit window + counters for one traffic direction. Inbound (client) and outbound (server, Blocco4)
 * keep separate windows so an outbound problem can never consume the inbound budget (Blocco3 behaviour unchanged).
 */
struct UnknownRateState
{
    std::atomic<std::uint64_t> unknownTotal{ 0 };
    std::atomic<std::uint64_t> outOfRangeTotal{ 0 };
    std::atomic<std::uint64_t> errorsEmitted{ 0 };
    std::atomic<std::uint64_t> errorsSuppressed{ 0 };
    std::atomic<std::uint64_t> windowStartMs{ 0 };
    std::atomic<std::uint32_t> windowCount{ 0 };
    std::atomic<std::uint32_t> windowErrors{ 0 };
    std::atomic<bool> windowArmed{ false };

    void Reset() noexcept
    {
        unknownTotal.store(0, std::memory_order_relaxed);
        outOfRangeTotal.store(0, std::memory_order_relaxed);
        errorsEmitted.store(0, std::memory_order_relaxed);
        errorsSuppressed.store(0, std::memory_order_relaxed);
        windowStartMs.store(0, std::memory_order_relaxed);
        windowCount.store(0, std::memory_order_relaxed);
        windowErrors.store(0, std::memory_order_relaxed);
        windowArmed.store(false, std::memory_order_relaxed);
    }
};

/**
 * Process-wide table for ONE active profile (ADR-002). Large (~0.3 MB): allocate once at boot
 * (static storage or one boot-time allocation); never per packet.
 */
class AdapterOpcodeTable
{
public:
    AdapterOpcodeTable() noexcept = default;
    AdapterOpcodeTable(AdapterOpcodeTable const&) = delete;
    AdapterOpcodeTable& operator=(AdapterOpcodeTable const&) = delete;

    DenseOpcodeMap cmsgWireToLogical;   // recv: wire -> logical (all reachable wires, aliases included)
    DenseOpcodeMap cmsgLogicalToWire;   // preferred wire per logical id
    DenseOpcodeMap smsgLogicalToWire;   // send: logical -> wire (Blocco4 SmsgAdapterTable)
    DenseOpcodeMap smsgWireToLogical;   // replay/sniff decode (first wire wins on collision)

    /** IE-M2 Blocco4: the dense preallocated SMSG logical -> wire table used by the send path (no duplicate copy). */
    DenseOpcodeMap const& SmsgAdapterTable() const noexcept { return smsgLogicalToWire; }

    void Reset() noexcept
    {
        cmsgWireToLogical.Reset();
        cmsgLogicalToWire.Reset();
        smsgLogicalToWire.Reset();
        smsgWireToLogical.Reset();
        ResetCounters();
        _policy = UnknownPolicy{};
        for (std::uint32_t i = 0; i < kMaxStubPayloads; ++i)
            _stubs[i] = StubPayload{};
        _stubCount = 0;
        _loaded = false;
    }

    /** Blocco5, boot only (loader): register the placeholder payload of a kFlagStub SMSG. False = full / oversized / duplicate. */
    bool AddStubPayload(std::uint32_t logical, std::uint8_t const* data, std::uint32_t size) noexcept
    {
        if (_stubCount >= kMaxStubPayloads || size > kMaxStubPayloadBytes || (size && !data))
            return false;
        for (std::uint32_t i = 0; i < _stubCount; ++i)
            if (_stubs[i].logical == logical)
                return false;
        StubPayload& st = _stubs[_stubCount];
        st.logical = logical;
        st.size = size;
        for (std::uint32_t i = 0; i < size; ++i)
            st.bytes[i] = data[i];
        ++_stubCount;
        return true;
    }

    /**
     * Blocco5 send path: payload of a kFlagStub SMSG. Bounded scan of <= kMaxStubPayloads (compile-time constant) entries,
     * reached only for entries that carry kFlagStub. noexcept, zero heap. nullptr = none.
     */
    StubPayload const* FindStubPayload(std::uint32_t logical) const noexcept
    {
        for (std::uint32_t i = 0; i < _stubCount; ++i)
            if (_stubs[i].logical == logical)
                return &_stubs[i];
        return nullptr;
    }

    std::uint32_t StubPayloadCount() const noexcept { return _stubCount; }

    void ResetCounters() noexcept
    {
        _ignoredKnownTotal.store(0, std::memory_order_relaxed);
        _in.Reset();
        _out.Reset();
    }

    // boot-time configuration
    void SetUnknownPolicy(UnknownPolicy const& p) noexcept { _policy = p; }
    UnknownPolicy const& GetUnknownPolicy() const noexcept { return _policy; }
    void SetUnknownErrorSink(UnknownErrorSink sink, void* ctx) noexcept { _sink = sink; _sinkCtx = ctx; }
    void MarkLoaded(bool v) noexcept { _loaded = v; }
    bool Loaded() const noexcept { return _loaded; }

    /** Recv path classification from the raw client wire id. noexcept, zero heap, zero I/O (except sink above threshold). */
    OpcodeClassification ClassifyClientWire(std::uint32_t wire, std::uint64_t nowMs) noexcept
    {
        return Classify(cmsgWireToLogical, wire, OpcodeDirection::ClientWire, nowMs);
    }

    /** Same, from an already translated (logical/core) client id - today's WorldSocket numbering. */
    OpcodeClassification ClassifyClientLogical(std::uint32_t logical, std::uint64_t nowMs) noexcept
    {
        return Classify(cmsgLogicalToWire, logical, OpcodeDirection::ClientLogical, nowMs);
    }

    /** IE-M2 Blocco4 send path: logical (core) SMSG id -> client wire. Unmapped -> Unknown/Drop on the OUTBOUND window. */
    OpcodeClassification ClassifyServerLogical(std::uint32_t logical, std::uint64_t nowMs) noexcept
    {
        return Classify(smsgLogicalToWire, logical, OpcodeDirection::ServerLogical, nowMs);
    }

    // inbound (client) counters - unchanged since Blocco1
    std::uint64_t IgnoredKnownTotal() const noexcept { return _ignoredKnownTotal.load(std::memory_order_relaxed); }
    std::uint64_t UnknownTotal() const noexcept { return _in.unknownTotal.load(std::memory_order_relaxed); }
    std::uint64_t OutOfRangeTotal() const noexcept { return _in.outOfRangeTotal.load(std::memory_order_relaxed); }
    std::uint64_t ErrorsEmitted() const noexcept { return _in.errorsEmitted.load(std::memory_order_relaxed); }
    std::uint64_t ErrorsSuppressed() const noexcept { return _in.errorsSuppressed.load(std::memory_order_relaxed); }

    // outbound (server, Blocco4) counters
    std::uint64_t OutboundUnknownTotal() const noexcept { return _out.unknownTotal.load(std::memory_order_relaxed); }
    std::uint64_t OutboundOutOfRangeTotal() const noexcept { return _out.outOfRangeTotal.load(std::memory_order_relaxed); }
    std::uint64_t OutboundErrorsEmitted() const noexcept { return _out.errorsEmitted.load(std::memory_order_relaxed); }
    std::uint64_t OutboundErrorsSuppressed() const noexcept { return _out.errorsSuppressed.load(std::memory_order_relaxed); }

private:
    OpcodeClassification Classify(DenseOpcodeMap& map, std::uint32_t key, OpcodeDirection dir, std::uint64_t nowMs) noexcept
    {
        OpcodeClassification r;
        if (OpcodeEntry const* e = map.Find(key))
        {
            map.Hit(e);
            r.cls = e->cls;
            r.translated = e->value;
            r.flags = e->flags;
            r.status = e->status;
            if (e->cls == OpcodeClass::IgnoredKnown)
            {
                // Golden rule: counter only. No log, no I/O, no heap.
                _ignoredKnownTotal.fetch_add(1, std::memory_order_relaxed);
                r.decision = OpcodeDecision::Drop;
            }
            else
                r.decision = OpcodeDecision::Dispatch;
            return r;
        }
        UnknownRateState& st = dir == OpcodeDirection::ServerLogical ? _out : _in;
        if (key > kOpcodeKeyMax)
            st.outOfRangeTotal.fetch_add(1, std::memory_order_relaxed);
        OnUnknown(st, key, dir, nowMs);
        return r;  // Unknown / Drop
    }

    void OnUnknown(UnknownRateState& st, std::uint32_t key, OpcodeDirection dir, std::uint64_t nowMs) noexcept
    {
        std::uint64_t const total = st.unknownTotal.fetch_add(1, std::memory_order_relaxed) + 1u;

        // Window roll (relaxed atomics; under a race only one thread wins the CAS and resets the window).
        bool const armed = st.windowArmed.load(std::memory_order_relaxed);
        std::uint64_t start = st.windowStartMs.load(std::memory_order_relaxed);
        if (!armed)
        {
            st.windowStartMs.store(nowMs, std::memory_order_relaxed);
            st.windowCount.store(0, std::memory_order_relaxed);
            st.windowErrors.store(0, std::memory_order_relaxed);
            st.windowArmed.store(true, std::memory_order_relaxed);
        }
        else if ((nowMs < start || nowMs - start >= _policy.windowMs)
                 && st.windowStartMs.compare_exchange_strong(start, nowMs, std::memory_order_relaxed))
        {
            st.windowCount.store(0, std::memory_order_relaxed);
            st.windowErrors.store(0, std::memory_order_relaxed);
        }

        std::uint32_t const inWindow = st.windowCount.fetch_add(1, std::memory_order_relaxed) + 1u;
        if (inWindow <= _policy.thresholdPerWindow)
            return;  // below threshold: counter only

        std::uint32_t const errIdx = st.windowErrors.fetch_add(1, std::memory_order_relaxed);
        if (errIdx < _policy.maxErrorsPerWindow && _sink)
        {
            st.errorsEmitted.fetch_add(1, std::memory_order_relaxed);
            _sink(_sinkCtx, key, dir, inWindow, total);
        }
        else
            st.errorsSuppressed.fetch_add(1, std::memory_order_relaxed);
    }

    UnknownPolicy _policy{};
    std::array<StubPayload, kMaxStubPayloads> _stubs{};  // Blocco5 kFlagStub payloads
    std::uint32_t _stubCount = 0;
    UnknownErrorSink _sink = nullptr;
    void* _sinkCtx = nullptr;
    bool _loaded = false;

    std::atomic<std::uint64_t> _ignoredKnownTotal{ 0 };
    UnknownRateState _in{};   // client -> server
    UnknownRateState _out{};  // server -> client (Blocco4)
};
} // namespace aeternum::protocol_adapter

#endif /* AETERNUM_PROTOCOL_ADAPTER_OPCODE_TABLE_HPP */
