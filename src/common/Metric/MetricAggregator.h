#ifndef TRINITYCORE_METRIC_AGGREGATOR_H
#define TRINITYCORE_METRIC_AGGREGATOR_H

#include "Define.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Trinity::MetricLocal
{
struct SampleStats
{
    static constexpr uint32 ReservoirCap = 32;

    uint64 Count = 0;
    double Sum = 0.0;
    double Min = 0.0;
    double Max = 0.0;
    double Reservoir[ReservoirCap] = {};
    uint32 ReservoirFilled = 0;
    uint64 ReservoirSeen = 0;

    void Add(double value)
    {
        if (!Count)
            Min = Max = value;
        else
        {
            if (value < Min)
                Min = value;
            if (value > Max)
                Max = value;
        }
        Sum += value;
        ++Count;
        AddReservoir(value);
    }

    void AddReservoir(double value)
    {
        ++ReservoirSeen;
        if (ReservoirFilled < ReservoirCap)
        {
            Reservoir[ReservoirFilled++] = value;
            return;
        }
        // Deterministic Algorithm-R style pick (no <random> dependency).
        uint64 j = (ReservoirSeen * 11400714819323198485ull) % ReservoirSeen;
        if (j < ReservoirCap)
            Reservoir[j] = value;
    }

    void MergeReservoir(SampleStats const& other)
    {
        for (uint32 i = 0; i < other.ReservoirFilled; ++i)
            AddReservoir(other.Reservoir[i]);
    }

    double Avg() const { return Count ? (Sum / double(Count)) : 0.0; }

    /// Approximate percentile from reservoir; falls back to Max when empty.
    double ApproxPercentile(double p) const
    {
        if (!ReservoirFilled)
            return Max;
        double sorted[ReservoirCap];
        for (uint32 i = 0; i < ReservoirFilled; ++i)
            sorted[i] = Reservoir[i];
        for (uint32 i = 1; i < ReservoirFilled; ++i)
        {
            double key = sorted[i];
            int32 j = int32(i) - 1;
            while (j >= 0 && sorted[j] > key)
            {
                sorted[j + 1] = sorted[j];
                --j;
            }
            sorted[j + 1] = key;
        }
        if (p <= 0.0)
            return sorted[0];
        if (p >= 1.0)
            return sorted[ReservoirFilled - 1];
        double idx = p * double(ReservoirFilled - 1);
        uint32 lo = uint32(idx);
        uint32 hi = (lo + 1 < ReservoirFilled) ? lo + 1 : lo;
        double frac = idx - double(lo);
        return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
    }
};

struct SeriesKey
{
    std::string Name;
    std::string Labels;

    bool operator==(SeriesKey const& o) const noexcept
    {
        return Name == o.Name && Labels == o.Labels;
    }
};

struct SeriesKeyHash
{
    std::size_t operator()(SeriesKey const& k) const noexcept
    {
        return std::hash<std::string>{}(k.Name) ^ (std::hash<std::string>{}(k.Labels) << 1);
    }
};

struct SeriesSnapshot
{
    SeriesKey Key;
    SampleStats Stats;
};

/// Keep the hottest series by |Sum| (bounded cardinality for dense scrape).
inline std::vector<SeriesSnapshot> TakeTopNBySum(std::vector<SeriesSnapshot> series, std::size_t topN)
{
    if (topN == 0 || series.size() <= topN)
        return series;
    std::partial_sort(series.begin(), series.begin() + std::ptrdiff_t(topN), series.end(),
        [](SeriesSnapshot const& a, SeriesSnapshot const& b)
        {
            double aa = a.Stats.Sum < 0 ? -a.Stats.Sum : a.Stats.Sum;
            double bb = b.Stats.Sum < 0 ? -b.Stats.Sum : b.Stats.Sum;
            if (aa != bb)
                return aa > bb;
            return a.Stats.Count > b.Stats.Count;
        });
    series.resize(topN);
    return series;
}

class TC_COMMON_API Aggregator
{
public:
    explicit Aggregator(std::size_t maxSeries = 256);

    void SetMaxSeries(std::size_t maxSeries);
    std::size_t GetMaxSeries() const { return _maxSeries; }
    std::size_t SeriesCount() const;

    void Record(std::string_view name, double value, std::string_view labels = {});

    /// Snapshot current window and clear it. Sorted by Count desc, then Name.
    std::vector<SeriesSnapshot> Flush();

    /// Merge snapshots into this window (roll-up). Evicts lowest-count series over max.
    void Merge(std::vector<SeriesSnapshot> const& other);

    static std::string CanonicalLabels(std::vector<std::pair<std::string_view, std::string_view>> const& tags);

private:
    void EvictIfNeeded();

    std::size_t _maxSeries;
    mutable std::mutex _mutex;
    std::unordered_map<SeriesKey, SampleStats, SeriesKeyHash> _series;
};
}

#endif
