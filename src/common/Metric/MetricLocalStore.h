#ifndef TRINITYCORE_METRIC_LOCAL_STORE_H
#define TRINITYCORE_METRIC_LOCAL_STORE_H

#include "MetricAggregator.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Trinity::MetricLocal
{
enum class Resolution : uint8
{
    TenSeconds = 0,
    OneMinute  = 1,
    OneHour    = 2,
    OneDay     = 3
};

struct StoreConfig
{
    std::filesystem::path Directory;
    uint32 BudgetBytes = 75u * 1024u * 1024u; // ~75 MB
    uint32 RetentionDays = 14;
    bool CompressRotated = false; // optional; uses zlib when true
    std::size_t MaxSeries = 256;
};

class TC_COMMON_API LocalStore
{
public:
    LocalStore() = default;

    void Configure(StoreConfig cfg);
    StoreConfig const& GetConfig() const { return _cfg; }

    /// Write one flushed window at the given resolution. Enforces budget afterwards.
    bool WriteWindow(Resolution res, std::chrono::system_clock::time_point ts,
        std::vector<SeriesSnapshot> const& series);

    /// Total bytes under the metrics directory (files only).
    uint64 DirectoryBytes() const;
    /// Count of regular files under the metrics directory.
    uint32 FileCount() const;

    /// Delete oldest files until under budget; also drop files older than retention.
    uint32 EnforceBudgetAndRetention();

    /// Paths helper for tests.
    static std::string FileNameFor(Resolution res, std::chrono::system_clock::time_point ts);
    static std::string FormatLine(SeriesSnapshot const& snap);

    /// Parse a compact line back (for roll-up / tests). Returns false on malformed.
    static bool ParseLine(std::string_view line, SeriesSnapshot& out);

    /// Load all series from a file (uncompressed .mlines).
    static std::vector<SeriesSnapshot> LoadFile(std::filesystem::path const& path);

private:
    StoreConfig _cfg;
};
}

#endif
