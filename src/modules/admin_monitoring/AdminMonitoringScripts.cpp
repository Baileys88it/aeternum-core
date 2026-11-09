/*
 * Aeternum Admin & Monitoring â€” SDK static module (M7) + M8 hardening.
 * critical=false: HTTP bind/token failure must not abort worldserver.
 */
#include "AdminHttpServer.h"
#include "AdminTokenResolve.h"
#include "aeternum/sdk/module.hpp"
#include "aeternum/sdk/registry.hpp"
#include "Config.h"
#include "Log.h"
#include "aeternum/api/ops/services.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

namespace
{
std::unique_ptr<AdminMonitoring::HttpServer> g_adminServer;

std::string ResolveEnv(std::string const& envName)
{
    if (envName.empty())
        return {};
    std::string token;
#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, envName.c_str()) == 0 && buf)
    {
        token.assign(buf);
        free(buf);
    }
#else
    if (char const* v = std::getenv(envName.c_str()))
        token = v;
#endif
    return token;
}

std::string ResolveToken(bool* confTokenNonEmptyOut = nullptr)
{
    std::string confToken = sConfigMgr->GetStringDefault("Admin.Token", "");
    std::string envName = AdminMonitoring::EffectiveTokenEnvVarName(
        sConfigMgr->GetStringDefault("Admin.TokenFromEnv", AdminMonitoring::kDefaultAdminTokenEnvVar),
        AdminMonitoring::kDefaultAdminTokenEnvVar);
    std::string envToken = ResolveEnv(envName);
    AdminMonitoring::ResolvedToken resolved = AdminMonitoring::ResolveAdminToken(confToken, envToken);
    if (confTokenNonEmptyOut)
        *confTokenNonEmptyOut = resolved.confTokenNonEmpty;
    for (char& c : confToken) c = 0;
    for (char& c : envToken) c = 0;
    return resolved.value;
}

std::string ResolveOperatorToken(bool* confTokenNonEmptyOut = nullptr)
{
    std::string confToken = sConfigMgr->GetStringDefault("Admin.OperatorToken", "");
    std::string envName = AdminMonitoring::EffectiveTokenEnvVarName(
        sConfigMgr->GetStringDefault("Admin.OperatorTokenFromEnv", AdminMonitoring::kDefaultAdminOperatorTokenEnvVar),
        AdminMonitoring::kDefaultAdminOperatorTokenEnvVar);
    std::string envToken = ResolveEnv(envName);
    AdminMonitoring::ResolvedToken resolved = AdminMonitoring::ResolveAdminToken(confToken, envToken);
    if (confTokenNonEmptyOut)
        *confTokenNonEmptyOut = resolved.confTokenNonEmpty;
    for (char& c : confToken) c = 0;
    for (char& c : envToken) c = 0;
    return resolved.value;
}

AdminMonitoring::ServerConfig LoadConfig()
{
    AdminMonitoring::ServerConfig cfg;
    cfg.Enable = sConfigMgr->GetBoolDefault("Admin.Enable", false);
    cfg.BindIP = sConfigMgr->GetStringDefault("Admin.BindIP", "127.0.0.1");
    cfg.Port = uint16(sConfigMgr->GetIntDefault("Admin.Port", 0));
    bool confTokWarn = false;
    bool confOpWarn = false;
    cfg.Token = ResolveToken(&confTokWarn);
    cfg.TokenMinLength = std::size_t(std::max(1, sConfigMgr->GetIntDefault("Admin.TokenMinLength", 32)));
    cfg.AuthFailMax = std::size_t(std::max(1, sConfigMgr->GetIntDefault("Admin.AuthFailMax", 10)));
    cfg.AuthFailWindowSec = uint32(std::max(1, sConfigMgr->GetIntDefault("Admin.AuthFailWindowSec", 60)));
    cfg.MetricsDirectory = sConfigMgr->GetStringDefault("Metric.Local.Directory", "Logs/metrics");
    cfg.AuditLogPath = sConfigMgr->GetStringDefault("Admin.AuditLogFile", "Logs/admin_audit.log");
    cfg.AuditRotateBytes = uint64(std::max(64, sConfigMgr->GetIntDefault("Admin.AuditRotateMB", 2))) * 1024ull * 1024ull;
    cfg.AuditKeepRotated = uint32(std::max(1, sConfigMgr->GetIntDefault("Admin.AuditKeepRotated", 3)));
    cfg.DenseScrapeDefault = sConfigMgr->GetBoolDefault("Admin.MetricsDenseDefault", false);
    cfg.MetricsTopN = uint32(std::max(1, sConfigMgr->GetIntDefault("Admin.MetricsTopN", 20)));
    cfg.OperatorToken = ResolveOperatorToken(&confOpWarn);
    if (confTokWarn)
        TC_LOG_WARN("module.admin", "{}", AdminMonitoring::FormatConfTokenDeprecationWarn(false));
    if (confOpWarn)
        TC_LOG_WARN("module.admin", "{}", AdminMonitoring::FormatConfTokenDeprecationWarn(true));
    cfg.DrainEnable = sConfigMgr->GetBoolDefault("Admin.Drain.Enable", false);
    cfg.DrainMinDelaySec = uint32(std::max(120, sConfigMgr->GetIntDefault("Admin.Drain.MinDelaySec", 120)));
    return cfg;
}

char const* kSupportedProfiles[] = { "1.60.1.70790", nullptr };

void AdminOnConfigReload(bool reload);

class AdminMonitoringModule final : public aeternum::sdk::IModule
{
public:
    AdminMonitoringModule()
    {
        _desc.identity.id = "admin.monitoring";
        _desc.identity.version = "0.3.0";
        _desc.identity.sdkRange = ">=0.2.0 <1.0.0";
        _desc.runtime = aeternum::sdk::ModuleRuntime::Static;
        _desc.critical = false;
        _desc.dependencies = nullptr;
        _desc.supportedProfiles = kSupportedProfiles;
    }

    aeternum::sdk::ModuleDescriptor const& GetDescriptor() const override { return _desc; }

    aeternum::sdk::ModuleResult OnLoad() override
    {
        aeternum::ops::SetConfigReloadHook(&AdminOnConfigReload);
        TC_LOG_WARN("module.admin", "admin.monitoring OnLoad (SDK M9)");
        return aeternum::sdk::ModuleResult::Ok;
    }

    aeternum::sdk::ModuleResult OnInit() override
    {
        TC_LOG_WARN("module.admin", "admin.monitoring OnInit (SDK M9)");
        return aeternum::sdk::ModuleResult::Ok;
    }

    aeternum::sdk::ModuleResult OnStart() override
    {
        auto cfg = LoadConfig();
        if (!cfg.Enable || cfg.Port == 0)
        {
            TC_LOG_WARN("module.admin", "admin.monitoring OnStart â€” HTTP off (Admin.Enable/Port)");
            for (char& c : cfg.Token) c = 0;
            for (char& c : cfg.OperatorToken) c = 0;
            return aeternum::sdk::ModuleResult::Ok;
        }
        g_adminServer = std::make_unique<AdminMonitoring::HttpServer>();
        if (!g_adminServer->Start(cfg))
        {
            g_adminServer.reset();
            for (char& c : cfg.Token) c = 0;
            for (char& c : cfg.OperatorToken) c = 0;
            return aeternum::sdk::ModuleResult::Failed;
        }
        for (char& c : cfg.Token) c = 0;
        for (char& c : cfg.OperatorToken) c = 0;
        TC_LOG_WARN("module.admin", "admin.monitoring OnStart â€” HTTP listening (M9)");
        return aeternum::sdk::ModuleResult::Ok;
    }

    aeternum::sdk::ModuleResult OnStop() override
    {
        if (g_adminServer)
        {
            g_adminServer->Stop();
            g_adminServer.reset();
        }
        TC_LOG_WARN("module.admin", "admin.monitoring OnStop");
        return aeternum::sdk::ModuleResult::Ok;
    }

    aeternum::sdk::ModuleResult OnShutdown() override
    {
        TC_LOG_WARN("module.admin", "admin.monitoring OnShutdown");
        return aeternum::sdk::ModuleResult::Ok;
    }

private:
    aeternum::sdk::ModuleDescriptor _desc{};
};

AdminMonitoringModule g_adminModule;

void AdminOnConfigReload(bool reload)
{
    if (!reload || !g_adminServer || !g_adminServer->IsRunning())
        return;
    auto cfg = LoadConfig();
    g_adminServer->ReloadAuthConfig(cfg);
    for (char& c : cfg.Token) c = 0;
    for (char& c : cfg.OperatorToken) c = 0;
}
} // anon

void AddSC_admin_monitoring()
{
    aeternum::sdk::RegisterModule(&g_adminModule);
}
