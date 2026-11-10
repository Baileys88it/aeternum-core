#include "AdminHttpServer.h"
#include "AdminTokenResolve.h"
#include "AdminAuth.h"
#include "AdminBind.h"
#include "AdminPrometheus.h"
#include "Log.h"
#include "aeternum/sdk/registry.hpp"
#include "aeternum/api/ops/services.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace AdminMonitoring
{
namespace
{
std::string ReadFileLimited(std::filesystem::path const& path, std::size_t maxBytes = 2 * 1024 * 1024)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return {};
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.size() > maxBytes)
        data.resize(maxBytes);
    return data;
}

std::filesystem::path NewestMlines(std::filesystem::path const& dir, bool preferTenSec)
{
    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};
    bool have = false;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec))
        return {};
    char const* prefix = preferTenSec ? "10s_" : "1m_";
    for (auto const& ent : std::filesystem::directory_iterator(dir, ec))
    {
        if (!ent.is_regular_file())
            continue;
        auto name = ent.path().filename().string();
        if (name.rfind(prefix, 0) != 0 || ent.path().extension() != ".mlines")
            continue;
        auto ft = ent.last_write_time(ec);
        if (!have || ft > bestTime)
        {
            best = ent.path();
            bestTime = ft;
            have = true;
        }
    }
    if (!have && preferTenSec)
        return NewestMlines(dir, false);
    return best;
}

std::vector<PromSeries> SnapshotsToProm(std::vector<aeternum::ops::MetricSeriesSnapshot> const& snaps)
{
    std::vector<PromSeries> series;
    series.reserve(snaps.size());
    for (auto const& s : snaps)
    {
        PromSeries ps;
        ps.Name = s.name;
        if (!s.labels.empty())
            ps.Labels = "labels=\"" + EscapeLabelValue(s.labels) + "\"";
        ps.Count = s.count;
        ps.Sum = s.sum;
        ps.Min = s.min;
        ps.Max = s.max;
        series.push_back(std::move(ps));
    }
    return series;
}

std::string BuildPrometheus(ServerConfig const& cfg, bool dense)
{
    std::string out;
    out += FormatGauge("aeternum_uptime_seconds", double((aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->UptimeSeconds() : 0)), "Seconds since process start");
    out += FormatGauge("aeternum_sessions_active", double((aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->ActiveSessionCount() : 0)), "Active world sessions");
    out += FormatGauge("aeternum_sessions_queued", double((aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->QueuedSessionCount() : 0)), "Queued sessions");
    out += FormatGauge("aeternum_players_online", double((aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->PlayerCount() : 0)), "Players in world");
    if (aeternum::ops::Services().metrics && aeternum::ops::Services().metrics->LocalSinkEnabled())
    {
        out += FormatGauge("aeternum_metric_store_bytes", double(aeternum::ops::Services().metrics->LocalStoreBytes()), "Local metric store bytes");
        out += FormatGauge("aeternum_metric_budget_bytes", double(aeternum::ops::Services().metrics->LocalBudgetBytes()), "Configured metric disk budget");
        out += FormatGauge("aeternum_metric_dense_enabled", aeternum::ops::Services().metrics->DenseScrapeEnabled() ? 1.0 : 0.0, "Dense scrape instrumentation armed");
        out += FormatGauge("aeternum_metric_dense_samples_total", double(aeternum::ops::Services().metrics->DenseSampleCount()), "Dense samples recorded");
        out += FormatGauge("aeternum_metric_dense_cpu_ns_total", double(aeternum::ops::Services().metrics->DenseCpuNanos()), "Approx CPU ns spent in dense record path");
    }

    std::filesystem::path dir = cfg.MetricsDirectory.empty()
        ? std::filesystem::path("Logs/metrics")
        : std::filesystem::path(cfg.MetricsDirectory);
    auto file = NewestMlines(dir, dense || cfg.DenseScrapeDefault);
    if (!file.empty())
    {
        std::size_t topN = (dense || cfg.DenseScrapeDefault) ? (cfg.MetricsTopN ? cfg.MetricsTopN : 20) : 0;
        auto snaps = aeternum::ops::Services().metrics
            ? aeternum::ops::Services().metrics->LoadSeriesFile(file.generic_string(), topN)
            : std::vector<aeternum::ops::MetricSeriesSnapshot>{};
        out += FormatSeriesAsPrometheus(SnapshotsToProm(snaps));
    }
    return out;
}

std::string BuildStoreJson()
{
    if (!aeternum::ops::Services().metrics)
        return "{\"error\":\"metrics_unavailable\"}";
    std::ostringstream oss;
    auto dir = aeternum::ops::Services().metrics ? std::filesystem::path(aeternum::ops::Services().metrics->LocalDirectory()) : std::filesystem::path{};
    std::error_code absEc;
    auto absDir = std::filesystem::absolute(dir, absEc);
    oss << "{\"bytes\":" << aeternum::ops::Services().metrics->LocalStoreBytes()
        << ",\"budget_bytes\":" << aeternum::ops::Services().metrics->LocalBudgetBytes()
        << ",\"retention_days\":" << aeternum::ops::Services().metrics->LocalRetentionDays()
        << ",\"max_series\":" << aeternum::ops::Services().metrics->LocalMaxSeries()
        << ",\"file_count\":" << aeternum::ops::Services().metrics->LocalFileCount()
        << ",\"directory\":\"" << dir.generic_string() << "\""
        << ",\"directory_abs\":\"" << (absEc ? dir.generic_string() : absDir.generic_string()) << "\""
        << ",\"dense\":" << (aeternum::ops::Services().metrics->DenseScrapeEnabled() ? "true" : "false")
        << ",\"dense_samples\":" << aeternum::ops::Services().metrics->DenseSampleCount()
        << ",\"dense_cpu_ns\":" << aeternum::ops::Services().metrics->DenseCpuNanos()
        << "}";
    return oss.str();
}

std::string JsonEscape(std::string_view s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        if (c == '"' || c == '\\') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else out.push_back(c);
    }
    return out;
}

std::string BuildHealthJson()
{
    bool worldOk = false, charOk = false, loginOk = false;
    if (auto* db = aeternum::ops::Services().database)
    {
        worldOk = db->PingWorld();
        charOk = db->PingCharacter();
        loginOk = db->PingLogin();
    }
    uint32 scripts = aeternum::ops::Services().scripts ? aeternum::ops::Services().scripts->ScriptCount() : 0;
    auto modules = aeternum::sdk::ListRegisteredModules();

    std::ostringstream oss;
    oss << "{\"status\":\"ok\""
        << ",\"db\":{\"world\":" << (worldOk ? "true" : "false")
        << ",\"characters\":" << (charOk ? "true" : "false")
        << ",\"login\":" << (loginOk ? "true" : "false") << "}"
        << ",\"scripts\":" << scripts
        << ",\"modules\":[";
    bool first = true;
    for (auto const& m : modules)
    {
        if (!first) oss << ',';
        first = false;
        oss << "{\"id\":\"" << JsonEscape(m.id) << "\",\"version\":\"" << JsonEscape(m.version)
            << "\",\"critical\":" << (m.critical ? "true" : "false")
            << ",\"started\":" << (m.started ? "true" : "false") << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string BuildModuleLifecycleJson()
{
    auto modules = aeternum::sdk::ListModuleLifecycle();
    std::ostringstream oss;
    oss << "{\"modules\":[";
    bool first = true;
    for (auto const& m : modules)
    {
        if (!first) oss << ',';
        first = false;
        oss << "{\"id\":\"" << JsonEscape(m.id)
            << "\",\"critical\":" << (m.critical ? "true" : "false")
            << ",\"state\":\"" << JsonEscape(m.state)
            << "\",\"reason\":\"" << JsonEscape(m.reason) << "\"}";
    }
    oss << "]}";
    return oss.str();
}

uint32 ParseDelaySec(std::string const& target, uint32 fallback)
{
    auto qpos = target.find('?');
    if (qpos == std::string::npos)
        return fallback;
    auto query = target.substr(qpos + 1);
    auto key = std::string("delay=");
    auto pos = query.find(key);
    if (pos == std::string::npos)
        return fallback;
    pos += key.size();
    uint32 v = 0;
    while (pos < query.size() && query[pos] >= '0' && query[pos] <= '9')
    {
        v = v * 10 + uint32(query[pos] - '0');
        ++pos;
    }
    return v ? v : fallback;
}

std::string ListDailyReports(std::filesystem::path const& dir)
{
    std::ostringstream oss;
    oss << "{\"files\":[";
    bool first = true;
    std::error_code ec;
    if (std::filesystem::exists(dir, ec))
    {
        std::vector<std::string> names;
        for (auto const& ent : std::filesystem::directory_iterator(dir, ec))
        {
            if (!ent.is_regular_file())
                continue;
            auto name = ent.path().filename().string();
            if (name.rfind("daily_", 0) == 0 && ent.path().extension() == ".txt")
                names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        for (auto const& n : names)
        {
            if (!first) oss << ',';
            first = false;
            oss << "\"" << n << "\"";
        }
    }
    oss << "]}";
    return oss.str();
}
}

struct HttpServer::Impl
{
    std::unique_ptr<net::io_context> ioc;
    std::unique_ptr<tcp::acceptor> acceptor;
};

HttpServer::HttpServer() = default;

HttpServer::~HttpServer()
{
    Stop();
}

ServerConfig HttpServer::SnapshotConfig() const
{
    std::lock_guard<std::mutex> g(_cfgMutex);
    return _cfg;
}

void HttpServer::ReloadAuthConfig(ServerConfig const& cfg)
{
    std::lock_guard<std::mutex> g(_cfgMutex);
    // Never log tokens. Swap in place.
    _cfg.Token = cfg.Token;
    _cfg.OperatorToken = cfg.OperatorToken;
    _cfg.TokenMinLength = cfg.TokenMinLength ? cfg.TokenMinLength : 32;
    _cfg.AuthFailMax = cfg.AuthFailMax ? cfg.AuthFailMax : 10;
    _cfg.AuthFailWindowSec = cfg.AuthFailWindowSec ? cfg.AuthFailWindowSec : 60;
    _cfg.DenseScrapeDefault = cfg.DenseScrapeDefault;
    _cfg.MetricsTopN = cfg.MetricsTopN ? cfg.MetricsTopN : 20;
    _cfg.AuditRotateBytes = cfg.AuditRotateBytes;
    _cfg.AuditKeepRotated = cfg.AuditKeepRotated;
    _cfg.DrainEnable = cfg.DrainEnable;
    _cfg.DrainMinDelaySec = cfg.DrainMinDelaySec ? cfg.DrainMinDelaySec : 120;
    _authLimiter.Configure(_cfg.AuthFailMax, std::chrono::seconds(_cfg.AuthFailWindowSec));
    _audit.Configure(_cfg.AuditLogPath, _cfg.AuditRotateBytes, _cfg.AuditKeepRotated);
    TC_LOG_WARN("module.admin", "Admin auth/config reloaded (tokenLen={}, opLen={}, min={}, rate={}/{}s, dense={}, drain={})",
        uint32(_cfg.Token.size()), uint32(_cfg.OperatorToken.size()), uint32(_cfg.TokenMinLength),
        uint32(_cfg.AuthFailMax), _cfg.AuthFailWindowSec,
        _cfg.DenseScrapeDefault ? 1 : 0, _cfg.DrainEnable ? 1 : 0);
}

bool HttpServer::Start(ServerConfig cfg)
{
    if (_running.load())
        return true;
    if (!cfg.Enable || cfg.Port == 0)
    {
        TC_LOG_INFO("module.admin", "Admin HTTP disabled (Admin.Enable=0 or Admin.Port=0)");
        return false;
    }
    if (!ValidateAdminBindOrReject(cfg.BindIP))
    {
        TC_LOG_ERROR("module.admin", "Admin HTTP refused to start: bind must be loopback only");
        return false;
    }
    if (cfg.TokenMinLength == 0)
        cfg.TokenMinLength = kDefaultTokenMinLength;
    if (!TokenMeetsMinLength(cfg.Token, cfg.TokenMinLength))
    {
        TC_LOG_ERROR("module.admin", "{}", FormatTokenMissingOrShortError(cfg.TokenMinLength));
        return false;
    }

    if (!cfg.OperatorToken.empty() && !TokenMeetsMinLength(cfg.OperatorToken, cfg.TokenMinLength))
    {
        TC_LOG_ERROR("module.admin", "Admin HTTP: OperatorToken shorter than Admin.TokenMinLength ({}); clearing operator token (never logged)",
            uint32(cfg.TokenMinLength));
        cfg.OperatorToken.clear();
    }
    if (!cfg.OperatorToken.empty() && ConstantTimeEqual(cfg.OperatorToken, cfg.Token))
        TC_LOG_WARN("module.admin", "Admin HTTP: OperatorToken equals read token â€” operator role will shadow readonly (misconfig)");
    if (cfg.DrainMinDelaySec < 120)
        cfg.DrainMinDelaySec = 120;

    {
        std::lock_guard<std::mutex> g(_cfgMutex);
        _cfg = std::move(cfg);
    }
    ServerConfig snap = SnapshotConfig();
    _audit.Configure(snap.AuditLogPath, snap.AuditRotateBytes, snap.AuditKeepRotated);
    _authLimiter.Configure(snap.AuthFailMax, std::chrono::seconds(snap.AuthFailWindowSec));
    _stop = false;
    _impl = std::make_unique<Impl>();
    try
    {
        _impl->ioc = std::make_unique<net::io_context>(1);
        auto const address = net::ip::make_address(snap.BindIP);
        tcp::endpoint ep{address, snap.Port};
        _impl->acceptor = std::make_unique<tcp::acceptor>(*_impl->ioc);
        _impl->acceptor->open(ep.protocol());
        _impl->acceptor->set_option(net::socket_base::reuse_address(true));
        _impl->acceptor->bind(ep);
        _impl->acceptor->listen(net::socket_base::max_listen_connections);
    }
    catch (std::exception const& ex)
    {
        TC_LOG_ERROR("module.admin", "Admin HTTP bind/listen failed: {}", ex.what());
        _impl.reset();
        return false;
    }

    _running = true;
    _thread = std::thread([this] { ThreadMain(); });
    TC_LOG_WARN("module.admin", "Admin HTTP listening on loopback port {} (read-only M9)", uint32(snap.Port));
    return true;
}

void HttpServer::Stop()
{
    if (!_running.exchange(false) && !_thread.joinable())
        return;
    _stop = true;
    if (_impl && _impl->acceptor)
    {
        beast::error_code ec;
        _impl->acceptor->close(ec);
    }
    if (_impl && _impl->ioc)
        _impl->ioc->stop();
    if (_thread.joinable())
        _thread.join();
    _impl.reset();
    TC_LOG_WARN("module.admin", "Admin HTTP stopped");
}

void HttpServer::ThreadMain()
{
    while (!_stop.load())
    {
        if (!_impl || !_impl->acceptor)
            break;
        beast::error_code ec;
        tcp::socket socket(*_impl->ioc);
        _impl->acceptor->accept(socket, ec);
        if (ec)
        {
            if (_stop.load())
                break;
            continue;
        }

        std::string peer = "unknown";
        try
        {
            auto ep = socket.remote_endpoint(ec);
            if (ec || !(ep.address().is_loopback()))
            {
                _audit.Write(AuditEvent{ peer, "ACCEPT", "denied", 403, "none", "non_loopback_peer", 0 });
                continue;
            }
            peer = ep.address().to_string();
        }
        catch (...)
        {
            continue;
        }

        try
        {
            auto const t0 = std::chrono::steady_clock::now();
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req, ec);
            if (ec)
                continue;

            http::response<http::string_body> res;
            res.version(req.version());
            res.keep_alive(false);
            res.set(http::field::server, "aeternum-admin/m9");

            std::string const target(req.target());
            std::string path = target;
            auto qpos = path.find('?');
            std::string query;
            if (qpos != std::string::npos)
            {
                query = path.substr(qpos + 1);
                path.resize(qpos);
            }
            bool denseQuery = query.find("dense=1") != std::string::npos || path == "/metrics/dense";

            auto send = [&](http::status st, std::string body, char const* contentType)
            {
                res.result(st);
                res.set(http::field::content_type, contentType);
                res.body() = std::move(body);
                res.prepare_payload();
            };

            auto auditDone = [&](std::string_view what, int status, std::string_view role, std::string_view result, std::string_view detail)
            {
                auto ms = uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0).count());
                _audit.Write(AuditEvent{ peer, what, result, status, role, detail, ms });
            };

            std::string what = std::string(http::to_string(req.method())) + " " + path;

            bool isDrain = (path == "/admin/drain");
            bool methodOk = (req.method() == http::verb::get) || (isDrain && req.method() == http::verb::post);

            if (!methodOk)
            {
                send(http::status::method_not_allowed, "{\"error\":\"method_not_allowed\"}", "application/json");
                auditDone(what, 405, "none", "denied", "mutating_or_non_get");
            }
            else if (path == "/health")
            {
                send(http::status::ok, BuildHealthJson(), "application/json");
                auditDone(what, 200, "anon", "ok", "health");
            }
            else
            {
                if (_authLimiter.IsBlocked(peer))
                {
                    send(http::status::too_many_requests, "{\"error\":\"rate_limited\"}", "application/json");
                    auditDone(what, 429, "none", "rate_limited", "auth_fail_window");
                }
                else
                {
                    ServerConfig snap = SnapshotConfig();
                    std::string token;
                    auto auth = req[http::field::authorization];
                    if (!ExtractBearerToken(std::string(auth), token))
                    {
                        auto xt = req["X-Admin-Token"];
                        if (!xt.empty())
                            token = std::string(xt);
                    }
                    Role role = AuthenticateDualToken(token, snap.Token, snap.OperatorToken, snap.TokenMinLength);
                    for (char& c : token) c = 0;
                    token.clear();

                    if (role == Role::None)
                    {
                        _authLimiter.RecordFailure(peer);
                        send(http::status::unauthorized, "{\"error\":\"unauthorized\"}", "application/json");
                        auditDone(what, 401, "none", "denied", "auth_failed");
                    }
                    else if (isDrain)
                    {
                        if (!snap.DrainEnable)
                        {
                            send(http::status::forbidden, "{\"error\":\"drain_disabled\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "drain_disabled");
                        }
                        else if (!RoleAllows(role, "admin.drain"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac_drain");
                        }
                        else
                        {
                            uint32 delay = ParseDelaySec(target, snap.DrainMinDelaySec);
                            if (delay < snap.DrainMinDelaySec)
                                delay = snap.DrainMinDelaySec;
                            if (auto* proc = aeternum::ops::Services().process)
                            {
                                proc->SendServerMessage(
                                    "Riavvio/arresto del server tra " + std::to_string(delay) + " secondi. Salva e disconnettiti in sicurezza.");
                                proc->SendServerMessage(
                                    "Server shutdown in " + std::to_string(delay) + " seconds. Please save and log out safely.");
                                proc->RequestShutdown(delay, "admin drain");
                            }
                            std::ostringstream body;
                            body << "{\"scheduled\":true,\"delay_sec\":" << delay << "}";
                            send(http::status::ok, body.str(), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", "drain");
                        }
                    }
                    else if (path == "/modules/lifecycle")
                    {
                        if (!RoleAllows(role, "admin.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            send(http::status::ok, BuildModuleLifecycleJson(), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", "modules_lifecycle");
                        }
                    }
                    else if (path == "/uptime")
                    {
                        if (!RoleAllows(role, "admin.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            std::ostringstream body;
                            body << "{\"uptime_seconds\":" << (aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->UptimeSeconds() : 0) << "}";
                            send(http::status::ok, body.str(), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", {});
                        }
                    }
                    else if (path == "/sessions")
                    {
                        if (!RoleAllows(role, "admin.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            std::ostringstream body;
                            body << "{\"active\":" << (aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->ActiveSessionCount() : 0)
                                 << ",\"queued\":" << (aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->QueuedSessionCount() : 0)
                                 << ",\"players\":" << (aeternum::ops::Services().runtime ? aeternum::ops::Services().runtime->PlayerCount() : 0) << "}";
                            send(http::status::ok, body.str(), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", {});
                        }
                    }
                    else if (path == "/metrics" || path == "/metrics/dense")
                    {
                        if (!RoleAllows(role, "metrics.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            send(http::status::ok, BuildPrometheus(snap, denseQuery || path == "/metrics/dense"),
                                "text/plain; version=0.0.4");
                            auditDone(what, 200, RoleName(role), "ok", denseQuery ? "dense" : "scrape");
                        }
                    }
                    else if (path == "/metrics/store")
                    {
                        if (!RoleAllows(role, "metrics.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            send(http::status::ok, BuildStoreJson(), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", "store");
                        }
                    }
                    else if (path == "/report/latest")
                    {
                        if (!RoleAllows(role, "metrics.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            std::filesystem::path dir = snap.MetricsDirectory.empty()
                                ? std::filesystem::path("Logs/metrics")
                                : std::filesystem::path(snap.MetricsDirectory);
                            auto body = ReadFileLimited(dir / "latest_report.txt");
                            if (body.empty())
                            {
                                send(http::status::not_found, "{\"error\":\"no_report\"}", "application/json");
                                auditDone(what, 404, RoleName(role), "error", "no_report");
                            }
                            else
                            {
                                send(http::status::ok, std::move(body), "text/plain; charset=utf-8");
                                auditDone(what, 200, RoleName(role), "ok", {});
                            }
                        }
                    }
                    else if (path == "/report/daily")
                    {
                        if (!RoleAllows(role, "metrics.read"))
                        {
                            send(http::status::forbidden, "{\"error\":\"forbidden\"}", "application/json");
                            auditDone(what, 403, RoleName(role), "denied", "rbac");
                        }
                        else
                        {
                            std::filesystem::path dir = snap.MetricsDirectory.empty()
                                ? std::filesystem::path("Logs/metrics")
                                : std::filesystem::path(snap.MetricsDirectory);
                            send(http::status::ok, ListDailyReports(dir), "application/json");
                            auditDone(what, 200, RoleName(role), "ok", {});
                        }
                    }
                    else
                    {
                        send(http::status::not_found, "{\"error\":\"not_found\"}", "application/json");
                        auditDone(what, 404, RoleName(role), "error", "not_found");
                    }
                }
            }

            http::write(socket, res, ec);
            socket.shutdown(tcp::socket::shutdown_both, ec);
        }
        catch (std::exception const& ex)
        {
            TC_LOG_ERROR("module.admin", "Admin HTTP session error: {}", ex.what());
        }
    }
}
}
