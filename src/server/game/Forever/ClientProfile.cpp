/*
 * Aeternum Forever — client profile foundation (M4) + data/update apply (M6).
 */

#include "Forever/ClientProfile.h"
#include "Config.h"
#include "Log.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace Forever
{
namespace
{
ClientProfileInfo g_appliedProfile;
bool g_profileApplied = false;
std::vector<uint32> g_confClientInserts;
std::vector<uint32> g_confServerInserts;
bool g_confParsed = false;

std::string Trim(std::string s)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    while (!s.empty() && !notSpace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && !notSpace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

bool ParseHexU32(std::string token, uint32& out)
{
    token = Trim(token);
    if (token.size() >= 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
        token = token.substr(2);
    if (token.empty())
        return false;
    try
    {
        out = static_cast<uint32>(std::stoul(token, nullptr, 16));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string FormatInsertList(std::vector<uint32> const& v)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        if (i)
            oss << ',';
        oss << "0x" << std::hex << v[i];
    }
    return oss.str();
}

std::vector<uint32> ParseOpcodeListFromConf(char const* key)
{
    std::vector<uint32> result;
    std::stringstream stream(sConfigMgr->GetStringDefault(key, ""));
    std::string token;
    while (std::getline(stream, token, ','))
    {
        token.erase(std::remove_if(token.begin(), token.end(),
            [](char c) { return c == ' ' || c == '\t' || c == '"'; }), token.end());
        if (token.empty())
            continue;
        try
        {
            result.push_back(uint32(std::stoul(token, nullptr, 0)));
        }
        catch (...)
        {
            TC_LOG_ERROR("server.loading", "{}: '{}' is not an opcode number, ignored", key, token);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void EnsureConfInsertsParsed()
{
    if (g_confParsed)
        return;
    g_confClientInserts = ParseOpcodeListFromConf("Classic.ClientExtraInserts");
    g_confServerInserts = ParseOpcodeListFromConf("Classic.ServerExtraInserts");
    g_confParsed = true;
}

bool AutoApplyEnabled(std::string const& profileId)
{
    int32 configured = sConfigMgr->GetIntDefault("Classic.ProfileExtraInserts.AutoApply", -1);
    if (configured == 0)
        return false;
    if (configured > 0)
        return true;
    return !profileId.empty();
}

bool ExtractManifestInsertArray(rapidjson::Value const& arr, std::vector<uint32>& out)
{
    out.clear();
    if (!arr.IsArray())
        return false;
    for (auto const& v : arr.GetArray())
    {
        if (!v.IsString())
            return false;
        uint32 n = 0;
        if (!ParseHexU32(v.GetString(), n))
            return false;
        out.push_back(n);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return true;
}
}

std::string GetConfiguredActiveProfileId()
{
    return Trim(sConfigMgr->GetStringDefault("Classic.ActiveClientProfile", ""));
}

std::string ProfileDirectoryRel(std::string const& profileId)
{
    return "forever/client_profiles/" + profileId;
}

std::vector<uint32> ParseOpcodeInsertList(std::string const& value)
{
    std::vector<uint32> out;
    std::string cur;
    for (char ch : value)
    {
        if (ch == ',')
        {
            uint32 v = 0;
            if (ParseHexU32(cur, v))
                out.push_back(v);
            cur.clear();
        }
        else
            cur.push_back(ch);
    }
    uint32 v = 0;
    if (ParseHexU32(cur, v))
        out.push_back(v);
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

bool TryLoadProfileExtraInserts(std::string const& sourceRoot, std::string const& profileId, ClientProfileInfo& out)
{
    out = ClientProfileInfo{};
    out.ProfileId = profileId;
    auto dot = profileId.rfind('.');
    if (dot != std::string::npos)
    {
        try { out.ClientBuild = static_cast<uint32>(std::stoul(profileId.substr(dot + 1))); }
        catch (...) { out.ClientBuild = 0; }
    }

    std::filesystem::path path = std::filesystem::path(sourceRoot) / ProfileDirectoryRel(profileId) / "extra_inserts.conf";

    std::ifstream in(path);
    if (!in)
        return false;

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::string t = Trim(line);
        if (t.empty() || t[0] == '#')
            continue;
        auto eq = t.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = Trim(t.substr(0, eq));
        std::string val = Trim(t.substr(eq + 1));
        if (!val.empty() && val.front() == '"' && val.back() == '"')
            val = val.substr(1, val.size() - 2);
        if (key == "Classic.ClientExtraInserts")
            out.ClientExtraInserts = ParseOpcodeInsertList(val);
        else if (key == "Classic.ServerExtraInserts")
            out.ServerExtraInserts = ParseOpcodeInsertList(val);
    }
    out.Loaded = true;
    return true;
}

std::string ResolveProfileDataRoot()
{
    std::string configured = Trim(sConfigMgr->GetStringDefault("Classic.ProfileDataRoot", ""));
    if (!configured.empty())
    {
        std::filesystem::path p(configured);
        if (std::filesystem::is_directory(p / "forever" / "client_profiles"))
            return p.generic_string();
        if (std::filesystem::is_directory(p) && p.filename() == "client_profiles")
            return p.parent_path().parent_path().generic_string();
        if (std::filesystem::is_directory(p / "client_profiles"))
            return p.parent_path().generic_string();
        return configured;
    }

    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (!ec && fs::is_directory(cwd / "forever" / "client_profiles", ec))
        return cwd.generic_string();

    fs::path walk = cwd;
    for (int i = 0; i < 6 && !walk.empty(); ++i)
    {
        if (fs::is_directory(walk / "forever" / "client_profiles", ec))
            return walk.generic_string();
        if (!walk.has_parent_path() || walk.parent_path() == walk)
            break;
        walk = walk.parent_path();
    }
    return {};
}

std::string ReadActiveProfilePointer(std::string const& sourceRoot)
{
    std::ifstream in(std::filesystem::path(sourceRoot) / "forever" / "client_profiles" / "ACTIVE");
    if (!in)
        return {};
    std::string id;
    std::getline(in, id);
    if (!id.empty() && id.back() == '\r')
        id.pop_back();
    return Trim(id);
}

ProfileDriftReport CheckProfileDataConsistency(std::string const& sourceRoot, std::string const& configuredId)
{
    ProfileDriftReport r;
    r.ConfiguredId = configuredId;
    r.ActiveFileId = ReadActiveProfilePointer(sourceRoot);
    r.ActiveFileReadable = !r.ActiveFileId.empty();
    if (r.ActiveFileReadable && !configuredId.empty())
        r.IdsMatch = (r.ActiveFileId == configuredId);
    else if (r.ActiveFileReadable && configuredId.empty())
        r.IdsMatch = false;

    std::string id = !configuredId.empty() ? configuredId : r.ActiveFileId;
    if (id.empty())
    {
        r.Detail = "no profile id";
        return r;
    }

    ClientProfileInfo fromFile;
    if (!TryLoadProfileExtraInserts(sourceRoot, id, fromFile))
    {
        r.Detail = "extra_inserts.conf missing";
        return r;
    }

    std::filesystem::path manPath = std::filesystem::path(sourceRoot) / ProfileDirectoryRel(id) / "manifest.json";
    std::ifstream min(manPath, std::ios::binary);
    if (!min)
    {
        r.Detail = "manifest.json missing";
        return r;
    }
    std::string json((std::istreambuf_iterator<char>(min)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    if (doc.Parse(json.c_str()).HasParseError() || !doc.IsObject())
    {
        r.Detail = "manifest.json parse error";
        return r;
    }
    r.ManifestReadable = true;
    if (!doc.HasMember("extra_inserts") || !doc["extra_inserts"].IsObject())
    {
        r.Detail = "manifest missing extra_inserts object";
        r.ManifestMatchesExtraInsertsFile = false;
        return r;
    }
    auto const& ei = doc["extra_inserts"];
    std::vector<uint32> manClient, manServer;
    if (!ei.HasMember("Classic.ClientExtraInserts") || !ExtractManifestInsertArray(ei["Classic.ClientExtraInserts"], manClient)
        || !ei.HasMember("Classic.ServerExtraInserts") || !ExtractManifestInsertArray(ei["Classic.ServerExtraInserts"], manServer))
    {
        r.Detail = "manifest extra_inserts arrays invalid";
        r.ManifestMatchesExtraInsertsFile = false;
        return r;
    }
    r.ManifestMatchesExtraInsertsFile =
        (manClient == fromFile.ClientExtraInserts) && (manServer == fromFile.ServerExtraInserts);
    if (!r.ManifestMatchesExtraInsertsFile)
        r.Detail = "manifest extra_inserts != extra_inserts.conf";
    else if (!r.IdsMatch)
        r.Detail = "ACTIVE pointer != Classic.ActiveClientProfile";
    else
        r.Detail = "ok";
    return r;
}

void ApplyActiveProfileDataAtStartup()
{
    g_profileApplied = false;
    g_appliedProfile = ClientProfileInfo{};
    EnsureConfInsertsParsed();

    std::string const profileId = GetConfiguredActiveProfileId();
    if (!AutoApplyEnabled(profileId))
    {
        TC_LOG_INFO("server.loading", "Forever profile ExtraInserts auto-apply off (using conf Classic.*ExtraInserts)");
        return;
    }
    if (profileId.empty())
    {
        TC_LOG_INFO("server.loading", "Forever profile ExtraInserts auto-apply skipped: Classic.ActiveClientProfile empty");
        return;
    }

    std::string root = ResolveProfileDataRoot();
    if (root.empty())
    {
        TC_LOG_ERROR("server.loading",
            "Forever profile ExtraInserts auto-apply failed: cannot find forever/client_profiles (set Classic.ProfileDataRoot)");
        return;
    }

    ProfileDriftReport drift = CheckProfileDataConsistency(root, profileId);
    if (!drift.IdsMatch)
        TC_LOG_WARN("server.loading",
            "Forever profile drift: ACTIVE='{}' Classic.ActiveClientProfile='{}' — fix before next client flip",
            drift.ActiveFileId, drift.ConfiguredId);
    if (drift.ManifestReadable && !drift.ManifestMatchesExtraInsertsFile)
        TC_LOG_WARN("server.loading", "Forever profile data inconsistency: {}", drift.Detail);

    ClientProfileInfo info;
    if (!TryLoadProfileExtraInserts(root, profileId, info) || !info.Loaded)
    {
        TC_LOG_ERROR("server.loading",
            "Forever profile ExtraInserts auto-apply failed to load {}/extra_inserts.conf under {}",
            ProfileDirectoryRel(profileId), root);
        return;
    }

    if (g_confClientInserts != info.ClientExtraInserts || g_confServerInserts != info.ServerExtraInserts)
    {
        TC_LOG_WARN("server.loading",
            "Forever profile ExtraInserts override conf (profile wins). conf client=[{}] server=[{}] profile client=[{}] server=[{}]",
            FormatInsertList(g_confClientInserts), FormatInsertList(g_confServerInserts),
            FormatInsertList(info.ClientExtraInserts), FormatInsertList(info.ServerExtraInserts));
    }

    g_appliedProfile = std::move(info);
    g_profileApplied = true;
    // WARN so it appears on Appender.Server (configured at level 3 / Warn)
    TC_LOG_WARN("server.loading",
        "Forever profile ExtraInserts auto-applied from {} (client={} server={})",
        ProfileDirectoryRel(profileId),
        FormatInsertList(g_appliedProfile.ClientExtraInserts),
        FormatInsertList(g_appliedProfile.ServerExtraInserts));
}

std::vector<uint32> const& EffectiveClientExtraInserts()
{
    EnsureConfInsertsParsed();
    if (g_profileApplied)
        return g_appliedProfile.ClientExtraInserts;
    return g_confClientInserts;
}

std::vector<uint32> const& EffectiveServerExtraInserts()
{
    EnsureConfInsertsParsed();
    if (g_profileApplied)
        return g_appliedProfile.ServerExtraInserts;
    return g_confServerInserts;
}

bool IsProfileExtraInsertsApplied()
{
    return g_profileApplied;
}

void LogClientProfileSeams(std::string const& profileId)
{
    if (profileId.empty())
    {
        TC_LOG_INFO("server.loading", "Forever client profile: Classic.ActiveClientProfile unset (legacy ExtraInserts-only mode)");
        return;
    }
    TC_LOG_INFO("server.loading", "Forever client profile active id: {}", profileId);
    TC_LOG_INFO("server.loading", "  ExtraInserts source: {}",
        g_profileApplied ? "profile file (auto-apply)" : "worldserver.conf");
    TC_LOG_INFO("server.loading", "  opcode seam: ClassicOpcodes TranslateClientOpcode / ExtraInserts + forever/client_profiles/{}/opcodes_table.md", profileId);
    TC_LOG_INFO("server.loading", "  hotfix seam: sql/custom/hotfixes + hotfixes DB; profile seams/hotfix_seams.md");
    TC_LOG_INFO("server.loading", "  auth seam: auth.build_info + realmlist.gamebuild (see profile build_auth_keys.md)");
}
}
