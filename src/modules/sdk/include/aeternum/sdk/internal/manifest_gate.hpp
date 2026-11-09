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

/*
 * Aeternum M1-Blocco2 - INTERNAL boot gate for module.manifest.json (D9).
 *
 * - Pure std + rapidjson (through manifest.hpp). No Trinity types: this tree is a lint FAIL surface.
 * - Runs ONCE from HostDiscoverAndStart() at world startup (on-boot-only path).
 *   NEVER call from World::Update, WorldSession::Update or any per-packet / per-tick path:
 *   it allocates (std::string, rapidjson DOM, file I/O) and that is only acceptable at boot.
 * - Lookup: <manifestDir>/<module id>/module.manifest.json
 * - Fail-closed: a manifest that exists but is invalid ALWAYS rejects the module
 *   (critical module => abort boot, non-critical => disabled, server keeps booting).
 * - Missing manifest: rejected only when requireManifest=true; otherwise legacy Level 1 load + WARN
 *   (M1 transitional rule, keeps vanilla / pre-manifest deployments booting).
 * - sha256 (D10 / Blocco5): integrity.file+sha256 (legacy) XOR integrity.files[{path,sha256}].
 *   Each path is relative under <manifestDir>/<id>/; hashed with OpenSSL EVP at boot only.
 *   Mismatch/missing => Rejected (reason names the path). Both forms together => rejected at parse.
 *   Missing integrity entirely: unchanged M1 policy (not required). Never RequireManifest solely for hash.
 */
#ifndef AETERNUM_SDK_INTERNAL_MANIFEST_GATE_HPP
#define AETERNUM_SDK_INTERNAL_MANIFEST_GATE_HPP

#include "aeternum/abi/AeternumABI.h"
#include "aeternum/sdk/internal/static_runtime.hpp"
#include "aeternum/sdk/manifest.hpp"
#include "aeternum/sdk/internal/sha256_file.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace aeternum::sdk::internal
{
inline constexpr char const* kModuleManifestFileName = "module.manifest.json";
inline constexpr std::size_t kMaxModuleManifestBytes = 64u * 1024u;

struct ManifestBootOptions
{
    std::string manifestDir;      // empty => "modules" relative to the working directory
    bool requireManifest = false; // M1 transitional default
    // IE-M3 Blocco1: OS-level native loader. Only honoured when the host is built with
    // AETERNUM_NATIVE_MODULE_RUNTIME=ON (CMake, default ON since Blocco3) AND config Aeternum.Modules.NativeRuntime=1 (default 0).
    bool nativeRuntime = false;
    bool nativeHotReload = false; // Aeternum.Modules.HotReload, default OFF
};

enum class ManifestGateOutcome : unsigned char
{
    Accepted = 0,
    LegacyNoManifest,
    Rejected
};

struct ManifestGateResult
{
    ManifestGateOutcome outcome = ManifestGateOutcome::Rejected;
    AeternumStatus status = AETERNUM_ERR_MANIFEST_INVALID;
    std::string reason;      // empty when Accepted / LegacyNoManifest
    ModuleManifest manifest; // meaningful only when Accepted
};

enum class ManifestReadStatus : unsigned char
{
    Ok = 0,
    NotFound,
    Error
};

/** Module ids are dotted lowercase identifiers; also guards the on-disk lookup against path traversal. */
inline bool IsSafeModuleId(std::string_view id) noexcept
{
    if (id.empty() || id.size() > 128 || id.front() == '.' || id.back() == '.')
        return false;
    char prev = '\0';
    for (char c : id)
    {
        bool const ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok || (c == '.' && prev == '.'))
            return false;
        prev = c;
    }
    return true;
}

inline std::filesystem::path ModuleManifestPath(std::filesystem::path const& dir, std::string_view moduleId)
{
    return dir / std::filesystem::path(std::string(moduleId)) / kModuleManifestFileName;
}

inline ManifestReadStatus ReadModuleManifestFile(std::filesystem::path const& path, std::string& out, std::string& error)
{
    std::error_code ec;
    bool const exists = std::filesystem::exists(path, ec);
    if (ec)
    {
        error = "manifest_stat_failed";
        return ManifestReadStatus::Error;
    }
    if (!exists)
        return ManifestReadStatus::NotFound;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
    {
        error = "manifest_not_regular_file";
        return ManifestReadStatus::Error;
    }
    std::uintmax_t const size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        error = "manifest_stat_failed";
        return ManifestReadStatus::Error;
    }
    if (size > kMaxModuleManifestBytes)
    {
        error = "manifest_too_large";
        return ManifestReadStatus::Error;
    }
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
    {
        error = "manifest_open_failed";
        return ManifestReadStatus::Error;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    if (in.bad())
    {
        error = "manifest_read_failed";
        return ManifestReadStatus::Error;
    }
    return ManifestReadStatus::Ok;
}

inline ManifestGateResult RejectManifest(AeternumStatus status, std::string reason)
{
    ManifestGateResult r;
    r.outcome = ManifestGateOutcome::Rejected;
    r.status = status;
    r.reason = std::move(reason);
    return r;
}


/** Normalize legacy or multi-file integrity into an ordered entry list. Empty => no hash check. */
inline std::vector<IntegrityFileEntry> CollectIntegrityEntries(ModuleManifest const& m)
{
    std::vector<IntegrityFileEntry> out;
    if (m.hasIntegrityFiles)
    {
        out = m.integrityFiles;
        return out;
    }
    if (m.hasSha256)
    {
        IntegrityFileEntry e;
        e.path = m.integrityFile; // may be empty -> gate rejects integrity_file_required
        e.sha256 = m.sha256;
        out.push_back(std::move(e));
    }
    return out;
}

/**
 * Pure validation of one module's manifest content against its compiled descriptor id.
 * When integrity.sha256 is set, pass the actual hex digest of the allowlisted file as
 * actualFileSha256Hex (non-empty). Empty actual + hasSha256 => Rejected (fail-closed).
 */
inline ManifestGateResult EvaluateModuleManifest(std::string_view descriptorId, bool manifestFound,
                                                 std::string const& json, bool requireManifest,
                                                 std::vector<std::string> const& actualHexInOrder = {})
{
    if (!manifestFound)
    {
        if (requireManifest)
            return RejectManifest(AETERNUM_ERR_NOT_FOUND, "manifest_missing");
        ManifestGateResult r;
        r.outcome = ManifestGateOutcome::LegacyNoManifest;
        r.status = AETERNUM_OK;
        return r;
    }

    ManifestGateResult r;
    char err[128]{};
    AeternumStatus const st = ParseModuleManifestJson(json, r.manifest, err, sizeof(err));
    if (st != AETERNUM_OK)
        return RejectManifest(st, std::string("manifest_invalid:") + err);

    if (r.manifest.id != descriptorId)
        return RejectManifest(AETERNUM_ERR_MANIFEST_INVALID, "manifest_id_mismatch:" + r.manifest.id);

    if (r.manifest.abiVersion != AETERNUM_MODULE_ABI_VERSION)
        return RejectManifest(AETERNUM_ERR_ABI_MISMATCH, "abi_mismatch:" + std::to_string(r.manifest.abiVersion));

    // IE-M3 Blocco1: this gate serves compiled-in (static IModule) modules only. A manifest that declares
    // "runtime":"native" belongs to the OS loader (native_runtime.hpp); never accept it for a static id.
    if (r.manifest.runtime == ManifestRuntime::Native)
        return RejectManifest(AETERNUM_ERR_MANIFEST_INVALID, "runtime_native_not_static_module");

    // M1 static host: boot-time load only. runtime-restart is equivalent at boot (MODULE_MANAGER_DESIGN);
    // hot-reloadable requires Level 3 (not implemented) => refuse instead of pretending.
    if (r.manifest.loadPolicy == ManifestLoadPolicy::HotReloadable)
        return RejectManifest(AETERNUM_ERR_UNSUPPORTED, "load_policy_unsupported:hot-reloadable");

    // D10 / Blocco5: verify every integrity entry (legacy single or files[]).
    std::vector<IntegrityFileEntry> const entries = CollectIntegrityEntries(r.manifest);
    if (!entries.empty())
    {
        // Validate paths first so empty/unsafe reasons win over size-mismatch unverified.
        for (IntegrityFileEntry const& e : entries)
        {
            if (e.path.empty())
                return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_file_required");
            if (!IsSafeIntegrityRelativePath(e.path))
                return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_file_unsafe:" + e.path);
        }
        if (actualHexInOrder.size() != entries.size())
            return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_hash_unverified");
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            IntegrityFileEntry const& e = entries[i];
            if (actualHexInOrder[i].empty())
                return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_hash_unverified:" + e.path);
            if (!Sha256HexEqual(e.sha256, actualHexInOrder[i]))
                return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_sha256_mismatch:" + e.path);
        }
    }

    r.outcome = ManifestGateOutcome::Accepted;
    r.status = AETERNUM_OK;
    return r;
}

inline ManifestGateResult EvaluateModuleManifestAt(std::filesystem::path const& dir, std::string_view moduleId,
                                                   bool requireManifest)
{
    if (!IsSafeModuleId(moduleId))
        return RejectManifest(AETERNUM_ERR_INVALID_ARGUMENT, "module_id_invalid");

    std::string json;
    std::string readError;
    ManifestReadStatus const rs = ReadModuleManifestFile(ModuleManifestPath(dir, moduleId), json, readError);
    if (rs == ManifestReadStatus::NotFound)
        return EvaluateModuleManifest(moduleId, false, json, requireManifest);
    if (rs != ManifestReadStatus::Ok)
        return RejectManifest(AETERNUM_ERR_MANIFEST_INVALID, readError);

    // Parse once to learn integrity entries, then hash each allowlisted file (boot-only).
    ModuleManifest preview;
    char err[128]{};
    AeternumStatus const pst = ParseModuleManifestJson(json, preview, err, sizeof(err));
    std::vector<std::string> actualHex;
    if (pst == AETERNUM_OK)
    {
        std::vector<IntegrityFileEntry> const entries = CollectIntegrityEntries(preview);
        if (!entries.empty())
        {
            // Pre-reject unsafe / missing path before hashing (reason carries path).
            for (IntegrityFileEntry const& e : entries)
            {
                if (e.path.empty())
                    return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_file_required");
                if (!IsSafeIntegrityRelativePath(e.path))
                    return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_file_unsafe:" + e.path);
            }
            actualHex.reserve(entries.size());
            for (IntegrityFileEntry const& e : entries)
            {
                std::filesystem::path const filePath = dir / std::filesystem::path(std::string(moduleId))
                    / std::filesystem::path(e.path);
                std::string hashError;
                std::string hex;
                Sha256FileStatus const hs = Sha256FileHex(filePath, hex, hashError);
                if (hs == Sha256FileStatus::NotFound)
                    return RejectManifest(AETERNUM_ERR_INTEGRITY, "integrity_file_missing:" + e.path);
                if (hs != Sha256FileStatus::Ok)
                    return RejectManifest(AETERNUM_ERR_INTEGRITY,
                        (hashError.empty() ? "integrity_hash_failed:" : (hashError + ":")) + e.path);
                actualHex.push_back(std::move(hex));
            }
        }
    }

    return EvaluateModuleManifest(moduleId, true, json, requireManifest, actualHex);
}

/**
 * Apply gate results to the planner input (same order/size as `planned`).
 * Rejected => enabled=false + disableReason. Returns true (abort boot) if a rejected module is critical.
 */
inline bool ApplyManifestGate(std::vector<PlannedModule>& planned, std::vector<ManifestGateResult> const& results,
                              std::string& abortMessage)
{
    std::size_t const n = planned.size() < results.size() ? planned.size() : results.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        if (results[i].outcome != ManifestGateOutcome::Rejected)
            continue;
        planned[i].enabled = false;
        planned[i].disableReason = results[i].reason;
        if (planned[i].critical)
        {
            abortMessage = "critical module '" + planned[i].id + "' rejected by manifest gate: " + results[i].reason;
            return true;
        }
    }
    return false;
}
} // namespace aeternum::sdk::internal

namespace aeternum::sdk
{
/** Host entry point with explicit manifest options (defined in ModuleHost.cpp). Boot-only. */
void HostDiscoverAndStart(internal::ManifestBootOptions const& options);
}

#endif /* AETERNUM_SDK_INTERNAL_MANIFEST_GATE_HPP */