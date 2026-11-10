#include "ConfigSchema.h"
#include "Config.h"
#include "Log.h"

#include <cstdio>

namespace Trinity::ConfigSchema
{
namespace
{
char const* const kKnownPrefixes[] = {
    "Admin.", "Metric.", "Logger.", "Appender.", "Log.", "LogLevel.",
    "Network.", "SOAP.", "Ra.", "Rate.", "Console.", "Player.",
    "WorldServer.", "LoginDatabase", "WorldDatabase", "CharacterDatabase", "HotfixDatabase",
    "MaxCoreStuckTime", "ThreadPool", "RealmID", "DataDir", "LogsDir", "PIDFile",
    "PacketLog", "Crash.", "Config.Schema.", "AuctionHouseBot.", "Battleground.", "Arena.",
    "Quests.", "Visibility.", "MapUpdate.", "vmap.", "mmap.", "DetectPosCollision",
    "Compression", "TargetPosRecalculateRange", "SessionAddDelay", "GridUnload",
    "SocketTimeOutTime", "ClientCacheVersion", "GameType", "RealmZone", "Expansion",
    "WorldServerPort", "BindIP", "MaxOverspeedPings", "UseProcessors", "ProcessPriority",
    "GridCleanUpDelay", "MapUpdateInterval", "ChangeWeatherInterval", "PlayerSaveInterval",
    "MinWorldUpdateTime", "CloseIdleConnections", "AllowTwoSide.", "IpLocationFile",
    "CharDelete.", "DBC.", "Load.", "Preload.", "Guild.", "Calendar.", "SkillChance.",
    "SkillGain.", "Stats.", "ListenRange.", "Warden.", "FeatureSystem.", "Allow.",
    "InstantLogout", "GM.", "Honor.", "PvP.", "ShowKickInWorld", "ShowMuteInWorld",
    "ShowBanInWorld", "Respawn.", "Command.", "ChatFlood.", "ChatStrictLinkChecking.",
    "Channel.", "Wintergrasp.", "Battlenet.", "Account.", "Updates.", "HotSwap.",
    "Server.", "PacketSpoof.", "Creature.", "Vehicle.", "MonsterSight", "Corpse.",
    "Death.", "DurabilityLoss.", "MailDeliveryDelay", "CleanCharacterDB",
    "Forever.", "Aeternum.", "Module.", "StrictPlayerNames", "StrictCharterNames",
    "StrictPetNames", "StrictChannelNames", "Database.", "CharacterCreating.",
    "MaxPlayerLevel", "StartPlayerLevel", "StartPlayerMoney", "MaxHonorPoints",
    "MaxArenaPoints", "RecruitAFriend.", "Currency.", "SkillChance", "Event.",
    "SaveRespawnTimeImmediately", "AlwaysMaxSkillForLevel", "Weather",
    "Group.", "Instance.", "ResetInstances.", "Ignore.", "TalentsInspecting",
    "ChatFakeMessagePreventing", "DeclinedNames", "MinTradeLevel", "PreserveCustomChannels",
    "PreserveCustomChannelDuration", "CharDelete", "CacheDataQueries",
    nullptr
};

char const* const kExactKnown[] = {
    "RealmID", "DataDir", "LogsDir", "LoginDatabaseInfo", "WorldDatabaseInfo",
    "CharacterDatabaseInfo", "HotfixDatabaseInfo", "MaxPlayerCount", "GameType",
    "RealmZone", "Expansion", "WorldServerPort", "BindIP", "ThreadPool", "PIDFile",
    "ClientCacheVersion", "Expansion", "MaxPingTime", "RealmList",
    nullptr
};

bool StartsWithAny(std::string_view key, char const* const* prefixes)
{
    for (char const* const* p = prefixes; *p; ++p)
    {
        std::string_view pref(*p);
        if (key.size() >= pref.size() && key.substr(0, pref.size()) == pref)
            return true;
    }
    return false;
}

bool ExactMatch(std::string_view key, char const* const* exact)
{
    for (char const* const* p = exact; *p; ++p)
        if (key == *p)
            return true;
    return false;
}
}

bool IsOwnedPrefix(std::string_view key)
{
    char const* owned[] = { "Admin.", "Metric.", "Crash.", "Config.Schema.", "Forever.", "Aeternum.", "Module.", nullptr };
    return StartsWithAny(key, owned);
}

bool IsKnownOwnedKey(std::string_view key)
{
    // Explicit allowlist for Aeternum-owned namespaces (WARN on anything else under these prefixes).
    char const* allowed[] = {
        "Admin.Enable", "Admin.BindIP", "Admin.Port", "Admin.Token", "Admin.TokenFromEnv",
        "Admin.TokenMinLength", "Admin.AuthFailMax", "Admin.AuthFailWindowSec",
        "Admin.AuditLogFile", "Admin.AuditRotateMB", "Admin.AuditKeepRotated",
        "Admin.MetricsDenseDefault", "Admin.MetricsTopN",
        "Admin.OperatorToken", "Admin.OperatorTokenFromEnv",
        "Admin.Drain.Enable", "Admin.Drain.MinDelaySec",
        "Metric.Enable", "Metric.Interval", "Metric.ConnectionInfo", "Metric.OverallStatusInterval",
        "Metric.Sink", "Metric.Local.Directory", "Metric.Local.BudgetMB", "Metric.Local.RetentionDays",
        "Metric.Local.FlushSeconds", "Metric.Local.ReportIntervalSeconds", "Metric.Local.Compress",
        "Metric.Local.MaxSeries", "Metric.DenseScrape", "Metric.Dense.SamplePercent",
        "Metric.Threshold.world_tick_ms",
        "Crash.Directory", "Crash.BudgetMB", "Crash.RetentionDays",
        "Config.Schema.FailFast", "Config.Schema.WarnUnknown", "Config.Schema.WarnUnknownAll",
        "Config.Schema.Required",
        "Aeternum.Modules.ManifestDir", "Aeternum.Modules.RequireManifest", "Aeternum.Modules.NativeRuntime",
        "Aeternum.Modules.HotReload",
        // Edge.Enable is read by worldserver StartEdge() (ignored; transport is aeternum_gateway).
        // The remaining Edge.* keys ship in worldserver.conf.dist and the operator overlay.
        "Aeternum.Edge.Enable", "Aeternum.Edge.BindIP", "Aeternum.Edge.Port", "Aeternum.Edge.MaxSessions",
        "Aeternum.Edge.ConnectTimeoutMs", "Aeternum.Edge.IdleTimeoutMs",
        "Aeternum.Edge.BackendAddresses", "Aeternum.Edge.BackendPorts",
        "Aeternum.TelemetryHttp.Enable", "Aeternum.TelemetryHttp.Port", "Aeternum.TelemetryHttp.BindIP",
        "Aeternum.ECS.Enabled", "Aeternum.ECS.OwnedMapId", "Aeternum.ECS.OwnedInstanceId",
        "Aeternum.ECS.Capacity", "Aeternum.ECS.Workers",
        "Aeternum.Welcome.Message",
        nullptr
    };
    if (ExactMatch(key, allowed))
        return true;
    // Metric.Threshold.* dynamic
    if (key.rfind("Metric.Threshold.", 0) == 0)
        return true;
    return false;
}

bool IsKnownKey(std::string_view key)
{
    if (key.empty())
        return true;
    if (ExactMatch(key, kExactKnown))
        return true;
    if (IsOwnedPrefix(key))
        return IsKnownOwnedKey(key);
    return StartsWithAny(key, kKnownPrefixes);
}

std::vector<std::string> DefaultRequiredKeys()
{
    std::string overrideList = sConfigMgr->GetStringDefault("Config.Schema.Required", "", true);
    if (!overrideList.empty())
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : overrideList)
        {
            if (c == ',' || c == ';')
            {
                if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            }
            else if (c != ' ' && c != '\t')
                cur.push_back(c);
        }
        if (!cur.empty())
            out.push_back(cur);
        return out;
    }
    return {
        "RealmID",
        "DataDir",
        "LoginDatabaseInfo",
        "WorldDatabaseInfo",
        "CharacterDatabaseInfo"
    };
}

ValidationResult ValidateLoadedConfig(bool warnUnknown)
{
    ValidationResult r;
    auto keys = sConfigMgr->GetKeysByString("");
    auto required = DefaultRequiredKeys();

    for (auto const& req : required)
    {
        std::string val = sConfigMgr->GetStringDefault(req, "", true);
        bool present = false;
        for (auto const& k : keys)
            if (k == req) { present = true; break; }
        if (!present)
        {
            r.ok = false;
            ++r.requiredFailed;
            r.issues.push_back({ ValidationIssue::Kind::MissingRequired, req, "required key missing" });
        }
        else if (val.empty())
        {
            r.ok = false;
            ++r.requiredFailed;
            r.issues.push_back({ ValidationIssue::Kind::EmptyRequired, req, "required key empty" });
        }
    }

    // Default: only WARN unknown keys under Aeternum-owned prefixes (Admin/Metric/Crash/Config.Schema/...).
    // Config.Schema.WarnUnknownAll=1 scans every key against the catalog (noisy on stock Trinity confs).
    bool warnAll = sConfigMgr->GetBoolDefault("Config.Schema.WarnUnknownAll", false, true);
    bool doWarn = warnUnknown && sConfigMgr->GetBoolDefault("Config.Schema.WarnUnknown", true, true);
    if (doWarn)
    {
        char const* owned[] = { "Admin.", "Metric.", "Crash.", "Config.Schema.", "Forever.", "Aeternum.", "Module.", nullptr };
        for (auto const& k : keys)
        {
            bool inOwned = warnAll;
            if (!inOwned)
            {
                for (char const** p = owned; *p; ++p)
                {
                    std::string_view pref(*p);
                    if (k.size() >= pref.size() && k.compare(0, pref.size(), pref) == 0)
                    {
                        inOwned = true;
                        break;
                    }
                }
            }
            if (!inOwned)
                continue;
            if (IsKnownKey(k))
                continue;
            ++r.unknownWarned;
            r.issues.push_back({ ValidationIssue::Kind::UnknownKey, k, "unknown key (not in schema catalog)" });
        }
    }
    return r;
}

void LogIssues(ValidationResult const& r)
{
    for (auto const& i : r.issues)
    {
        if (i.kind == ValidationIssue::Kind::UnknownKey)
            TC_LOG_WARN("server.loading", "Config schema: unknown key '{}' (WARN)", i.key);
        else
            TC_LOG_ERROR("server.loading", "Config schema: {} — {}", i.key, i.message);
    }
    if (r.ok)
        TC_LOG_INFO("server.loading", "Config schema validation OK (unknown_warned={})", r.unknownWarned);
    else
        TC_LOG_ERROR("server.loading", "Config schema validation FAIL (required_failures={})", r.requiredFailed);
}

int PrintAndExitCode(ValidationResult const& r)
{
    std::printf("Aeternum --check-config: %s (required_failures=%u unknown_warned=%u issues=%zu)\n",
        r.ok ? "OK" : "FAIL", r.requiredFailed, r.unknownWarned, r.issues.size());
    for (auto const& i : r.issues)
    {
        char const* kind = "?";
        switch (i.kind)
        {
            case ValidationIssue::Kind::MissingRequired: kind = "MISSING"; break;
            case ValidationIssue::Kind::EmptyRequired: kind = "EMPTY"; break;
            case ValidationIssue::Kind::UnknownKey: kind = "UNKNOWN"; break;
        }
        std::printf("  [%s] %s - %s\n", kind, i.key.c_str(), i.message.c_str());
    }
    return r.ok ? 0 : 1;
}
}
