/*
 * Aeternum Engine public API — metrics / Prometheus text (SDK 1.0).
 * Real consumers: Metric local sink + AdminPrometheus helpers.
 */
#ifndef AETERNUM_API_OPS_METRICS_HPP
#define AETERNUM_API_OPS_METRICS_HPP

#include "aeternum/api/status.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <string_view>

namespace aeternum::ops
{
enum class MetricSinkKind : unsigned char
{
    Off = 0,
    Local,
    // External push remains optional/legacy — not part of frozen 1.0 surface.
};

struct MetricLocalConfig
{
    std::string_view directory;
    std::uint32_t budgetMB = 75;
    std::uint32_t retentionDays = 14;
    std::uint32_t flushSeconds = 10;
    std::uint32_t maxSeries = 256;
};

class IMetricSink
{
public:
    virtual ~IMetricSink() = default;
    virtual bool Enabled() const = 0;
    virtual MetricSinkKind Kind() const = 0;
    virtual Status Observe(std::string_view name, double value) = 0;
    virtual Status Flush() = 0;
};

/** Prometheus text exposition — produced by admin /metrics when enabled. */
class IPrometheusText
{
public:
    virtual ~IPrometheusText() = default;
    virtual std::string Render() const = 0;
};

/** Local metric store query + .mlines file load (admin /metrics, /store). */
struct MetricSeriesSnapshot
{
    std::string name;
    std::string labels;
    std::uint64_t count = 0;
    double sum = 0.0;
    double min = 0.0;
    double max = 0.0;
};

class IMetricLocalQuery
{
public:
    virtual ~IMetricLocalQuery() = default;
    virtual bool LocalSinkEnabled() const = 0;
    virtual std::uint64_t LocalStoreBytes() const = 0;
    virtual std::uint64_t LocalBudgetBytes() const = 0;
    virtual std::uint32_t LocalRetentionDays() const = 0;
    virtual std::size_t LocalMaxSeries() const = 0;
    virtual std::uint32_t LocalFileCount() const = 0;
    virtual std::string LocalDirectory() const = 0;
    virtual bool DenseScrapeEnabled() const = 0;
    virtual std::uint64_t DenseSampleCount() const = 0;
    virtual std::uint64_t DenseCpuNanos() const = 0;
    /** Load .mlines; if topN>0 apply TakeTopNBySum semantics. */
    virtual std::vector<MetricSeriesSnapshot> LoadSeriesFile(std::string_view path, std::size_t topN) const = 0;
};
}

#endif
