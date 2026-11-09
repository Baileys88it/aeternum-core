#include "AdminAuditLog.h"
#include "Log.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace AdminMonitoring
{
void AuditLog::Configure(std::string path, uint64_t rotateBytes, uint32_t keepRotated)
{
    std::lock_guard<std::mutex> g(_mutex);
    _path = std::move(path);
    _rotateBytes = rotateBytes ? rotateBytes : (2ull * 1024ull * 1024ull);
    _keepRotated = keepRotated ? keepRotated : 1;
}

void AuditLog::RotateIfNeededUnlocked()
{
    if (_path.empty())
        return;
    std::error_code ec;
    if (!std::filesystem::exists(_path, ec))
        return;
    auto sz = std::filesystem::file_size(_path, ec);
    if (ec || sz < _rotateBytes)
        return;

    // Rotate: log -> .1 -> .2 ... ; drop beyond keep
    std::filesystem::path base(_path);
    std::filesystem::remove(base.string() + "." + std::to_string(_keepRotated), ec);
    for (uint32_t i = _keepRotated; i >= 1; --i)
    {
        std::filesystem::path src = (i == 1) ? base : std::filesystem::path(base.string() + "." + std::to_string(i - 1));
        std::filesystem::path dst(base.string() + "." + std::to_string(i));
        if (std::filesystem::exists(src, ec))
            std::filesystem::rename(src, dst, ec);
        if (i == 1)
            break; // uint wrap guard
    }
}

void AuditLog::Write(AuditEvent const& ev)
{
    std::lock_guard<std::mutex> g(_mutex);
    if (_path.empty())
        return;
    RotateIfNeededUnlocked();

    auto const now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream line;
    line << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ")
         << " who=" << (ev.Who.empty() ? "-" : ev.Who)
         << " what=" << (ev.What.empty() ? "-" : ev.What)
         << " result=" << (ev.Result.empty() ? "-" : ev.Result)
         << " status=" << ev.Status
         << " role=" << (ev.Role.empty() ? "-" : ev.Role)
         << " duration_ms=" << ev.DurationMs;
    if (!ev.Detail.empty())
        line << " detail=" << ev.Detail;
    line << '\n';

    std::ofstream out(_path, std::ios::app | std::ios::binary);
    if (!out)
    {
        TC_LOG_ERROR("module.admin", "Admin audit log open failed (path configured; token never logged)");
        return;
    }
    out << line.str();
}

void AuditLog::Write(std::string_view method, std::string_view path, int status, std::string_view role, std::string_view detail)
{
    std::string what = std::string(method) + " " + std::string(path);
    char const* result = "ok";
    if (status == 401 || status == 403)
        result = "denied";
    else if (status == 429)
        result = "rate_limited";
    else if (status >= 400)
        result = "error";
    Write(AuditEvent{ "-", what, result, status, role, detail, 0 });
}
}
