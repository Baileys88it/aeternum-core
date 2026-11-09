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
 * Aeternum M1-Blocco4 (D10) - Trinity-free SHA-256 of an allowlisted module file.
 * Uses OpenSSL EVP already in-tree (same primitive as Trinity::Crypto::SHA256).
 * Boot-only: may allocate; never call from World::Update / packet paths.
 */
#ifndef AETERNUM_SDK_INTERNAL_SHA256_FILE_HPP
#define AETERNUM_SDK_INTERNAL_SHA256_FILE_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <openssl/evp.h>

namespace aeternum::sdk::internal
{
inline constexpr std::size_t kSha256HexLen = 64u;
inline constexpr std::uintmax_t kMaxIntegrityFileBytes = 256ull * 1024ull * 1024ull;
inline constexpr std::size_t kIntegrityReadChunk = 64u * 1024u;

enum class Sha256FileStatus : unsigned char
{
    Ok = 0,
    NotFound,
    Error
};

/** Relative path under <manifestDir>/<id>/ — no absolute, no "..", no backslash. */
inline bool IsSafeIntegrityRelativePath(std::string_view path) noexcept
{
    if (path.empty() || path.size() > 240)
        return false;
    if (path.front() == '/' || path.front() == '\\')
        return false;
    if (path.size() >= 2 && path[1] == ':')
        return false;
    if (path.find('\\') != std::string_view::npos)
        return false;
    if (path.find('\0') != std::string_view::npos)
        return false;

    std::size_t i = 0;
    while (i < path.size())
    {
        if (path[i] == '/')
            return false; // empty segment
        std::size_t j = i;
        while (j < path.size() && path[j] != '/')
            ++j;
        std::string_view const seg = path.substr(i, j - i);
        if (seg.empty() || seg == "." || seg == "..")
            return false;
        for (char c : seg)
        {
            bool const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                || c == '.' || c == '_' || c == '-';
            if (!ok)
                return false;
        }
        if (j == path.size())
            break;
        i = j + 1;
        if (i == path.size())
            return false; // trailing slash
    }
    return true;
}

inline char NibbleToHexLower(unsigned v) noexcept
{
    return static_cast<char>(v < 10 ? ('0' + v) : ('a' + (v - 10)));
}

inline bool HexNibble(char c, unsigned& out) noexcept
{
    if (c >= '0' && c <= '9') { out = static_cast<unsigned>(c - '0'); return true; }
    if (c >= 'a' && c <= 'f') { out = static_cast<unsigned>(c - 'a' + 10); return true; }
    if (c >= 'A' && c <= 'F') { out = static_cast<unsigned>(c - 'A' + 10); return true; }
    return false;
}

/** Case-insensitive compare of two 64-char hex digests. */
inline bool Sha256HexEqual(std::string_view a, std::string_view b) noexcept
{
    if (a.size() != kSha256HexLen || b.size() != kSha256HexLen)
        return false;
    unsigned diff = 0;
    for (std::size_t i = 0; i < kSha256HexLen; ++i)
    {
        unsigned na = 0, nb = 0;
        if (!HexNibble(a[i], na) || !HexNibble(b[i], nb))
            return false;
        diff |= (na ^ nb);
    }
    return diff == 0;
}

inline void DigestToHexLower(std::array<unsigned char, 32> const& digest, std::string& outHex)
{
    outHex.resize(kSha256HexLen);
    for (std::size_t i = 0; i < digest.size(); ++i)
    {
        outHex[i * 2] = NibbleToHexLower(digest[i] >> 4);
        outHex[i * 2 + 1] = NibbleToHexLower(digest[i] & 0x0Fu);
    }
}

/**
 * Stream-hash a regular file to lowercase hex (64 chars).
 * Fail-closed on missing / not regular / too large / I/O / OpenSSL errors.
 */
inline Sha256FileStatus Sha256FileHex(std::filesystem::path const& path, std::string& outHex, std::string& error)
{
    outHex.clear();
    std::error_code ec;
    bool const exists = std::filesystem::exists(path, ec);
    if (ec)
    {
        error = "integrity_stat_failed";
        return Sha256FileStatus::Error;
    }
    if (!exists)
    {
        error = "integrity_file_missing";
        return Sha256FileStatus::NotFound;
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec)
    {
        error = "integrity_not_regular_file";
        return Sha256FileStatus::Error;
    }
    std::uintmax_t const size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        error = "integrity_stat_failed";
        return Sha256FileStatus::Error;
    }
    if (size > kMaxIntegrityFileBytes)
    {
        error = "integrity_file_too_large";
        return Sha256FileStatus::Error;
    }

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
    {
        error = "integrity_open_failed";
        return Sha256FileStatus::Error;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        error = "integrity_hash_init_failed";
        return Sha256FileStatus::Error;
    }
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        error = "integrity_hash_init_failed";
        return Sha256FileStatus::Error;
    }

    std::array<char, kIntegrityReadChunk> buf{};
    while (in)
    {
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        std::streamsize const got = in.gcount();
        if (got > 0)
        {
            if (EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(got)) != 1)
            {
                EVP_MD_CTX_free(ctx);
                error = "integrity_hash_update_failed";
                return Sha256FileStatus::Error;
            }
        }
    }
    if (in.bad())
    {
        EVP_MD_CTX_free(ctx);
        error = "integrity_read_failed";
        return Sha256FileStatus::Error;
    }

    std::array<unsigned char, 32> digest{};
    unsigned len = 0;
    if (EVP_DigestFinal_ex(ctx, digest.data(), &len) != 1 || len != digest.size())
    {
        EVP_MD_CTX_free(ctx);
        error = "integrity_hash_final_failed";
        return Sha256FileStatus::Error;
    }
    EVP_MD_CTX_free(ctx);

    DigestToHexLower(digest, outHex);
    error.clear();
    return Sha256FileStatus::Ok;
}

/** Hash in-memory bytes (Catch2 compare path without temp files). */
inline bool Sha256BytesHex(void const* data, std::size_t len, std::string& outHex) noexcept
{
    outHex.clear();
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        return false;
    bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1
        && EVP_DigestUpdate(ctx, data, len) == 1;
    std::array<unsigned char, 32> digest{};
    unsigned dlen = 0;
    ok = ok && EVP_DigestFinal_ex(ctx, digest.data(), &dlen) == 1 && dlen == digest.size();
    EVP_MD_CTX_free(ctx);
    if (!ok)
        return false;
    DigestToHexLower(digest, outHex);
    return true;
}
} // namespace aeternum::sdk::internal

#endif /* AETERNUM_SDK_INTERNAL_SHA256_FILE_HPP */