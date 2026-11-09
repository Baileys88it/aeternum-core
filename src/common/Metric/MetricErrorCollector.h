#ifndef TRINITYCORE_METRIC_ERROR_COLLECTOR_H
#define TRINITYCORE_METRIC_ERROR_COLLECTOR_H

#include "Define.h"
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Trinity::MetricLocal
{
/// Lightweight warn/error/fatal tally for the daily summary file.
/// Safe to call from the logging path; no Metric/Log includes in the header.
class TC_COMMON_API ErrorWarningCollector
{
public:
    static ErrorWarningCollector& Instance();

    /// level: 4=WARN, 5=ERROR, 6=FATAL (matches LogLevel). Others ignored.
    void Note(uint8 level, std::string_view category, std::string_view message);

    struct CategoryCount
    {
        std::string Category;
        uint64 Count = 0;
    };

    struct TopMessage
    {
        uint8 Level = 0;
        std::string Category;
        std::string Message;
        uint64 Count = 0;
    };

    struct Snapshot
    {
        uint64 WarnTotal = 0;
        uint64 ErrorTotal = 0;
        uint64 FatalTotal = 0;
        std::vector<CategoryCount> WarnByCategory;
        std::vector<CategoryCount> ErrorByCategory;
        std::vector<TopMessage> TopMessages;
    };

    Snapshot Peek() const;
    Snapshot Flush(); // snapshot + clear

    void SetLimits(std::size_t maxCategories, std::size_t maxMessages, std::size_t maxMessageChars);

private:
    ErrorWarningCollector() = default;

    mutable std::mutex _mutex;
    uint64 _warnTotal = 0;
    uint64 _errorTotal = 0;
    uint64 _fatalTotal = 0;
    std::unordered_map<std::string, uint64> _warnByCat;
    std::unordered_map<std::string, uint64> _errorByCat;
    std::unordered_map<std::string, uint64> _msgCounts; // "L|cat|msg"
    std::size_t _maxCategories = 64;
    std::size_t _maxMessages = 32;
    std::size_t _maxMessageChars = 120;

    Snapshot BuildUnlocked() const;
};

/// C ABI-ish free function so Log.cpp can call without heavy deps beyond the header.
TC_COMMON_API void NoteLogForDailySummary(uint8 level, std::string_view category, std::string_view message);
}

#endif
