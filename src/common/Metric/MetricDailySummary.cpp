#include "MetricDailySummary.h"
#include "Log.h"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Trinity::MetricLocal
{
namespace
{
char const* LevelName(uint8 level)
{
    switch (level)
    {
        case 4: return "WARN";
        case 5: return "ERROR";
        case 6: return "FATAL";
        default: return "LOG";
    }
}
}

std::string DailySummaryWriter::FileNameFor(std::chrono::system_clock::time_point dayUtc)
{
    std::time_t t = std::chrono::system_clock::to_time_t(dayUtc);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << "daily_" << std::put_time(&tm, "%Y%m%d") << ".txt";
    return oss.str();
}

std::string DailySummaryWriter::Format(DailySummary const& summary)
{
    std::time_t t = std::chrono::system_clock::to_time_t(summary.DayUtc);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(3);
    oss << "# aeternum-daily-v1\n";
    oss << "# date_utc=" << std::put_time(&tm, "%Y-%m-%d") << '\n';
    oss << "# metrics_count=" << summary.Metrics.size() << '\n';
    oss << "# p95=reservoir_approx_or_max\n";
    oss << "#\n";
    oss << "# metrics: name|labels|count|avg|p95|max\n";
    for (SeriesSnapshot const& s : summary.Metrics)
    {
        oss << s.Key.Name << '|'
            << s.Key.Labels << '|'
            << s.Stats.Count << '|'
            << s.Stats.Avg() << '|'
            << s.Stats.ApproxPercentile(0.95) << '|'
            << s.Stats.Max << '\n';
    }
    oss << "#\n";
    oss << "# errors_warnings\n";
    oss << "warn_total=" << summary.Errors.WarnTotal << '\n';
    oss << "error_total=" << summary.Errors.ErrorTotal << '\n';
    oss << "fatal_total=" << summary.Errors.FatalTotal << '\n';
    oss << "# by_category\n";
    for (auto const& c : summary.Errors.WarnByCategory)
        oss << "WARN|" << c.Category << '=' << c.Count << '\n';
    for (auto const& c : summary.Errors.ErrorByCategory)
        oss << "ERROR|" << c.Category << '=' << c.Count << '\n';
    oss << "# top_messages (count|level|category|message)\n";
    for (auto const& m : summary.Errors.TopMessages)
        oss << m.Count << '|' << LevelName(m.Level) << '|' << m.Category << '|' << m.Message << '\n';
    return oss.str();
}

std::filesystem::path DailySummaryWriter::Write(std::filesystem::path const& directory, DailySummary const& summary)
{
    if (directory.empty())
        return {};
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    std::filesystem::path path = directory / FileNameFor(summary.DayUtc);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        TC_LOG_ERROR("metric", "Metric daily summary: failed to open {}", path.string());
        return {};
    }
    out << Format(summary);
    if (!out)
    {
        TC_LOG_ERROR("metric", "Metric daily summary: failed to write {}", path.string());
        return {};
    }
    return path;
}
}
