#include "MetricHumanReport.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Trinity::MetricLocal
{
void HumanReport::SetThreshold(std::string name, ThresholdHint hint)
{
    _thresholds[std::move(name)] = std::move(hint);
}

void HumanReport::LoadDefaultCatalogThresholds()
{
    // Starting guesses from OBSERVABILITY catalog (M2/M3).
    SetThreshold("world_tick_ms", { 100.0, "World loop / scripts / map backlog - profile Update maps & sessions" });
    SetThreshold("world_update_time_total", { 100.0, "Same as world tick - reduce work in the heaviest Update phase" });
    SetThreshold("update_time_diff", { 200.0, "Tick budget slip - server overloaded or stalled" });
    SetThreshold("world_tick_overrun_total", { 1.0, "Sustained overruns - find top world_update_time types" });
    SetThreshold("map_updater_queue_wait_ms", { 50.0, "MapUpdater threads vs map load; lock contention" });
    SetThreshold("map_update_time_diff", { 50.0, "Heavy creatures/scripts on that map_id" });
    SetThreshold("db_queue_login", { 50.0, "Login DB pool saturation - slow SQL / pool size" });
    SetThreshold("db_queue_character", { 50.0, "Character DB pool saturation" });
    SetThreshold("db_queue_world", { 50.0, "World DB pool saturation" });
    SetThreshold("db_queue_hotfixes", { 50.0, "Hotfix DB pool saturation" });
    SetThreshold("sessions_active", { 5000.0, "Unexpected pop cliff or connection storm" });
    SetThreshold("pkt_in_packets", { 200000.0, "Inbound flood - check throttles / abuse" });
    SetThreshold("pkt_out_packets", { 400000.0, "Outbound broadcast storms" });
}

std::string HumanReport::Build(std::vector<SeriesSnapshot> const& series, std::size_t topN) const
{
    std::ostringstream oss;
    oss << "Aeternum metrics report (human v0)\n";
    oss << "=================================\n";
    if (series.empty())
    {
        oss << "(no samples in window)\n";
        return oss.str();
    }

    std::vector<SeriesSnapshot> sorted = series;
    std::sort(sorted.begin(), sorted.end(),
        [](SeriesSnapshot const& a, SeriesSnapshot const& b)
        {
            double aa = std::fabs(a.Stats.Max);
            double bb = std::fabs(b.Stats.Max);
            if (aa != bb)
                return aa > bb;
            return a.Stats.Count > b.Stats.Count;
        });

    oss << "Top-" << topN << " hotspots (by max):\n";
    std::size_t shown = 0;
    for (SeriesSnapshot const& s : sorted)
    {
        if (shown >= topN)
            break;
        oss << "  - " << s.Key.Name;
        if (!s.Key.Labels.empty())
            oss << '{' << s.Key.Labels << '}';
        oss << " count=" << s.Stats.Count
            << " avg=" << s.Stats.Avg()
            << " max=" << s.Stats.Max
            << '\n';
        ++shown;
    }

    oss << "\nWhere to improve:\n";
    bool anyHint = false;
    for (SeriesSnapshot const& s : sorted)
    {
        auto it = _thresholds.find(s.Key.Name);
        if (it == _thresholds.end())
            continue;
        if (s.Stats.Max < it->second.WarnAbove && s.Stats.Avg() < it->second.WarnAbove)
            continue;
        oss << "  * " << s.Key.Name << ": " << it->second.Hint
            << " (max=" << s.Stats.Max << " warn>=" << it->second.WarnAbove << ")\n";
        anyHint = true;
    }
    if (!anyHint)
        oss << "  (all catalog thresholds quiet)\n";

    return oss.str();
}
}
