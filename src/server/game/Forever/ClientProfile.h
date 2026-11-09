/*
 * Aeternum Forever — versioned client profile + data/update management (M4 foundation, M6 apply).
 * Single active profile per deployment. Profile ExtraInserts can auto-apply at startup.
 */
#ifndef AETERNUM_FOREVER_CLIENT_PROFILE_H
#define AETERNUM_FOREVER_CLIENT_PROFILE_H

#include "Define.h"

#include <string>
#include <vector>

namespace Forever
{
struct ClientProfileInfo
{
    std::string ProfileId;          // e.g. 1.60.1.70790
    uint32 ClientBuild = 0;
    std::vector<uint32> ClientExtraInserts;
    std::vector<uint32> ServerExtraInserts;
    bool Loaded = false;
};

struct ProfileDriftReport
{
    bool ActiveFileReadable = false;
    std::string ActiveFileId;
    std::string ConfiguredId;
    bool IdsMatch = true;
    bool ManifestReadable = false;
    bool ManifestMatchesExtraInsertsFile = true;
    std::string Detail;
};

/** Conf key Classic.ActiveClientProfile (empty = unset / use legacy ExtraInserts only). */
std::string GetConfiguredActiveProfileId();

/** Relative repo path forever/client_profiles/<id>/ */
std::string ProfileDirectoryRel(std::string const& profileId);

/** Parse comma-separated 0xGGIIII list (same rules as ClassicOpcodes ParseOpcodeList value). */
std::vector<uint32> ParseOpcodeInsertList(std::string const& value);

/**
 * Load ExtraInserts from forever/client_profiles/<id>/extra_inserts.conf
 * when sourceRoot is the engine source tree root. Returns false on I/O/parse failure.
 */
bool TryLoadProfileExtraInserts(std::string const& sourceRoot, std::string const& profileId, ClientProfileInfo& out);

/**
 * Resolve directory that contains forever/client_profiles/.
 * Order: Classic.ProfileDataRoot (conf) → cwd → executable parent walk (best-effort).
 */
std::string ResolveProfileDataRoot();

/** Read forever/client_profiles/ACTIVE (trimmed). Empty if missing. */
std::string ReadActiveProfilePointer(std::string const& sourceRoot);

/**
 * Lightweight check: ACTIVE vs Classic.ActiveClientProfile, and manifest.json
 * extra_inserts arrays vs extra_inserts.conf (rapidjson; already a game dep).
 */
ProfileDriftReport CheckProfileDataConsistency(std::string const& sourceRoot, std::string const& configuredId);

/**
 * M6: when Classic.ProfileExtraInserts.AutoApply is enabled (default 1 if
 * ActiveClientProfile set, else 0) load profile ExtraInserts into the process
 * cache used by ClassicOpcodes. Profile wins over conf lists; mismatches are logged.
 * Safe no-op if profile unset / AutoApply off / load fails (keeps conf).
 */
void ApplyActiveProfileDataAtStartup();

/** Effective ExtraInserts for opcode remap (profile cache if applied, else conf). */
std::vector<uint32> const& EffectiveClientExtraInserts();
std::vector<uint32> const& EffectiveServerExtraInserts();

bool IsProfileExtraInsertsApplied();

/** Log opcode/hotfix seam pointers for operators (EN). Safe if profile unset. */
void LogClientProfileSeams(std::string const& profileId);
}

#endif
