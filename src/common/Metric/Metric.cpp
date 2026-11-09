/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Metric.h"
#include "Config.h"
#include "DeadlineTimer.h"
#include "IoContext.h"
#include "Log.h"
#include "Util.h"
#include "MetricLocalStore.h"
#include "MetricHumanReport.h"
#include "MetricAggregator.h"
#include "MetricDailySummary.h"
#include "MetricErrorCollector.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <span>

void Metric::Initialize(std::string const& realmName, Trinity::Asio::IoContext& ioContext, std::function<void()> overallStatusLogger)
{
    _dataStream = std::make_unique<boost::asio::ip::tcp::iostream>();
    _realmName = FormatInfluxDBTagValue(realmName);
    _batchTimer = std::make_unique<Trinity::Asio::DeadlineTimer>(ioContext);
    _overallStatusTimer = std::make_unique<Trinity::Asio::DeadlineTimer>(ioContext);
    _localFlushTimer = std::make_unique<Trinity::Asio::DeadlineTimer>(ioContext);
    _reportTimer = std::make_unique<Trinity::Asio::DeadlineTimer>(ioContext);
    _overallStatusLogger = std::move(overallStatusLogger);
    _rateWindowStart = std::chrono::steady_clock::now();
    _humanReport.LoadDefaultCatalogThresholds();
    LoadFromConfigs();
}


bool Metric::Connect()
{
    GetDataStream().connect(_hostname, _port);
    if (boost::system::error_code const& error = GetDataStream().error())
    {
        TC_LOG_ERROR("metric", "Error connecting to '{}:{}', disabling Influx Metric sink. Error message : {}",
            _hostname, _port, error.message());
        _influxEnabled = false;
        _enabled = _localEnabled || _influxEnabled;
        return false;
    }
    GetDataStream().clear();
    return true;
}


void Metric::LoadFromConfigs()
{
    bool previousValue = _enabled;
    bool master = sConfigMgr->GetBoolDefault("Metric.Enable", false);
    std::string sink = sConfigMgr->GetStringDefault("Metric.Sink", "local");
    for (char& c : sink)
        c = char(std::tolower(static_cast<unsigned char>(c)));

    _localEnabled = false;
    _influxEnabled = false;
    if (master)
    {
        if (sink == "influx")
            _influxEnabled = true;
        else if (sink == "both")
        {
            _localEnabled = true;
            _influxEnabled = true;
        }
        else
            _localEnabled = true;
    }
    _enabled = _localEnabled || _influxEnabled;

    _updateInterval = sConfigMgr->GetIntDefault("Metric.Interval", 10);
    if (_updateInterval < 1)
    {
        TC_LOG_ERROR("metric", "'Metric.Interval' config set to {}, overriding to 1.", _updateInterval);
        _updateInterval = 1;
    }

    _overallStatusTimerInterval = sConfigMgr->GetIntDefault("Metric.OverallStatusInterval", 10);
    if (_overallStatusTimerInterval < 1)
    {
        TC_LOG_ERROR("metric", "'Metric.OverallStatusInterval' config set to {}, overriding to 1.", _overallStatusTimerInterval);
        _overallStatusTimerInterval = 1;
    }

    _localFlushSeconds = sConfigMgr->GetIntDefault("Metric.Local.FlushSeconds", 10);
    if (_localFlushSeconds < 1)
        _localFlushSeconds = 10;
    _reportIntervalSeconds = sConfigMgr->GetIntDefault("Metric.Local.ReportIntervalSeconds", 300);
    if (_reportIntervalSeconds < 30)
        _reportIntervalSeconds = 30;
    _worldTickBudgetMs = int32(sConfigMgr->GetIntDefault("Metric.Threshold.world_tick_ms", 100));
    if (_worldTickBudgetMs < 1)
        _worldTickBudgetMs = 100;

    Trinity::MetricLocal::StoreConfig storeCfg;
    storeCfg.Directory = sConfigMgr->GetStringDefault("Metric.Local.Directory", "Logs/metrics");
    storeCfg.BudgetBytes = uint32(sConfigMgr->GetIntDefault("Metric.Local.BudgetMB", 75)) * 1024u * 1024u;
    if (!storeCfg.BudgetBytes)
        storeCfg.BudgetBytes = 75u * 1024u * 1024u;
    storeCfg.RetentionDays = uint32(sConfigMgr->GetIntDefault("Metric.Local.RetentionDays", 14));
    storeCfg.CompressRotated = sConfigMgr->GetBoolDefault("Metric.Local.Compress", false);
    storeCfg.MaxSeries = size_t(std::max(1, sConfigMgr->GetIntDefault("Metric.Local.MaxSeries", 256)));
    _localStore.Configure(storeCfg);
    _localAggregator.SetMaxSeries(storeCfg.MaxSeries);
    _minuteRollup.SetMaxSeries(storeCfg.MaxSeries);
    _hourRollup.SetMaxSeries(storeCfg.MaxSeries);
    _dayRollup.SetMaxSeries(storeCfg.MaxSeries);

    bool dense = sConfigMgr->GetBoolDefault("Metric.DenseScrape", false);
    _denseScrape.store(dense, std::memory_order_relaxed);
    int samplePct = sConfigMgr->GetIntDefault("Metric.Dense.SamplePercent", 10);
    if (samplePct < 1) samplePct = 1;
    if (samplePct > 100) samplePct = 100;
    _denseSamplePercent.store(uint32(samplePct), std::memory_order_relaxed);
    if (dense)
        TC_LOG_WARN("metric", "Metric dense scrape ENABLED (sample={}%, maxSeries={}, budget={}MB)",
            samplePct, uint32(storeCfg.MaxSeries), storeCfg.BudgetBytes / (1024 * 1024));

    _thresholds.clear();
    _humanReport.LoadDefaultCatalogThresholds();
    std::vector<std::string> thresholdSettings = sConfigMgr->GetKeysByString("Metric.Threshold.");
    for (std::string& thresholdSetting : thresholdSettings)
    {
        int64 thresholdValue = sConfigMgr->GetIntDefault(thresholdSetting, 0);
        std::string thresholdName = thresholdSetting.substr(strlen("Metric.Threshold."));
        _thresholds.emplace(thresholdName, thresholdValue);
        Trinity::MetricLocal::ThresholdHint hint;
        hint.WarnAbove = double(thresholdValue);
        hint.Hint = "threshold exceeded - see observability catalog";
        _humanReport.SetThreshold(std::move(thresholdName), std::move(hint));
    }

    if (_enabled && !previousValue)
    {
        if (_influxEnabled)
        {
            std::string connectionInfo = sConfigMgr->GetStringDefault("Metric.ConnectionInfo", "");
            if (connectionInfo.empty())
            {
                TC_LOG_ERROR("metric", "'Metric.ConnectionInfo' not specified; disabling Influx sink.");
                _influxEnabled = false;
            }
            else
            {
                std::vector<std::string_view> tokens = Trinity::Tokenize(connectionInfo, ';', true);
                if (tokens.size() != 3)
                {
                    TC_LOG_ERROR("metric", "'Metric.ConnectionInfo' wrong format; disabling Influx sink.");
                    _influxEnabled = false;
                }
                else
                {
                    _hostname.assign(tokens[0]);
                    _port.assign(tokens[1]);
                    _databaseName.assign(tokens[2]);
                    Connect();
                }
            }
        }

        _enabled = _localEnabled || _influxEnabled;
        if (!_enabled)
            return;

        if (_influxEnabled)
            ScheduleSend();
        if (_localEnabled)
        {
            std::error_code absEc;
            auto absDir = std::filesystem::absolute(storeCfg.Directory, absEc);
            TC_LOG_WARN("metric", "Metric local sink enabled (dir='{}', abs='{}', budget={} MB, maxSeries={})",
                storeCfg.Directory.string(), absEc ? storeCfg.Directory.string() : absDir.string(),
                storeCfg.BudgetBytes / (1024 * 1024), uint32(storeCfg.MaxSeries));
            FlushLocalWindows(); // schedules itself
        }
        ScheduleOverallStatusLog();
    }
    else if (!_enabled && previousValue)
    {
        if (_batchTimer)
            _batchTimer->cancel();
        if (_localFlushTimer)
            _localFlushTimer->cancel();
        if (_reportTimer)
            _reportTimer->cancel();
        if (_overallStatusTimer)
            _overallStatusTimer->cancel();
    }
}


void Metric::RecordLocalValue(std::string_view category, double value, MetricTags const& tags)
{
    auto spanTags = [](MetricTags const& t) -> std::span<MetricTag const>
    {
        switch (t.index())
        {
            case 1: return std::get<1>(t);
            case 2: return std::get<2>(t);
            default: break;
        }
        return {};
    }(tags);

    std::vector<std::pair<std::string_view, std::string_view>> pairs;
    // Cardinality policy: keep only low-risk labels (pool/type), drop map_id / account_id / map_instanceid.
    for (auto const& [k, v] : spanTags)
    {
        if (k == "map_id" || k == "map_instanceid" || k == "account_id")
            continue;
        pairs.emplace_back(k, v);
    }
    std::string labels = Trinity::MetricLocal::Aggregator::CanonicalLabels(pairs);
    _localAggregator.Record(category, value, labels);
}


void Metric::WriteHumanReport(std::vector<Trinity::MetricLocal::SeriesSnapshot> const& series)
{
    std::string report = _humanReport.Build(series, 10);
    TC_LOG_INFO("metric", "{}", report);

    std::filesystem::path path = _localStore.GetConfig().Directory / "latest_report.txt";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out)
        out << report;
}


void Metric::FlushLocalWindows()
{
    if (!_localEnabled)
        return;

    using namespace Trinity::MetricLocal;
    auto now = std::chrono::system_clock::now();

    // packet rates since last flush
    uint64 inPkts = _packetsIn.exchange(0);
    uint64 outPkts = _packetsOut.exchange(0);
    _localAggregator.Record("pkt_in_packets", double(inPkts));
    _localAggregator.Record("pkt_out_packets", double(outPkts));

    std::vector<SeriesSnapshot> tenSec = _localAggregator.Flush();
    _localStore.WriteWindow(Resolution::TenSeconds, now, tenSec);
    _minuteRollup.Merge(tenSec);
    ++_tenSecFlushes;

    bool wroteMinute = false;
    if (_tenSecFlushes >= 6) // ~60s if flush=10s
    {
        _tenSecFlushes = 0;
        std::vector<SeriesSnapshot> minute = _minuteRollup.Flush();
        _localStore.WriteWindow(Resolution::OneMinute, now, minute);
        _hourRollup.Merge(minute);
        ++_minuteFlushes;
        wroteMinute = true;
        WriteHumanReport(minute);
    }

    if (_minuteFlushes >= 60)
    {
        _minuteFlushes = 0;
        std::vector<SeriesSnapshot> hour = _hourRollup.Flush();
        _localStore.WriteWindow(Resolution::OneHour, now, hour);
        _dayRollup.Merge(hour);
        ++_hourFlushes;
    }

    if (_hourFlushes >= 24)
    {
        _hourFlushes = 0;
        std::vector<SeriesSnapshot> day = _dayRollup.Flush();
        // Compact machine window (optional rollup artifact).
        _localStore.WriteWindow(Resolution::OneDay, now, day);
        // ONE human-readable daily file: avg/p95/max + error/warn summary (counts toward budget).
        DailySummary daily;
        daily.DayUtc = now;
        daily.Metrics = day;
        daily.Errors = ErrorWarningCollector::Instance().Flush();
        auto dailyPath = DailySummaryWriter::Write(_localStore.GetConfig().Directory, daily);
        if (!dailyPath.empty())
        {
            TC_LOG_INFO("metric", "Metric daily summary written: {}", dailyPath.string());
            _localStore.EnforceBudgetAndRetention();
        }
    }

    (void)wroteMinute;

    if (_localFlushTimer)
    {
        _localFlushTimer->expires_after(std::chrono::seconds(_localFlushSeconds));
        _localFlushTimer->async_wait([this](boost::system::error_code const& ec)
        {
            if (!ec)
                FlushLocalWindows();
        });
    }
}


uint64 Metric::GetLocalStoreBytes() const
{
    return _localStore.DirectoryBytes();
}

void Metric::RecordDense(std::string_view category, double valueMs, std::string_view labelKey, std::string_view labelValue)
{
    if (!_denseScrape.load(std::memory_order_relaxed) || !_localEnabled)
        return;
    uint32 pct = _denseSamplePercent.load(std::memory_order_relaxed);
    uint64 cursor = _denseSampleCursor.fetch_add(1, std::memory_order_relaxed);
    if (pct < 100 && (cursor % 100) >= pct)
        return;

    auto t0 = std::chrono::steady_clock::now();
    if (labelKey.empty())
        LogValue(category, valueMs);
    else
        LogValue(category, valueMs, MetricTag{ std::string(labelKey), std::string(labelValue) });
    auto ns = uint64(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
    _denseCpuNs.fetch_add(ns, std::memory_order_relaxed);
    _denseSamples.fetch_add(1, std::memory_order_relaxed);
}



void Metric::Update()
{
    if (_overallStatusTimerTriggered)
    {
        _overallStatusTimerTriggered = false;
        _overallStatusLogger();
    }
}

bool Metric::ShouldLog(std::string_view category, int64 value) const
{
    auto threshold = _thresholds.find(category);
    if (threshold == _thresholds.end())
        return false;
    return value >= threshold->second;
}

void Metric::LogEvent(std::string_view category, std::string_view title, std::string description)
{
    if (_localEnabled)
        _localAggregator.Record(category, 1.0, std::string(title));

    if (!_influxEnabled)
        return;

    MetricData* data = new MetricData
    {
        .Category = std::string(category),
        .Timestamp = std::chrono::system_clock::now(),
        .Type = METRIC_DATA_EVENT,
        .Title = std::string(title),
        .ValueOrEventText = std::move(description)
    };

    _queuedData.Enqueue(data);
}

void Metric::SendBatch()
{
    using namespace std::chrono;

    std::stringstream batchedData;
    MetricData* data;
    bool firstLoop = true;
    while (_queuedData.Dequeue(data))
    {
        if (!firstLoop)
            batchedData << "\n";

        batchedData << data->Category;
        if (!_realmName.empty())
            batchedData << ",realm=" << _realmName;

        auto tags = [](MetricTags const& tags) -> std::span<MetricTag const>
        {
            switch (tags.index())
            {
                case 1: return std::get<1>(tags);
                case 2: return std::get<2>(tags);
                default:
                    break;
            }
            return {};
        }(data->Tags);

        for (auto const& [tagName, tagValue] : tags)
            if (!tagName.empty())
                batchedData << "," << tagName << "=" << FormatInfluxDBTagValue(tagValue);

        batchedData << " ";

        switch (data->Type)
        {
            case METRIC_DATA_VALUE:
                batchedData << "value=" << data->ValueOrEventText;
                break;
            case METRIC_DATA_EVENT:
                batchedData << "title=\"" << data->Title << "\",text=\"" << data->ValueOrEventText << "\"";
                break;
        }

        batchedData << " ";

        batchedData << duration_cast<nanoseconds>(data->Timestamp.time_since_epoch()).count();

        firstLoop = false;
        delete data;
    }

    // Check if there's any data to send
    std::streamoff batchedDataSize = batchedData.tellp();
    if (batchedDataSize <= 0)
    {
        ScheduleSend();
        return;
    }

    if (!GetDataStream().good() && !Connect())
        return;

    GetDataStream() << "POST " << "/write?db=" << _databaseName << " HTTP/1.1\r\n";
    GetDataStream() << "Host: " << _hostname << ":" << _port << "\r\n";
    GetDataStream() << "Accept: */*\r\n";
    GetDataStream() << "Content-Type: application/octet-stream\r\n";
    GetDataStream() << "Content-Transfer-Encoding: binary\r\n";

    GetDataStream() << "Content-Length: " << batchedDataSize << "\r\n\r\n";
    GetDataStream() << batchedData.rdbuf();

    std::string http_version;
    GetDataStream() >> http_version;
    unsigned int status_code = 0;
    GetDataStream() >> status_code;
    if (status_code != 204)
    {
        TC_LOG_ERROR("metric", "Error sending data, returned HTTP code: {}", status_code);
    }

    // Read and ignore the status description
    std::string status_description;
    std::getline(GetDataStream(), status_description);
    // Read headers
    std::string header;
    while (std::getline(GetDataStream(), header) && header != "\r")
        if (header == "Connection: close\r")
            GetDataStream().close();

    ScheduleSend();
}

void Metric::ScheduleSend()
{
    if (_influxEnabled)
    {
        _batchTimer->expires_after(std::chrono::seconds(_updateInterval));
        _batchTimer->async_wait([this](boost::system::error_code const&){ SendBatch(); });
    }
    else
    {
        if (_dataStream)
            GetDataStream().close();
        MetricData* data;
        while (_queuedData.Dequeue(data))
            delete data;
    }
}

void Metric::Unload()
{
    if (_localEnabled)
    {
        FlushLocalWindows();
        _localEnabled = false;
    }

    if (_enabled && _influxEnabled && Trinity::Asio::get_io_context(*_batchTimer).stopped())
    {
        _influxEnabled = false;
        _enabled = false;
        SendBatch();
    }

    _enabled = false;
    if (_batchTimer)
        _batchTimer->cancel();
    if (_overallStatusTimer)
        _overallStatusTimer->cancel();
    if (_localFlushTimer)
        _localFlushTimer->cancel();
    if (_reportTimer)
        _reportTimer->cancel();
}
void Metric::ScheduleOverallStatusLog()
{
    if (_enabled)
    {
        _overallStatusTimer->expires_after(std::chrono::seconds(_overallStatusTimerInterval));
        _overallStatusTimer->async_wait([this](const boost::system::error_code&)
        {
            _overallStatusTimerTriggered = true;
            ScheduleOverallStatusLog();
        });
    }
}

std::string Metric::FormatInfluxDBValue(bool value)
{
    return std::string(1, value ? 't' : 'f');
}

template<class T> requires std::integral<T> && (!std::same_as<T, bool>)
std::string Metric::FormatInfluxDBValue(T value)
{
    std::string result = std::to_string(value);
    result += 'i';
    return result;
}

std::string Metric::FormatInfluxDBValue(std::string const& value)
{
    std::string result = StringReplaceAll(value, "\"", "\\\"");
    result.insert(result.begin(), '"');
    result.append(1, '"');
    return result;
}

std::string Metric::FormatInfluxDBValue(char const* value)
{
    return FormatInfluxDBValue(std::string(value));
}

std::string Metric::FormatInfluxDBValue(double value)
{
    return std::to_string(value);
}

std::string Metric::FormatInfluxDBValue(float value)
{
    return FormatInfluxDBValue(double(value));
}

std::string Metric::FormatInfluxDBTagValue(std::string const& value)
{
    // ToDo: should handle '=' and ',' characters too
    return StringReplaceAll(value, " ", "\\ ");
}

std::string Metric::FormatInfluxDBValue(std::chrono::nanoseconds value)
{
    return FormatInfluxDBValue(std::chrono::duration_cast<Milliseconds>(value).count());
}

Metric::Metric()
{
}

Metric::~Metric()
{
}

Metric* Metric::instance()
{
    static Metric instance;
    return &instance;
}

template TC_COMMON_API std::string Metric::FormatInfluxDBValue(int8);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(uint8);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(int16);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(uint16);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(int32);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(uint32);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(int64);
template TC_COMMON_API std::string Metric::FormatInfluxDBValue(uint64);
