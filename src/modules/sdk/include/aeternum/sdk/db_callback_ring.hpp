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
 * Aeternum M1 - lock-free MPSC ring for DB callback descriptors (D6 / P-b).
 *
 * Choice: MPSC (multi-producer, single-consumer).
 * Justification: DatabaseWorkerPool posts work to Trinity::Asio::IoContext with
 * `_async_threads` workers (DatabaseWorkerPool.h/.cpp: SetConnectionInfo /
 * OpenConnections(IDX_ASYNC, _async_threads)). Completions that must reach the
 * World Thread can therefore arrive from multiple DB worker threads concurrently.
 * World Thread is the sole consumer and must never block (try_pop only).
 *
 * Algorithm: Vyukov bounded MPSC (per-slot sequence numbers).
 * Memory order: producers use acq_rel on slot.sequence after write; consumer
 * uses acquire on slot.sequence before read and release after reclaim.
 * No heap allocation after construction. Cache-line padding on indices.
 */
#ifndef AETERNUM_SDK_DB_CALLBACK_RING_HPP
#define AETERNUM_SDK_DB_CALLBACK_RING_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

namespace aeternum::sdk
{

/** POD callback descriptor for the module DB path (no owning pointers required). */
struct DbCallbackDescriptor
{
    uint64_t token = 0;
    uint32_t nbytes = 0;
    uint32_t flags = 0;
};

static_assert(std::is_trivially_copyable_v<DbCallbackDescriptor>, "DbCallbackDescriptor must be POD-ish");

template <std::size_t Capacity>
class DbCallbackRing
{
    static_assert((Capacity & (Capacity - 1u)) == 0u, "Capacity must be power of two");
    static_assert(Capacity >= 2u, "Capacity too small");

public:
    static constexpr std::size_t kCapacity = Capacity;
    static constexpr std::size_t kMask = Capacity - 1u;

    DbCallbackRing() noexcept
    {
        for (std::size_t i = 0; i < Capacity; ++i)
            _slots[i].sequence.store(i, std::memory_order_relaxed);
        _enqueuePos.store(0, std::memory_order_relaxed);
        _dequeuePos.store(0, std::memory_order_relaxed);
    }

    DbCallbackRing(DbCallbackRing const&) = delete;
    DbCallbackRing& operator=(DbCallbackRing const&) = delete;

    /** Producer: returns false when full (backpressure). Never blocks. */
    bool try_push(DbCallbackDescriptor const& item) noexcept
    {
        std::size_t pos = _enqueuePos.load(std::memory_order_relaxed);
        for (;;)
        {
            Slot& slot = _slots[pos & kMask];
            std::size_t const seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t const dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (dif == 0)
            {
                if (_enqueuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                {
                    slot.data = item;
                    slot.sequence.store(pos + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0)
            {
                return false; /* full */
            }
            else
            {
                pos = _enqueuePos.load(std::memory_order_relaxed);
            }
        }
    }

    /** Consumer (World Thread): non-blocking. Returns false when empty. */
    bool try_pop(DbCallbackDescriptor& out) noexcept
    {
        std::size_t pos = _dequeuePos.load(std::memory_order_relaxed);
        Slot& slot = _slots[pos & kMask];
        std::size_t const seq = slot.sequence.load(std::memory_order_acquire);
        intptr_t const dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
        if (dif == 0)
        {
            _dequeuePos.store(pos + 1, std::memory_order_relaxed);
            out = slot.data;
            slot.sequence.store(pos + kCapacity, std::memory_order_release);
            return true;
        }
        return false; /* empty or not yet published */
    }

    std::size_t approx_size() const noexcept
    {
        std::size_t const e = _enqueuePos.load(std::memory_order_relaxed);
        std::size_t const d = _dequeuePos.load(std::memory_order_relaxed);
        return e - d;
    }

private:
    struct alignas(64) Slot
    {
        std::atomic<std::size_t> sequence;
        DbCallbackDescriptor data{};
    };

    Slot _slots[Capacity];
    alignas(64) std::atomic<std::size_t> _enqueuePos;
    alignas(64) std::atomic<std::size_t> _dequeuePos;
};

/** M1 prescribed capacity (maintainer): 8192 slots. */
using DbCallbackRing8192 = DbCallbackRing<8192>;

} // namespace aeternum::sdk

#endif /* AETERNUM_SDK_DB_CALLBACK_RING_HPP */