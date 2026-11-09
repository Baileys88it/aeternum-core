/*
 * Aeternum Admin & Monitoring — Prometheus text exposition helpers (header-only).
 */
#ifndef AETERNUM_ADMIN_PROMETHEUS_H
#define AETERNUM_ADMIN_PROMETHEUS_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace AdminMonitoring
{
inline std::string SanitizeMetricName(std::string_view name)
{
    std::string out;
    out.reserve(name.size() + 16);
    for (char ch : name)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
            out.push_back(ch);
        else
            out.push_back('_');
    }
    if (out.empty() || (out[0] >= '0' && out[0] <= '9'))
        out.insert(out.begin(), 'm');
    return out;
}

inline std::string EscapeLabelValue(std::string_view v)
{
    std::string out;
    out.reserve(v.size());
    for (char ch : v)
    {
        if (ch == '\\' || ch == '"')
            out.push_back('\\');
        if (ch == '\n')
        {
            out += "\\n";
            continue;
        }
        out.push_back(ch);
    }
    return out;
}

inline std::string FormatGauge(std::string_view name, double value, std::string_view help = {})
{
    std::string metric = SanitizeMetricName(name);
    std::string out;
    if (!help.empty())
    {
        out += "# HELP ";
        out += metric;
        out += ' ';
        out += help;
        out += '\n';
    }
    out += "# TYPE ";
    out += metric;
    out += " gauge\n";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6g", value);
    out += metric;
    out += ' ';
    out += buf;
    out += '\n';
    return out;
}

struct PromSeries
{
    std::string Name;
    std::string Labels;
    uint64_t Count = 0;
    double Sum = 0;
    double Min = 0;
    double Max = 0;
};

inline std::string FormatSeriesAsPrometheus(std::vector<PromSeries> const& series)
{
    std::string out;
    out.reserve(series.size() * 96);
    for (PromSeries const& s : series)
    {
        std::string base = SanitizeMetricName(s.Name);
        std::string labels;
        if (!s.Labels.empty())
            labels = "{" + s.Labels + "}";
        char buf[96];
        auto line = [&](std::string const& suffix, double v)
        {
            out += base;
            out += suffix;
            out += labels;
            out += ' ';
            std::snprintf(buf, sizeof(buf), "%.6g", v);
            out += buf;
            out += '\n';
        };
        line("_count", static_cast<double>(s.Count));
        line("_sum", s.Sum);
        line("_min", s.Min);
        line("_max", s.Max);
    }
    return out;
}
}
#endif
