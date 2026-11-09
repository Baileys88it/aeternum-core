#include "CrashDumpBudget.h"
#include "Log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <vector>

#ifdef _WIN32
# include <Windows.h>
#endif

namespace Trinity::Crash
{
namespace
{
DumpStore g_store;
}

DumpStore& GetDumpStore() { return g_store; }

std::filesystem::path DumpStore::DefaultDirectory()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH)
    {
        std::filesystem::path p(buf);
        return p.parent_path() / "Crashes";
    }
#endif
    return std::filesystem::path("Crashes");
}

void DumpStore::Configure(DumpStoreConfig cfg)
{
    if (cfg.Directory.empty())
        cfg.Directory = DefaultDirectory();
    if (!cfg.BudgetBytes)
        cfg.BudgetBytes = 256ull * 1024ull * 1024ull;
    if (!cfg.RetentionDays)
        cfg.RetentionDays = 30;
    _cfg = std::move(cfg);
    std::error_code ec;
    std::filesystem::create_directories(_cfg.Directory, ec);
}

uint64 DumpStore::DirectoryBytes() const
{
    if (_cfg.Directory.empty())
        return 0;
    uint64 total = 0;
    std::error_code ec;
    if (!std::filesystem::exists(_cfg.Directory, ec))
        return 0;
    for (auto const& ent : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (ec) break;
        if (ent.is_regular_file(ec))
            total += uint64(ent.file_size(ec));
    }
    return total;
}

uint32 DumpStore::FileCount() const
{
    if (_cfg.Directory.empty())
        return 0;
    uint32 n = 0;
    std::error_code ec;
    if (!std::filesystem::exists(_cfg.Directory, ec))
        return 0;
    for (auto const& ent : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (ec) break;
        if (ent.is_regular_file(ec))
            ++n;
    }
    return n;
}

uint32 DumpStore::EnforceBudgetAndRetention()
{
    if (_cfg.Directory.empty())
        return 0;
    std::error_code ec;
    if (!std::filesystem::exists(_cfg.Directory, ec))
        return 0;

    struct Entry { std::filesystem::path path; uint64 size; std::filesystem::file_time_type mtime; };
    std::vector<Entry> files;
    for (auto const& ent : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (ec) break;
        if (!ent.is_regular_file(ec))
            continue;
        Entry e;
        e.path = ent.path();
        e.size = uint64(ent.file_size(ec));
        e.mtime = ent.last_write_time(ec);
        files.push_back(std::move(e));
    }

    uint32 deleted = 0;
    auto now = std::filesystem::file_time_type::clock::now();
    auto maxAge = std::chrono::hours(24) * _cfg.RetentionDays;
    for (auto const& f : files)
    {
        if (now - f.mtime > maxAge)
        {
            std::filesystem::remove(f.path, ec);
            if (!ec) ++deleted;
        }
    }

    // refresh
    files.clear();
    uint64 total = 0;
    for (auto const& ent : std::filesystem::directory_iterator(_cfg.Directory, ec))
    {
        if (ec) break;
        if (!ent.is_regular_file(ec))
            continue;
        Entry e;
        e.path = ent.path();
        e.size = uint64(ent.file_size(ec));
        e.mtime = ent.last_write_time(ec);
        total += e.size;
        files.push_back(std::move(e));
    }
    std::sort(files.begin(), files.end(), [](Entry const& a, Entry const& b) { return a.mtime < b.mtime; });
    for (auto const& f : files)
    {
        if (total <= _cfg.BudgetBytes)
            break;
        std::filesystem::remove(f.path, ec);
        if (!ec)
        {
            total = total > f.size ? total - f.size : 0;
            ++deleted;
        }
    }
    return deleted;
}

bool DumpStore::WriteTestMarker(std::string_view tag)
{
    if (_cfg.Directory.empty())
        Configure({});
    std::error_code ec;
    std::filesystem::create_directories(_cfg.Directory, ec);
    auto path = _cfg.Directory / ("test_marker_" + std::string(tag) + ".marker");
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;
    out << "aeternum-crash-test-marker\n" << tag << "\n";
    out.close();
    EnforceBudgetAndRetention();
    return true;
}
}
