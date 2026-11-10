#ifndef TRINITYCORE_METRIC_HUMAN_REPORT_H
#define TRINITYCORE_METRIC_HUMAN_REPORT_H

#include "MetricAggregator.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Trinity::MetricLocal
{
struct ThresholdHint
{
    double WarnAbove = 0.0;
    std::string Hint; // "where to improve"
};

class TC_COMMON_API HumanReport
{
public:
    void SetThreshold(std::string name, ThresholdHint hint);
    void LoadDefaultCatalogThresholds();

    /// Build a short human-readable report (markdown-ish plain text).
    std::string Build(std::vector<SeriesSnapshot> const& series, std::size_t topN = 10) const;

private:
    std::unordered_map<std::string, ThresholdHint> _thresholds;
};
}

#endif
