#ifndef TRINITYCORE_METRIC_DAILY_SUMMARY_H
#define TRINITYCORE_METRIC_DAILY_SUMMARY_H

#include "MetricAggregator.h"
#include "MetricErrorCollector.h"
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace Trinity::MetricLocal
{
/// Compact human-readable daily rollup (avg / p95 / max + error/warn summary).
struct DailySummary
{
    std::chrono::system_clock::time_point DayUtc{};
    std::vector<SeriesSnapshot> Metrics;
    ErrorWarningCollector::Snapshot Errors;
};

class TC_COMMON_API DailySummaryWriter
{
public:
    /// Build body text (no I/O). Stable format aeternum-daily-v1.
    static std::string Format(DailySummary const& summary);

    /// Filename: daily_YYYYMMDD.txt (one file per UTC day).
    static std::string FileNameFor(std::chrono::system_clock::time_point dayUtc);

    /// Write under directory; returns path written or empty on failure.
    static std::filesystem::path Write(std::filesystem::path const& directory, DailySummary const& summary);
};
}

#endif
