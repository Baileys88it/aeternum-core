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
 * Aeternum M1 - module.manifest.json parser (on-boot-only loading path).
 * JSON library: rapidjson (dep/rapidjson - already in tree). No new deps.
 * Public API does not throw: all errors return AeternumStatus + message buffer.
 * integrity (D10 / M1-Blocco4/5):
 *   Legacy single: integrity.file + integrity.sha256 (64 hex).
 *   Multi-file:    integrity.files = [{ "path", "sha256" }, ...].
 * Both forms together => parse error integrity_ambiguous_form (safe; no silent skip).
 * Paths: relative under <manifestDir>/<id>/; validated by manifest_gate.
 * IE-M3 Blocco1 (native runtime, optional keys, defaults keep M1 behaviour):
 *   "runtime":  "static" (default) | "native"  - native = OS-level .dll/.so loaded by the M3 loader
 *   "library":  relative path of the native image under <manifestDir>/<id>/ (native only; must be hashed)
 *   "critical": bool (default false) - native modules only (static modules take it from the descriptor)
 */
#ifndef AETERNUM_SDK_MANIFEST_HPP
#define AETERNUM_SDK_MANIFEST_HPP

#include "aeternum/abi/AeternumABI.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

/*
 * Trinity dep/rapidjson INTERFACE sets RAPIDJSON_ASSERT=WPAssert on consumers of `game`.
 * Provide a local WPAssert when Errors.h is not included so the SDK header stays Trinity-free.
 */
#ifndef WPAssert
#  define WPAssert(x) assert(x)
#endif
#ifndef RAPIDJSON_HAS_STDSTRING
#  define RAPIDJSON_HAS_STDSTRING 1
#endif
#include "rapidjson/document.h"

namespace aeternum::sdk
{

enum class ManifestLoadPolicy : uint32_t
{
    OnBootOnly = AETERNUM_LOAD_POLICY_ON_BOOT_ONLY,
    RuntimeRestart = AETERNUM_LOAD_POLICY_RUNTIME_RESTART,
    HotReloadable = AETERNUM_LOAD_POLICY_HOT_RELOADABLE
};

enum class ManifestRuntime : uint32_t
{
    Static = 0,
    Native = 1
};

struct IntegrityFileEntry
{
    std::string path;   // relative allowlist path under <manifestDir>/<id>/
    std::string sha256; // 64 hex
};

struct ModuleManifest
{
    std::string id;
    uint32_t abiVersion = 0;
    ManifestLoadPolicy loadPolicy = ManifestLoadPolicy::OnBootOnly;
    // Legacy single-file form (integrity.file + integrity.sha256)
    std::string sha256;
    bool hasSha256 = false;
    std::string integrityFile;
    bool hasIntegrityFile = false;
    // Multi-file form (integrity.files array). Mutually exclusive with legacy form.
    std::vector<IntegrityFileEntry> integrityFiles;
    bool hasIntegrityFiles = false;
    // IE-M3 Blocco1: native runtime keys (absent => static, M1 behaviour unchanged)
    ManifestRuntime runtime = ManifestRuntime::Static;
    std::string library;
    bool hasLibrary = false;
    bool critical = false;
};

inline bool IsHexSha256(std::string const& s) noexcept
{
    if (s.size() != 64)
        return false;
    for (unsigned char c : s)
    {
        if (!std::isxdigit(c))
            return false;
    }
    return true;
}

inline bool ParseLoadPolicy(char const* text, ManifestLoadPolicy& out) noexcept
{
    if (!text)
        return false;
    if (std::strcmp(text, "on-boot-only") == 0) { out = ManifestLoadPolicy::OnBootOnly; return true; }
    if (std::strcmp(text, "runtime-restart") == 0) { out = ManifestLoadPolicy::RuntimeRestart; return true; }
    if (std::strcmp(text, "hot-reloadable") == 0) { out = ManifestLoadPolicy::HotReloadable; return true; }
    return false;
}

inline AeternumStatus ParseModuleManifestJson(char const* json, size_t jsonLen, ModuleManifest& out,
                                              char* errBuf, size_t errCap) noexcept
{
    auto setErr = [&](char const* msg) noexcept {
        if (errBuf && errCap > 0)
        {
            std::snprintf(errBuf, errCap, "%s", msg ? msg : "error");
            errBuf[errCap - 1] = '\0';
        }
    };

    try
    {
        if (!json || jsonLen == 0)
        {
            setErr("empty json");
            return AETERNUM_ERR_MANIFEST_INVALID;
        }

        rapidjson::Document doc;
        doc.Parse(json, jsonLen);
        if (doc.HasParseError() || !doc.IsObject())
        {
            setErr("json parse failed");
            return AETERNUM_ERR_MANIFEST_INVALID;
        }

        ModuleManifest tmp;
        if (!doc.HasMember("id") || !doc["id"].IsString() || doc["id"].GetStringLength() == 0)
        {
            setErr("missing id");
            return AETERNUM_ERR_MANIFEST_INVALID;
        }
        tmp.id.assign(doc["id"].GetString(), doc["id"].GetStringLength());

        if (!doc.HasMember("abiVersion") || !doc["abiVersion"].IsUint())
        {
            setErr("missing abiVersion");
            return AETERNUM_ERR_MANIFEST_INVALID;
        }
        tmp.abiVersion = doc["abiVersion"].GetUint();

        tmp.loadPolicy = ManifestLoadPolicy::OnBootOnly;
        if (doc.HasMember("loadPolicy"))
        {
            if (!doc["loadPolicy"].IsString())
            {
                setErr("loadPolicy type");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
            ManifestLoadPolicy pol;
            if (!ParseLoadPolicy(doc["loadPolicy"].GetString(), pol))
            {
                setErr("loadPolicy value");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
            tmp.loadPolicy = pol;
        }

        if (doc.HasMember("runtime"))
        {
            if (!doc["runtime"].IsString())
            {
                setErr("runtime type");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
            char const* rt = doc["runtime"].GetString();
            if (std::strcmp(rt, "static") == 0)
                tmp.runtime = ManifestRuntime::Static;
            else if (std::strcmp(rt, "native") == 0)
                tmp.runtime = ManifestRuntime::Native;
            else
            {
                setErr("runtime value");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
        }

        if (doc.HasMember("library"))
        {
            if (!doc["library"].IsString() || doc["library"].GetStringLength() == 0)
            {
                setErr("library type");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
            tmp.library.assign(doc["library"].GetString(), doc["library"].GetStringLength());
            tmp.hasLibrary = true;
        }

        if (doc.HasMember("critical"))
        {
            if (!doc["critical"].IsBool())
            {
                setErr("critical type");
                return AETERNUM_ERR_MANIFEST_INVALID;
            }
            tmp.critical = doc["critical"].GetBool();
        }

        if (doc.HasMember("integrity"))
        {
            if (!doc["integrity"].IsObject())
            {
                setErr("integrity type");
                return AETERNUM_ERR_INTEGRITY;
            }
            auto const& integ = doc["integrity"];

            bool const hasLegacySha = integ.HasMember("sha256");
            bool const hasLegacyFile = integ.HasMember("file");
            bool const hasFilesArr = integ.HasMember("files");

            // Safe precedence: legacy single (file+sha256) XOR files array. Both => reject.
            if (hasFilesArr && (hasLegacySha || hasLegacyFile))
            {
                setErr("integrity_ambiguous_form");
                return AETERNUM_ERR_INTEGRITY;
            }

            if (hasFilesArr)
            {
                if (!integ["files"].IsArray())
                {
                    setErr("integrity.files type");
                    return AETERNUM_ERR_INTEGRITY;
                }
                auto const& arr = integ["files"];
                if (arr.Empty())
                {
                    setErr("integrity.files empty");
                    return AETERNUM_ERR_INTEGRITY;
                }
                // Cap entry count (boot-only; keep DOM tiny).
                if (arr.Size() > 64u)
                {
                    setErr("integrity.files too many");
                    return AETERNUM_ERR_INTEGRITY;
                }
                std::vector<IntegrityFileEntry> entries;
                entries.reserve(arr.Size());
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
                {
                    if (!arr[i].IsObject())
                    {
                        setErr("integrity.files entry type");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    auto const& e = arr[i];
                    if (!e.HasMember("path") || !e["path"].IsString() || e["path"].GetStringLength() == 0)
                    {
                        setErr("integrity.files path");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    if (!e.HasMember("sha256") || !e["sha256"].IsString())
                    {
                        setErr("integrity.files sha256 type");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    std::string path(e["path"].GetString(), e["path"].GetStringLength());
                    std::string sha(e["sha256"].GetString(), e["sha256"].GetStringLength());
                    if (!IsHexSha256(sha))
                    {
                        setErr("integrity.files sha256 format");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    for (auto const& prev : entries)
                    {
                        if (prev.path == path)
                        {
                            setErr("integrity.files duplicate path");
                            return AETERNUM_ERR_INTEGRITY;
                        }
                    }
                    IntegrityFileEntry ent;
                    ent.path = std::move(path);
                    ent.sha256 = std::move(sha);
                    entries.push_back(std::move(ent));
                }
                tmp.integrityFiles = std::move(entries);
                tmp.hasIntegrityFiles = true;
            }
            else
            {
                if (hasLegacySha)
                {
                    if (!integ["sha256"].IsString())
                    {
                        setErr("sha256 type");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    std::string sha(integ["sha256"].GetString(), integ["sha256"].GetStringLength());
                    if (!IsHexSha256(sha))
                    {
                        setErr("sha256 format");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    tmp.sha256 = std::move(sha);
                    tmp.hasSha256 = true;
                }
                if (hasLegacyFile)
                {
                    if (!integ["file"].IsString() || integ["file"].GetStringLength() == 0)
                    {
                        setErr("integrity.file type");
                        return AETERNUM_ERR_INTEGRITY;
                    }
                    tmp.integrityFile.assign(integ["file"].GetString(), integ["file"].GetStringLength());
                    tmp.hasIntegrityFile = true;
                }
            }
        }

        out = std::move(tmp);
        if (errBuf && errCap > 0)
            errBuf[0] = '\0';
        return AETERNUM_OK;
    }
    catch (...)
    {
        setErr("internal");
        return AETERNUM_ERR_INTERNAL;
    }
}

inline AeternumStatus ParseModuleManifestJson(std::string const& json, ModuleManifest& out,
                                              char* errBuf, size_t errCap) noexcept
{
    return ParseModuleManifestJson(json.data(), json.size(), out, errBuf, errCap);
}

} // namespace aeternum::sdk

#endif /* AETERNUM_SDK_MANIFEST_HPP */