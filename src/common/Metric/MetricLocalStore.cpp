#include "MetricLocalStore.h"
#include "Log.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <zlib.h>

namespace Trinity::MetricLocal
{
namespace
{
char const* ResPrefix(Resolution res)
{
    switch (res)
    {
        case Resolution::TenSeconds: return "10s";
        case Resolution::OneMinute:  return "1m";
        case Resolution::OneHour:    return "1h";
        case Resolution::OneDay:     return "1d";
    }
    return "unk";
}

// Bundled Windows zlib omits gz*.c; use raw deflate with gzip wrapper (windowBits=31).
bool WriteGzipFile(std::filesystem::path const& path, std::string const& payload)
{
    z_stream strm{};
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return false;

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(payload.data()));
    strm.avail_in = uInt(payload.size());

    std::string out;
    out.resize(deflateBound(&strm, uLong(payload.size())));
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = uInt(out.size());

    int rc = deflate(&strm, Z_FINISH);
    uLong produced = strm.total_out;
    deflateEnd(&strm);
    if (rc != Z_STREAM_END)
        return false;
    out.resize(produced);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return false;
    file.write(out.data(), std::streamsize(out.size()));
    return bool(file);
}
}

void LocalStore::Configure(StoreConfig cfg)
{
    if (!cfg.BudgetBytes)
        cfg.BudgetBytes = 75u * 1024u * 1024u;
    if (!cfg.RetentionDays)
        cfg.RetentionDays = 1;
    if (!cfg.MaxSeries)
        cfg.MaxSeries = 256;
    _cfg = std::move(cfg);
    std::error_code ec;
    std::filesystem::create_directories(_cfg.Directory, ec);
}

std::string LocalStore::FileNameFor(Resolution res, std::chrono::system_clock::time_point ts)
{
    std::time_t t = std::chrono::system_clock::to_time_t(ts);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << ResPrefix(res) << '_'
        << std::put_time(&tm, "%Y%m%dT%H%M%SZ")
        << ".mlines";
    return oss.str();
}

std::string LocalStore::FormatLine(SeriesSnapshot const& snap)
{
    // Compact CSV-ish: name|labels|count|sum|min|max
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << snap.Key.Name << '|'
        << snap.Key.Labels << '|'
        << snap.Stats.Count << '|'
        << snap.Stats.Sum << '|'
        << snap.Stats.Min << '|'
        << snap.Stats.Max;
    return oss.str();
}

bool LocalStore::ParseLine(std::string_view line, SeriesSnapshot& out)
{
    if (line.empty() || line[0] == '#')
        return false;
    std::string tmp(line);
    std::vector<std::string> parts;
    {
        std::stringstream ss(tmp);
        std::string item;
        while (std::getline(ss, item, '|'))
            parts.push_back(item);
    }
    if (parts.size() != 6)
        return false;
    try
    {
        out.Key.Name = parts[0];
        out.Key.Labels = parts[1];
        out.Stats.Count = std::stoull(parts[2]);
        out.Stats.Sum = std::stod(parts[3]);
        out.Stats.Min = std::stod(parts[4]);
        out.Stats.Max = std::stod(parts[5]);
    }
    catch (...)
    {
        return false;
    }
    return !out.Key.Name.empty() && out.Stats.Count > 0;
}

std::vector<SeriesSnapshot> LocalStore::LoadFile(std::filesystem::path const& path)
{
    std::vector<SeriesSnapshot> out;
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return out;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        SeriesSnapshot snap;
        if (ParseLine(line, snap))
            out.push_back(std::move(snap));
    }
    return out;
}

bool LocalStore::WriteWindow(Resolution res, std::chrono::system_clock::time_point ts,
    std::vector<SeriesSnapshot> const& series)
{
    if (_cfg.Directory.empty())
        return false;

    std::error_code ec;
    std::filesystem::create_directories(_cfg.Directory, ec);

    std::ostringstream body;
    body << "# aeternum-metric-v1 res=" << ResPrefix(res) << " series=" << series.size() << '\n';
    for (SeriesSnapshot const& snap : series)
        body << FormatLine(snap) << '\n';

    std::string name = FileNameFor(res, ts);
    std::filesystem::path path = _cfg.Directory / name;

    if (_cfg.CompressRotated && (res == Resolution::OneHour || res == Resolution::OneDay))
    {
        path += ".gz";
        if (!WriteGzipFile(path, body.str()))
        {
            TC_LOG_ERROR("metric", "Metric local store: failed to write {}", path.string());
            return false;
        }
    }
    else
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            TC_LOG_ERROR("metric", "Metric local store: failed to open {}", path.string());
            return false;
        }
        out << body.str();
    }

    EnforceBudgetAndRetention();
    return true;
}

uint64 LocalStore::DirectoryBytes() const
{
    if (_cfg.Directory.empty() || !std::filesystem::exists(_cfg.Directory))
        return 0;
    uint64 total = 0;
    std::error_code ec;
    for (auto const& entry : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (!entry.is_regular_file(ec))
            continue;
        total += uint64(entry.file_size(ec));
    }
    return total;
}


uint32 LocalStore::FileCount() const
{
    if (_cfg.Directory.empty())
        return 0;
    uint32 n = 0;
    std::error_code ec;
    if (!std::filesystem::exists(_cfg.Directory, ec))
        return 0;
    for (auto const& ent : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (ec)
            break;
        if (ent.is_regular_file(ec))
            ++n;
    }
    return n;
}
uint32 LocalStore::EnforceBudgetAndRetention()
{
    if (_cfg.Directory.empty() || !std::filesystem::exists(_cfg.Directory))
        return 0;

    std::error_code ec;
    using FileClock = std::filesystem::file_time_type::clock;
    auto now = FileClock::now();
    auto retention = std::chrono::duration_cast<FileClock::duration>(std::chrono::hours(24) * _cfg.RetentionDays);

    struct FileInfo
    {
        std::filesystem::path Path;
        uint64 Size = 0;
        std::filesystem::file_time_type Mtime{};
    };
    std::vector<FileInfo> files;
    for (auto const& entry : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (!entry.is_regular_file(ec))
            continue;
        FileInfo fi;
        fi.Path = entry.path();
        fi.Size = uint64(entry.file_size(ec));
        fi.Mtime = entry.last_write_time(ec);
        files.push_back(std::move(fi));
    }

    uint32 removed = 0;
    // Retention: remove old files first
    for (auto it = files.begin(); it != files.end(); )
    {
        if (now - it->Mtime > retention)
        {
            std::filesystem::remove(it->Path, ec);
            ++removed;
            it = files.erase(it);
        }
        else
            ++it;
    }

    uint64 total = 0;
    for (auto const& f : files)
        total += f.Size;

    if (total <= _cfg.BudgetBytes)
        return removed;

    std::sort(files.begin(), files.end(),
        [](FileInfo const& a, FileInfo const& b) { return a.Mtime < b.Mtime; });

    for (FileInfo const& f : files)
    {
        if (total <= _cfg.BudgetBytes)
            break;
        std::filesystem::remove(f.Path, ec);
        if (!ec)
        {
            total -= f.Size;
            ++removed;
        }
    }
    return removed;
}
}
