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
 * Aeternum IE-M2 Blocco1 - boot-only Protocol Profile loader (forever/client_profiles/<id>/).
 *
 *  1. manifest.json: profile_id, client_build, load_policy == "on-boot-only",
 *     integrity: files[{path,sha256}] XOR legacy {file,sha256} (same rules/reasons as M1 module manifests).
 *  2. Every integrity entry hashed with M1 Sha256FileHex (fail-closed, reason names the path).
 *  3. opcodes/opcodes.json + opcodes/unknown_policy.json MUST be integrity-covered.
 *  4. Rows parsed through the rapidjson DOM (temporary) straight into the dense AdapterOpcodeTable.
 * May allocate (boot). Never call from packet paths. Not wired into WorldSocket in Blocco1.
 */
#ifndef AETERNUM_PROTOCOL_ADAPTER_PROFILE_LOADER_HPP
#define AETERNUM_PROTOCOL_ADAPTER_PROFILE_LOADER_HPP

#include "aeternum/protocol_adapter/opcode_table.hpp"
#include "aeternum/sdk/internal/sha256_file.hpp"
#include "aeternum/sdk/manifest.hpp"  // IsHexSha256, IntegrityFileEntry, rapidjson (+ WPAssert fallback)

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace aeternum::protocol_adapter
{
inline constexpr char const* kProfileManifestFile = "manifest.json";
inline constexpr char const* kOpcodesFile = "opcodes/opcodes.json";
inline constexpr char const* kUnknownPolicyFile = "opcodes/unknown_policy.json";
inline constexpr std::uintmax_t kMaxProfileManifestBytes = 64u * 1024u;
inline constexpr std::uintmax_t kMaxOpcodesJsonBytes = 4u * 1024u * 1024u;

struct ProfileLoadReport
{
    bool ok = false;
    std::string reason;  // empty on success; "<code>[:<detail>]" on failure
    std::string profileId;
    std::uint32_t clientBuild = 0;
    std::uint32_t cmsgRows = 0;
    std::uint32_t smsgRows = 0;
    std::uint32_t ignoredKnown = 0;
    std::uint32_t cmsgAliases = 0;
    std::uint32_t smsgWireCollisions = 0;
    std::uint32_t smsgBlocked = 0;  // Blocco5 kFlagBlocked rows
    std::uint32_t smsgStub = 0;     // Blocco5 kFlagStub rows (= stub payloads loaded)
    std::uint32_t slotsCmsgWire = 0;
    std::uint32_t slotsCmsgLogical = 0;
    std::uint32_t slotsSmsgLogical = 0;
    std::uint32_t slotsSmsgWire = 0;
    std::vector<sdk::IntegrityFileEntry> verified;
};

namespace detail
{
inline bool ReadFileCapped(std::filesystem::path const& p, std::uintmax_t cap, std::string& out, std::string& err)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(p, ec) || ec) { err = "file_missing"; return false; }
    std::uintmax_t const size = std::filesystem::file_size(p, ec);
    if (ec) { err = "file_stat_failed"; return false; }
    if (size > cap) { err = "file_too_large"; return false; }
    std::ifstream in(p, std::ios::in | std::ios::binary);
    if (!in) { err = "file_open_failed"; return false; }
    out.assign(static_cast<std::size_t>(size), '\0');
    if (size && !in.read(out.data(), static_cast<std::streamsize>(size))) { err = "file_read_failed"; return false; }
    return true;
}

inline bool ParseHexOpcode(rapidjson::Value const& v, std::uint32_t& out) noexcept
{
    if (!v.IsString())
        return false;
    char const* s = v.GetString();
    unsigned long long x = 0;
    if (std::sscanf(s, "%llx", &x) != 1 || x > kOpcodeKeyMax)
        return false;
    out = static_cast<std::uint32_t>(x);
    return true;
}

/** M1 rules: files[] XOR legacy {file,sha256}; both -> integrity_ambiguous_form; dup paths rejected; cap 64. */
inline bool ParseIntegrity(rapidjson::Value const& doc, std::vector<sdk::IntegrityFileEntry>& out, std::string& reason)
{
    if (!doc.HasMember("integrity")) { reason = "integrity_missing"; return false; }
    rapidjson::Value const& integ = doc["integrity"];
    if (!integ.IsObject()) { reason = "integrity_type"; return false; }
    bool const hasFiles = integ.HasMember("files");
    bool const hasSha = integ.HasMember("sha256");
    bool const hasFile = integ.HasMember("file");
    if (hasFiles && (hasSha || hasFile)) { reason = "integrity_ambiguous_form"; return false; }
    out.clear();
    if (hasFiles)
    {
        rapidjson::Value const& arr = integ["files"];
        if (!arr.IsArray() || arr.Empty()) { reason = "integrity_files_invalid"; return false; }
        if (arr.Size() > 64u) { reason = "integrity_files_too_many"; return false; }
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            rapidjson::Value const& e = arr[i];
            if (!e.IsObject() || !e.HasMember("path") || !e["path"].IsString() || !e.HasMember("sha256") || !e["sha256"].IsString())
            {
                reason = "integrity_files_entry_invalid";
                return false;
            }
            sdk::IntegrityFileEntry ent;
            ent.path.assign(e["path"].GetString(), e["path"].GetStringLength());
            ent.sha256.assign(e["sha256"].GetString(), e["sha256"].GetStringLength());
            if (!sdk::IsHexSha256(ent.sha256)) { reason = "integrity_sha256_format:" + ent.path; return false; }
            for (auto const& prev : out)
                if (prev.path == ent.path) { reason = "integrity_duplicate_path:" + ent.path; return false; }
            out.push_back(std::move(ent));
        }
        return true;
    }
    if (!(hasSha && hasFile) || !integ["file"].IsString() || !integ["sha256"].IsString())
    {
        reason = "integrity_legacy_incomplete";
        return false;
    }
    sdk::IntegrityFileEntry ent;
    ent.path.assign(integ["file"].GetString(), integ["file"].GetStringLength());
    ent.sha256.assign(integ["sha256"].GetString(), integ["sha256"].GetStringLength());
    if (!sdk::IsHexSha256(ent.sha256)) { reason = "integrity_sha256_format:" + ent.path; return false; }
    out.push_back(std::move(ent));
    return true;
}

struct Row
{
    std::uint32_t wire = 0, logical = 0;
    std::uint8_t status = 0;
    std::uint16_t flags = 0;
};

inline bool ParseRows(rapidjson::Value const& doc, char const* key, std::vector<Row>& out, std::string& reason)
{
    if (!doc.HasMember(key) || !doc[key].IsArray()) { reason = std::string("opcodes_section_missing:") + key; return false; }
    rapidjson::Value const& arr = doc[key];
    out.clear();
    out.reserve(arr.Size());
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        rapidjson::Value const& r = arr[i];
        if (!r.IsArray() || r.Size() != 5 || !r[0].IsUint() || !r[1].IsUint() || !r[2].IsUint() || !r[3].IsUint() || !r[4].IsString()
            || r[0].GetUint() > kOpcodeKeyMax || r[1].GetUint() > kOpcodeKeyMax || r[2].GetUint() > 6u || r[3].GetUint() > 0x7FFFu)
        {
            reason = std::string("opcodes_row_invalid:") + key + ":" + std::to_string(i);
            return false;
        }
        out.push_back(Row{ r[0].GetUint(), r[1].GetUint(), static_cast<std::uint8_t>(r[2].GetUint()), static_cast<std::uint16_t>(r[3].GetUint()) });
    }
    return true;
}

inline bool ExpectCount(rapidjson::Value const& counts, char const* key, std::uint32_t actual, std::string& reason)
{
    if (!counts.HasMember(key) || !counts[key].IsUint() || counts[key].GetUint() != actual)
    {
        reason = std::string("opcodes_count_mismatch:") + key;
        return false;
    }
    return true;
}

inline OpcodeClass ClientClass(std::uint16_t flags) noexcept
{
    if (flags & kFlagIgnoredKnown) return OpcodeClass::IgnoredKnown;
    if (flags & kFlagHandled) return OpcodeClass::Handled;
    return OpcodeClass::KnownUnhandled;
}

inline OpcodeClass ServerClass(std::uint16_t flags) noexcept
{
    return (flags & kFlagBlocked) ? OpcodeClass::KnownUnhandled : OpcodeClass::Handled;
}

inline char const* InsertReason(DenseOpcodeMap::InsertResult r) noexcept
{
    switch (r)
    {
        case DenseOpcodeMap::InsertResult::Duplicate: return "opcodes_duplicate_key:";
        case DenseOpcodeMap::InsertResult::OutOfRange: return "opcodes_key_out_of_range:";
        default: return "opcodes_table_not_laid_out:";
    }
}

/** Fill one dense map: NoteKey -> Layout -> Insert. skipAlias drops alias-flagged rows (reverse maps). */
inline bool BuildMap(DenseOpcodeMap& map, std::vector<Row> const& rows, bool keyIsWire, bool skipAlias, bool client,
                     char const* mapName, std::uint32_t& slots, std::string& reason)
{
    map.Reset();
    for (Row const& r : rows)
    {
        if (skipAlias && (r.flags & kFlagAlias))
            continue;
        map.NoteKey(keyIsWire ? r.wire : r.logical);
    }
    if (!map.Layout()) { reason = std::string("opcodes_table_capacity:") + mapName; return false; }
    for (Row const& r : rows)
    {
        if (skipAlias && (r.flags & kFlagAlias))
            continue;
        OpcodeEntry e;
        e.key = keyIsWire ? r.wire : r.logical;
        e.value = keyIsWire ? r.logical : r.wire;
        e.flags = r.flags;
        e.status = r.status;
        e.cls = client ? ClientClass(r.flags) : ServerClass(r.flags);
        DenseOpcodeMap::InsertResult const ir = map.Insert(e);
        if (ir != DenseOpcodeMap::InsertResult::Ok)
        {
            char hex[16];
            std::snprintf(hex, sizeof(hex), "0x%06X", e.key);
            reason = std::string(InsertReason(ir)) + mapName + ":" + hex;
            return false;
        }
    }
    slots = map.SlotsUsed();
    return true;
}

/** Blocco5: lowercase/uppercase hex -> bytes (even length, <= kMaxStubPayloadBytes). */
inline bool ParseStubHex(rapidjson::Value const& v, std::uint8_t (&out)[kMaxStubPayloadBytes], std::uint32_t& size) noexcept
{
    if (!v.IsString())
        return false;
    std::uint32_t const len = v.GetStringLength();
    if (len % 2u || len / 2u > kMaxStubPayloadBytes)
        return false;
    char const* s = v.GetString();
    auto nib = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    };
    for (std::uint32_t i = 0; i < len / 2u; ++i)
    {
        int const hi = nib(s[2 * i]), lo = nib(s[2 * i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    size = len / 2u;
    return true;
}

inline bool ParseUnknownPolicy(std::string const& json, std::string const& profileId, UnknownPolicy& out, std::string& reason)
{
    rapidjson::Document d;
    d.Parse(json.data(), json.size());
    if (d.HasParseError() || !d.IsObject()) { reason = "unknown_policy_invalid:json"; return false; }
    if (!d.HasMember("profile_id") || !d["profile_id"].IsString() || profileId != d["profile_id"].GetString())
    {
        reason = "unknown_policy_invalid:profile_id";
        return false;
    }
    if (!d.HasMember("unknown") || !d["unknown"].IsObject()) { reason = "unknown_policy_invalid:unknown"; return false; }
    rapidjson::Value const& u = d["unknown"];
    auto getU = [&](char const* k, std::uint32_t lo, std::uint32_t hi, std::uint32_t& v) -> bool {
        if (!u.HasMember(k) || !u[k].IsUint() || u[k].GetUint() < lo || u[k].GetUint() > hi)
            return false;
        v = u[k].GetUint();
        return true;
    };
    UnknownPolicy p;
    if (!getU("window_ms", 1u, 3600000u, p.windowMs)) { reason = "unknown_policy_invalid:window_ms"; return false; }
    if (!getU("threshold_per_window", 0u, 1000000u, p.thresholdPerWindow)) { reason = "unknown_policy_invalid:threshold_per_window"; return false; }
    if (!getU("max_errors_per_window", 0u, 1000u, p.maxErrorsPerWindow)) { reason = "unknown_policy_invalid:max_errors_per_window"; return false; }
    out = p;
    return true;
}
} // namespace detail

/**
 * Load + verify + build. On failure the table is Reset() (never half-filled) and report.reason is set.
 */
inline bool LoadProtocolProfile(std::filesystem::path const& profileDir, AdapterOpcodeTable& table, ProfileLoadReport& rep)
{
    rep = ProfileLoadReport{};
    table.Reset();
    auto fail = [&](std::string reason) {
        table.Reset();
        rep.ok = false;
        rep.reason = std::move(reason);
        return false;
    };

    // 1. manifest
    std::string text, err;
    if (!detail::ReadFileCapped(profileDir / kProfileManifestFile, kMaxProfileManifestBytes, text, err))
        return fail("manifest_" + err);
    rapidjson::Document man;
    man.Parse(text.data(), text.size());
    if (man.HasParseError() || !man.IsObject())
        return fail("manifest_invalid:json");
    if (!man.HasMember("profile_id") || !man["profile_id"].IsString() || man["profile_id"].GetStringLength() == 0)
        return fail("manifest_invalid:profile_id");
    rep.profileId.assign(man["profile_id"].GetString(), man["profile_id"].GetStringLength());
    if (!man.HasMember("client_build") || !man["client_build"].IsUint())
        return fail("manifest_invalid:client_build");
    rep.clientBuild = man["client_build"].GetUint();
    if (!man.HasMember("load_policy") || !man["load_policy"].IsString() || std::string_view(man["load_policy"].GetString()) != "on-boot-only")
        return fail("load_policy_unsupported");

    // 2. integrity (M1 rules + M1 hashing)
    std::vector<sdk::IntegrityFileEntry> entries;
    std::string reason;
    if (!detail::ParseIntegrity(man, entries, reason))
        return fail(reason);
    for (auto const& e : entries)
    {
        if (!sdk::internal::IsSafeIntegrityRelativePath(e.path))
            return fail("integrity_file_unsafe:" + e.path);
        std::string actual, herr;
        sdk::internal::Sha256FileStatus const st = sdk::internal::Sha256FileHex(profileDir / std::filesystem::path(e.path), actual, herr);
        if (st == sdk::internal::Sha256FileStatus::NotFound)
            return fail("integrity_file_missing:" + e.path);
        if (st != sdk::internal::Sha256FileStatus::Ok)
            return fail((herr.empty() ? std::string("integrity_hash_failed") : herr) + ":" + e.path);
        if (!sdk::internal::Sha256HexEqual(actual, e.sha256))
            return fail("integrity_sha256_mismatch:" + e.path);
    }
    for (char const* required : { kOpcodesFile, kUnknownPolicyFile })
    {
        bool covered = false;
        for (auto const& e : entries)
            covered = covered || e.path == required;
        if (!covered)
            return fail(std::string("integrity_uncovered:") + required);
    }
    rep.verified = entries;

    // 3. unknown policy
    if (!detail::ReadFileCapped(profileDir / kUnknownPolicyFile, kMaxProfileManifestBytes, text, err))
        return fail("unknown_policy_" + err);
    UnknownPolicy policy;
    if (!detail::ParseUnknownPolicy(text, rep.profileId, policy, reason))
        return fail(reason);

    // 4. opcode rows
    if (!detail::ReadFileCapped(profileDir / kOpcodesFile, kMaxOpcodesJsonBytes, text, err))
        return fail("opcodes_" + err);
    rapidjson::Document op;
    op.Parse(text.data(), text.size());
    if (op.HasParseError() || !op.IsObject())
        return fail("opcodes_invalid:json");
    if (!op.HasMember("format") || !op["format"].IsUint() || op["format"].GetUint() != 1u)
        return fail("opcodes_invalid:format");
    if (!op.HasMember("profile_id") || !op["profile_id"].IsString() || rep.profileId != op["profile_id"].GetString())
        return fail("opcodes_invalid:profile_id");
    if (!op.HasMember("client_build") || !op["client_build"].IsUint() || op["client_build"].GetUint() != rep.clientBuild)
        return fail("opcodes_invalid:client_build");
    if (!op.HasMember("counts") || !op["counts"].IsObject())
        return fail("opcodes_invalid:counts");

    std::vector<detail::Row> cmsg, smsg;
    if (!detail::ParseRows(op, "cmsg", cmsg, reason) || !detail::ParseRows(op, "smsg", smsg, reason))
        return fail(reason);

    std::uint32_t ignored = 0, aliases = 0, collisions = 0;
    for (auto const& r : cmsg)
    {
        if (r.flags & kFlagAlias) ++aliases;
        else if (r.flags & kFlagIgnoredKnown) ++ignored;
    }
    for (auto const& r : smsg)
        if (r.flags & kFlagAlias) ++collisions;

    rapidjson::Value const& counts = op["counts"];
    if (!detail::ExpectCount(counts, "cmsg", static_cast<std::uint32_t>(cmsg.size()), reason)
        || !detail::ExpectCount(counts, "smsg", static_cast<std::uint32_t>(smsg.size()), reason)
        || !detail::ExpectCount(counts, "ignored_known", ignored, reason)
        || !detail::ExpectCount(counts, "cmsg_aliases", aliases, reason)
        || !detail::ExpectCount(counts, "smsg_wire_collisions", collisions, reason))
        return fail(reason);

    // ignored_known section must match the flagged rows (logical ids)
    if (!op.HasMember("ignored_known") || !op["ignored_known"].IsArray() || op["ignored_known"].Size() != ignored)
        return fail("opcodes_invalid:ignored_known");
    for (rapidjson::SizeType i = 0; i < op["ignored_known"].Size(); ++i)
    {
        rapidjson::Value const& e = op["ignored_known"][i];
        std::uint32_t logical = 0;
        if (!e.IsObject() || !e.HasMember("logical") || !detail::ParseHexOpcode(e["logical"], logical))
            return fail("opcodes_invalid:ignored_known");
        bool found = false;
        for (auto const& r : cmsg)
            found = found || (r.logical == logical && (r.flags & kFlagIgnoredKnown));
        if (!found)
            return fail("opcodes_invalid:ignored_known_row");
    }

    // 4b. IE-M2 Blocco5: SMSG send policy (replaces ClassicOpcodes::IsServerOpcodeBlocked / GetServerStubPayload).
    // kFlagBlocked rows are never sent; every kFlagStub row needs exactly one smsg_stub_payloads entry and vice versa.
    std::uint32_t smsgBlocked = 0, smsgStub = 0;
    for (auto const& r : smsg)
    {
        if ((r.flags & kFlagBlocked) && (r.flags & kFlagStub))
            return fail("opcodes_invalid:smsg_blocked_and_stub");
        if (r.flags & kFlagBlocked) ++smsgBlocked;
        if (r.flags & kFlagStub) ++smsgStub;
    }
    if (!detail::ExpectCount(counts, "smsg_blocked", smsgBlocked, reason) || !detail::ExpectCount(counts, "smsg_stub", smsgStub, reason))
        return fail(reason);
    if (smsgStub > kMaxStubPayloads)
        return fail("opcodes_stub_capacity");
    if (!op.HasMember("smsg_stub_payloads") || !op["smsg_stub_payloads"].IsArray() || op["smsg_stub_payloads"].Size() != smsgStub)
        return fail("opcodes_invalid:smsg_stub_payloads");
    struct PendingStub { std::uint32_t logical = 0; std::uint32_t size = 0; std::uint8_t bytes[kMaxStubPayloadBytes] = {}; };
    std::vector<PendingStub> stubs(smsgStub);
    for (rapidjson::SizeType i = 0; i < op["smsg_stub_payloads"].Size(); ++i)
    {
        rapidjson::Value const& e = op["smsg_stub_payloads"][i];
        PendingStub& ps = stubs[i];
        if (!e.IsObject() || !e.HasMember("logical") || !detail::ParseHexOpcode(e["logical"], ps.logical))
            return fail("opcodes_invalid:smsg_stub_payloads");
        if (!e.HasMember("payload_hex") || !detail::ParseStubHex(e["payload_hex"], ps.bytes, ps.size))
            return fail("opcodes_invalid:smsg_stub_payload_hex");
        bool found = false;
        for (auto const& r : smsg)
            found = found || (r.logical == ps.logical && (r.flags & kFlagStub));
        if (!found)
            return fail("opcodes_invalid:smsg_stub_row");
    }

    // 5. dense tables
    if (!detail::BuildMap(table.cmsgWireToLogical, cmsg, true, false, true, "cmsg_wire", rep.slotsCmsgWire, reason)
        || !detail::BuildMap(table.cmsgLogicalToWire, cmsg, false, true, true, "cmsg_logical", rep.slotsCmsgLogical, reason)
        || !detail::BuildMap(table.smsgLogicalToWire, smsg, false, false, false, "smsg_logical", rep.slotsSmsgLogical, reason)
        || !detail::BuildMap(table.smsgWireToLogical, smsg, true, true, false, "smsg_wire", rep.slotsSmsgWire, reason))
        return fail(reason);

    for (auto const& ps : stubs)
        if (!table.AddStubPayload(ps.logical, ps.bytes, ps.size))
            return fail("opcodes_invalid:smsg_stub_duplicate");

    table.SetUnknownPolicy(policy);
    table.MarkLoaded(true);
    rep.cmsgRows = static_cast<std::uint32_t>(cmsg.size());
    rep.smsgRows = static_cast<std::uint32_t>(smsg.size());
    rep.ignoredKnown = ignored;
    rep.cmsgAliases = aliases;
    rep.smsgWireCollisions = collisions;
    rep.smsgBlocked = smsgBlocked;
    rep.smsgStub = smsgStub;
    rep.ok = true;
    return true;
}
} // namespace aeternum::protocol_adapter

#endif /* AETERNUM_PROTOCOL_ADAPTER_PROFILE_LOADER_HPP */
