#include "MetricAggregator.h"
#include <algorithm>
#include <sstream>

namespace Trinity::MetricLocal
{
Aggregator::Aggregator(std::size_t maxSeries) : _maxSeries(maxSeries ? maxSeries : 1)
{
}

void Aggregator::SetMaxSeries(std::size_t maxSeries)
{
    std::scoped_lock lock(_mutex);
    _maxSeries = maxSeries ? maxSeries : 1;
    EvictIfNeeded();
}

std::size_t Aggregator::SeriesCount() const
{
    std::scoped_lock lock(_mutex);
    return _series.size();
}

std::string Aggregator::CanonicalLabels(std::vector<std::pair<std::string_view, std::string_view>> const& tags)
{
    if (tags.empty())
        return {};

    std::vector<std::pair<std::string, std::string>> sorted;
    sorted.reserve(tags.size());
    for (auto const& [k, v] : tags)
    {
        if (k.empty())
            continue;
        sorted.emplace_back(std::string(k), std::string(v));
    }
    std::sort(sorted.begin(), sorted.end());

    std::ostringstream oss;
    bool first = true;
    for (auto const& [k, v] : sorted)
    {
        if (!first)
            oss << ',';
        first = false;
        oss << k << '=' << v;
    }
    return oss.str();
}

void Aggregator::Record(std::string_view name, double value, std::string_view labels)
{
    if (name.empty())
        return;

    std::scoped_lock lock(_mutex);
    SeriesKey key{ std::string(name), std::string(labels) };
    auto it = _series.find(key);
    if (it == _series.end())
    {
        if (_series.size() >= _maxSeries)
        {
            // Drop new series when at capacity (cardinality hard limit).
            // Prefer keeping existing hot series.
            return;
        }
        SampleStats stats;
        stats.Add(value);
        _series.emplace(std::move(key), stats);
        return;
    }
    it->second.Add(value);
}

void Aggregator::EvictIfNeeded()
{
    while (_series.size() > _maxSeries)
    {
        auto victim = std::min_element(_series.begin(), _series.end(),
            [](auto const& a, auto const& b)
            {
                if (a.second.Count != b.second.Count)
                    return a.second.Count < b.second.Count;
                return a.first.Name < b.first.Name;
            });
        if (victim == _series.end())
            break;
        _series.erase(victim);
    }
}

std::vector<SeriesSnapshot> Aggregator::Flush()
{
    std::scoped_lock lock(_mutex);
    std::vector<SeriesSnapshot> out;
    out.reserve(_series.size());
    for (auto& [key, stats] : _series)
        out.push_back(SeriesSnapshot{ key, stats });
    _series.clear();
    std::sort(out.begin(), out.end(),
        [](SeriesSnapshot const& a, SeriesSnapshot const& b)
        {
            if (a.Stats.Count != b.Stats.Count)
                return a.Stats.Count > b.Stats.Count;
            if (a.Key.Name != b.Key.Name)
                return a.Key.Name < b.Key.Name;
            return a.Key.Labels < b.Key.Labels;
        });
    return out;
}

void Aggregator::Merge(std::vector<SeriesSnapshot> const& other)
{
    std::scoped_lock lock(_mutex);
    for (SeriesSnapshot const& snap : other)
    {
        auto& dst = _series[snap.Key];
        if (!dst.Count)
        {
            dst = snap.Stats;
            continue;
        }
        dst.Count += snap.Stats.Count;
        dst.Sum += snap.Stats.Sum;
        if (snap.Stats.Min < dst.Min)
            dst.Min = snap.Stats.Min;
        if (snap.Stats.Max > dst.Max)
            dst.Max = snap.Stats.Max;
        dst.MergeReservoir(snap.Stats);
    }
    EvictIfNeeded();
}
}
