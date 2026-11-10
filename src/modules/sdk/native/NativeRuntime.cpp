#include "aeternum/sdk/internal/native_send.hpp"
#include "aeternum/sdk/internal/native_ecs.hpp"
#include "aeternum/sdk/internal/data_hooks.hpp"
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
 * Aeternum IE-M3 Blocco1 - OS-level native module loader (see native_runtime.hpp for the contract).
 * Boot/shutdown only. Not Trinity WITH_DYNAMIC_LINKING / ScriptReloadMgr (T2).
 * IE-M3 Blocco2: module slots + crossing TLS around every call, HostAlloc/HostFree, packet hook staging in on_init,
 * publish + freeze at the end of boot, tainted => 60 s graceful shutdown (native_host.hpp).
 */
#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#  include <dlfcn.h>
#  include <fcntl.h>
#  include <link.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#include "aeternum/sdk/internal/native_runtime.hpp"
#include "aeternum/sdk/internal/native_host.hpp"
#include "aeternum/sdk/internal/manifest_gate.hpp"
#include "aeternum/sdk/internal/sha256_file.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <system_error>
#include <thread>
#include <utility>
#include <memory>

namespace aeternum::sdk::internal::native
{
namespace
{
namespace fs = std::filesystem;

std::atomic<std::uint64_t> g_mapCalls{ 0 };
std::atomic<NativeLogFn> g_hostLog{ nullptr };

void Emit(NativeLogFn fn, NativeLogLevel level, std::string const& msg)
{
    if (fn)
        fn(level, msg.c_str());
}

// ---------------- Host FT (ABI v1 + appended fields, struct_size = sizeof) ----------------
// Called by module code: no allocation, no exception can escape (fixed buffers only).
void HostLogLevel(NativeLogLevel level, AeternumStringView msg) noexcept
{
    NativeLogFn fn = g_hostLog.load();
    if (!fn || !msg.data)
        return;
    char buf[512];
    std::uint32_t const n = msg.len < sizeof(buf) - 1 ? msg.len : static_cast<std::uint32_t>(sizeof(buf) - 1);
    std::memcpy(buf, msg.data, n);
    buf[n] = '\0';
    fn(level, buf);
}

void AETERNUM_ABI_CALL AeternumHostLogInfo(AeternumStringView msg)
{
    HostLogLevel(NativeLogLevel::Info, msg);
}

void AETERNUM_ABI_CALL AeternumHostLogError(AeternumStringView msg)
{
    HostLogLevel(NativeLogLevel::Error, msg);
}

AeternumStatus AETERNUM_ABI_CALL AeternumHostDbAsyncQuery(
    AeternumStringView, void (AETERNUM_ABI_CALL*)(void*, AeternumStatus, void const*, std::uint32_t), void*)
{
    // Not wired in Blocco1 (DB MPSC ring drain is a later block). Never sync DB on the game thread (P-b/F4).
    return AETERNUM_ERR_UNSUPPORTED;
}

AeternumHostFT const g_hostFT = {
    static_cast<std::uint32_t>(sizeof(AeternumHostFT)),
    &AeternumHostLogInfo,
    &AeternumHostDbAsyncQuery,
    &AeternumHostLogError,
    &AbiRuntimeTainted,
    // IE-M3 Blocco2 (appended, struct_size growth only)
    &AbiModuleSelf,
    &AbiHostAlloc,
    &AbiHostFree,
    &AbiSubscribePacket,
    &AbiSubscribeCombat,
    &AbiSubscribeSpell,
    &AbiSubscribeDataLoaded,
    &AbiGetCreatureHP,
    &AbiSetCreatureHP,
    &AbiGetItemStat,
    &AbiSetItemStat,
    &AbiGetWeaponDamage,
    &AbiSetWeaponDamage,
    &AbiSendPacket,
    &AbiGetTelemetryDump,
    &AbiGetSpellRecovery, &AbiSetSpellRecovery, &AbiGetLootChance, &AbiSetLootChance,
    &AbiPersistentState,
    &AbiEcsRegister, &AbiEcsUnregister, &AbiEcsRead,
};
static_assert(sizeof(AeternumHostFT) == AETERNUM_FT_FIELD_END(AeternumHostFT, aeternum_ecs_read_entity),
              "Host FT initializer must cover every appended field");

/** F3: in_flight == 0 wait budget at shutdown before a module image may be released. */
constexpr std::chrono::milliseconds kInFlightDrainBudget{ 5000 };

// ---------------- path helpers ----------------
bool PathEquals(fs::path const& a, fs::path const& b)
{
#if defined(_WIN32)
    std::wstring x = a.lexically_normal().native();
    std::wstring y = b.lexically_normal().native();
    while (x.size() > 3 && (x.back() == L'\\' || x.back() == L'/')) x.pop_back();
    while (y.size() > 3 && (y.back() == L'\\' || y.back() == L'/')) y.pop_back();
    if (x.size() > 0x7FFFFFFF || y.size() > 0x7FFFFFFF)
        return false;
    return CompareStringOrdinal(x.c_str(), static_cast<int>(x.size()), y.c_str(), static_cast<int>(y.size()), TRUE)
        == CSTR_EQUAL;
#else
    return a.lexically_normal() == b.lexically_normal();
#endif
}

/** true if `p` is a symlink, a junction / any reparse point, or cannot be inspected (fail closed). */
bool IsReparseOrSymlink(fs::path const& p)
{
    std::error_code ec;
    fs::file_status const st = fs::symlink_status(p, ec);
    if (ec || !fs::exists(st))
        return true;
    if (fs::is_symlink(st))
        return true;
#if defined(_WIN32)
    DWORD const attrs = GetFileAttributesW(p.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT))
        return true;
#endif
    return false;
}

bool HasSuffixCaseInsensitive(std::string const& s, char const* suffix)
{
    std::size_t const n = std::strlen(suffix);
    if (s.size() <= n)
        return false;
    for (std::size_t i = 0; i < n; ++i)
    {
        char a = s[s.size() - n + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

bool InImage(AeternumGuardImage const& img, void const* p) noexcept
{
    auto const a = reinterpret_cast<std::uintptr_t>(p);
    return img.base != 0 && a >= img.base && a < img.end;
}

/** Copy a NUL-terminated string that MUST live inside the module image (static storage). */
bool CopyImageString(AeternumGuardImage const& img, char const* s, std::size_t maxLen, std::string& out)
{
    if (!s || !InImage(img, s))
        return false;
    auto const start = reinterpret_cast<std::uintptr_t>(s);
    for (std::size_t i = 0; i < maxLen && start + i < img.end; ++i)
    {
        if (s[i] == '\0')
        {
            out.assign(s, i);
            return true;
        }
    }
    return false;
}

std::string Hex32(std::uint32_t v)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(v));
    return buf;
}

std::string HexPtr(std::uintptr_t v)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%llX", static_cast<unsigned long long>(v));
    return buf;
}

// ---------------- SHA-256 through an already-open (locked) file ----------------
class EvpSha256
{
public:
    EvpSha256() : _ctx(EVP_MD_CTX_new())
    {
        _ok = _ctx && EVP_DigestInit_ex(_ctx, EVP_sha256(), nullptr) == 1;
    }
    ~EvpSha256() { if (_ctx) EVP_MD_CTX_free(_ctx); }
    EvpSha256(EvpSha256 const&) = delete;
    EvpSha256& operator=(EvpSha256 const&) = delete;

    bool Update(void const* data, std::size_t len)
    {
        _ok = _ok && EVP_DigestUpdate(_ctx, data, len) == 1;
        return _ok;
    }
    bool Final(std::string& hex)
    {
        std::array<unsigned char, 32> digest{};
        unsigned len = 0;
        _ok = _ok && EVP_DigestFinal_ex(_ctx, digest.data(), &len) == 1 && len == digest.size();
        if (_ok)
            DigestToHexLower(digest, hex);
        return _ok;
    }
    bool Ok() const { return _ok; }

private:
    EVP_MD_CTX* _ctx;
    bool _ok = false;
};

void ReleaseLock(NativeModuleRecord& rec) noexcept
{
    if (!rec.fileLock)
        return;
#if defined(_WIN32)
    CloseHandle(reinterpret_cast<HANDLE>(rec.fileLock));
#else
    close(static_cast<int>(rec.fileLock - 1));
#endif
    rec.fileLock = 0;
}

/**
 * Open the native image deny-write (Windows) / O_NOFOLLOW (POSIX), verify it is the expected canonical
 * regular file, and hash it through that handle. The handle stays open until the image is mapped.
 */
bool LockAndHashImage(fs::path const& canonicalPath, NativeModuleRecord& rec, std::string& hex, std::string& why)
{
    ReleaseLock(rec);
    std::vector<unsigned char> buf(kIntegrityReadChunk);
    EvpSha256 sha;
    if (!sha.Ok())
    {
        why = "native_hash_init_failed";
        return false;
    }
#if defined(_WIN32)
    HANDLE h = CreateFileW(canonicalPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (h == INVALID_HANDLE_VALUE)
    {
        why = "native_image_lock_failed:" + std::to_string(GetLastError());
        return false;
    }
    rec.fileLock = reinterpret_cast<std::uintptr_t>(h);

    FILE_ATTRIBUTE_TAG_INFO tag{};
    if (!GetFileInformationByHandleEx(h, FileAttributeTagInfo, &tag, sizeof(tag))
        || (tag.FileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))
    {
        ReleaseLock(rec);
        why = "native_image_reparse";
        return false;
    }
    std::vector<wchar_t> finalPath(32768);
    DWORD const n = GetFinalPathNameByHandleW(h, finalPath.data(), static_cast<DWORD>(finalPath.size()),
                                              FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (n == 0 || n >= finalPath.size())
    {
        ReleaseLock(rec);
        why = "native_image_final_path_failed";
        return false;
    }
    std::wstring fp(finalPath.data(), n);
    if (fp.rfind(L"\\\\?\\", 0) == 0 && fp.rfind(L"\\\\?\\UNC\\", 0) != 0)
        fp.erase(0, 4);
    if (!PathEquals(fs::path(fp), canonicalPath))
    {
        ReleaseLock(rec);
        why = "native_image_path_mismatch";
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0
        || static_cast<std::uintmax_t>(size.QuadPart) > kMaxIntegrityFileBytes)
    {
        ReleaseLock(rec);
        why = "native_image_too_large";
        return false;
    }
    for (;;)
    {
        DWORD got = 0;
        if (!ReadFile(h, buf.data(), static_cast<DWORD>(buf.size()), &got, nullptr))
        {
            ReleaseLock(rec);
            why = "native_image_read_failed";
            return false;
        }
        if (got == 0)
            break;
        if (!sha.Update(buf.data(), got))
            break;
    }
#else
    int const fd = open(canonicalPath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
    {
        why = "native_image_open_failed";
        return false;
    }
    rec.fileLock = static_cast<std::uintptr_t>(fd) + 1u;
    struct stat st{};
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)
        || static_cast<std::uintmax_t>(st.st_size) > kMaxIntegrityFileBytes)
    {
        ReleaseLock(rec);
        why = "native_image_not_regular_file";
        return false;
    }
    for (;;)
    {
        ssize_t const got = read(fd, buf.data(), buf.size());
        if (got < 0)
        {
            ReleaseLock(rec);
            why = "native_image_read_failed";
            return false;
        }
        if (got == 0)
            break;
        if (!sha.Update(buf.data(), static_cast<std::size_t>(got)))
            break;
    }
#endif
    if (!sha.Final(hex))
    {
        ReleaseLock(rec);
        why = "native_hash_failed";
        return false;
    }
    return true;
}


// Never recursively remove a module directory. Faulted/in-flight images retain their
// shadow for process lifetime, exactly like the existing retained-image policy.
void CleanupShadow(NativeModuleRecord& rec) noexcept
{
#if defined(_WIN32)
    if (rec.osHandle || rec.shadowDirectory.empty()) return;
    std::error_code ec;
    // Assigned only after successful CREATE_NEW; no allocating path manipulation in noexcept cleanup.
    if (!rec.shadowFile.empty())
        fs::remove(rec.shadowFile, ec);
    if (rec.shadowDirectoryLock)
        CloseHandle(reinterpret_cast<HANDLE>(rec.shadowDirectoryLock));
    rec.shadowDirectoryLock = 0;
    fs::remove(rec.shadowDirectory, ec); // empty owned directory only; never remove_all
    rec.shadowFile.clear(); rec.shadowDirectory.clear();
#else
    (void)rec;
#endif
}

#if defined(_WIN32)
bool PrepareShadow(NativeModuleRecord& rec, std::string& why)
{
    // Original remains deny-write locked until the copy is complete. Then the COPY
    // is independently opened deny-write/delete and SHA-checked through that handle.
    // Any replacement in the copy/open gap must still match the trusted manifest SHA.
    try
    {
        unsigned char nonce[16];
        if (RAND_bytes(nonce, sizeof(nonce)) != 1) { why="native_shadow_nonce_failed"; return false; }
        char hex[33]; static char const digits[]="0123456789abcdef";
        for (unsigned i=0;i<16;++i) { hex[2*i]=digits[nonce[i]>>4];hex[2*i+1]=digits[nonce[i]&15]; }
        hex[32]=0;
        auto directory=fs::temp_directory_path() / (std::string("aeternum-native-")+hex);
        if (!CreateDirectoryW(directory.c_str(), nullptr)) { why="native_shadow_create_failed"; return false; }
        rec.shadowDirectory=std::move(directory);
        HANDLE dir=CreateFileW(rec.shadowDirectory.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if(dir==INVALID_HANDLE_VALUE) { why="native_shadow_directory_lock_failed"; return false; }
        rec.shadowDirectoryLock=reinterpret_cast<std::uintptr_t>(dir);
        if(IsReparseOrSymlink(rec.shadowDirectory)) { why="native_shadow_directory_reparse"; return false; }
        rec.shadowDirectory=fs::canonical(rec.shadowDirectory);
        auto target=rec.shadowDirectory / rec.libraryPath.filename();
        HANDLE output=CreateFileW(target.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(output==INVALID_HANDLE_VALUE) { why="native_shadow_copy_create_failed"; return false; }
        std::unique_ptr<void,decltype(&CloseHandle)> closeOutput(output,&CloseHandle);
        rec.shadowFile=std::move(target); // noexcept ownership transfer after CREATE_NEW
        LARGE_INTEGER zero{};
        if(!rec.fileLock || !SetFilePointerEx(reinterpret_cast<HANDLE>(rec.fileLock),zero,nullptr,FILE_BEGIN))
            { why="native_shadow_source_seek_failed"; return false; }
        std::array<unsigned char,65536> bytes;
        for(;;) {
            DWORD read=0,written=0;
            if(!ReadFile(reinterpret_cast<HANDLE>(rec.fileLock),bytes.data(),DWORD(bytes.size()),&read,nullptr))
                { why="native_shadow_source_read_failed"; return false; }
            if(!read) break;
            if(!WriteFile(output,bytes.data(),read,&written,nullptr) || written!=read)
                { why="native_shadow_write_failed"; return false; }
        }
        closeOutput.reset();
        std::string digest;
        if(!LockAndHashImage(rec.shadowFile,rec,digest,why)) return false;
        if(!Sha256HexEqual(rec.librarySha256,digest)) { why="native_shadow_sha256_mismatch"; return false; }
        rec.sourceLibraryPath=rec.libraryPath;
        rec.libraryPath=rec.shadowFile;
        return true;
    }
    catch(...) { why="native_shadow_copy_exception"; return false; }
}
#endif

void UnmapImage(NativeModuleRecord& rec) noexcept
{
    if (!rec.osHandle) { CleanupShadow(rec); return; }
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(rec.osHandle));
#else
    dlclose(rec.osHandle);
#endif
    rec.osHandle = nullptr;
    rec.image = AeternumGuardImage{};
    CleanupShadow(rec);
}

#if !defined(_WIN32)
struct PhdrQuery
{
    std::uintptr_t loadBias = 0;
    char const* name = nullptr;
    std::uintptr_t lo = 0;
    std::uintptr_t hi = 0;
    bool found = false;
};

int PhdrCallback(struct dl_phdr_info* info, std::size_t, void* data)
{
    auto* q = static_cast<PhdrQuery*>(data);
    if (info->dlpi_addr != q->loadBias || !info->dlpi_name || !q->name || std::strcmp(info->dlpi_name, q->name) != 0)
        return 0;
    std::uintptr_t lo = ~static_cast<std::uintptr_t>(0), hi = 0;
    for (int i = 0; i < info->dlpi_phnum; ++i)
    {
        if (info->dlpi_phdr[i].p_type != PT_LOAD)
            continue;
        std::uintptr_t const a = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr;
        std::uintptr_t const b = a + info->dlpi_phdr[i].p_memsz;
        lo = std::min(lo, a);
        hi = std::max(hi, b);
    }
    if (hi > lo)
    {
        q->lo = lo;
        q->hi = hi;
        q->found = true;
    }
    return 1;
}
#endif

bool Fail(NativeModuleRecord& rec, std::string why)
{
    ReleaseLock(rec);
    CleanupShadow(rec);
    rec.step = NativeStep::Error;
    rec.reason = std::move(why);
    if (rec.slot >= 0)
        QuarantineSlot(rec.slot, rec.reason.c_str()); // slot Error: no crossing, hooks never go live
    return false;
}

bool RecordFault(NativeModuleRecord& rec, char const* stage)
{
    // F1 / maintainer Q6: Error, sticky, no re-entry, image intentionally leaked (never FreeLibrary after a fault).
    rec.faulted = true;
    std::uintptr_t const rva = rec.fault.address >= rec.image.base ? rec.fault.address - rec.image.base : 0;
    Fail(rec, std::string("native_fault:") + stage + ":code=" + Hex32(rec.fault.code) + ":rva=" + HexPtr(rva));
    // F6: tainted + one-shot 60 s clean shutdown. F2: the module heap blocks are never walked after an AV.
    MarkTaintedAndArmGracefulShutdown(rec.reason.c_str());
    if (rec.slot >= 0)
        AbandonSlotBlocks(rec.slot);
    return false;
}

/** A host service (HostFree / subscribe) quarantined the slot during the crossing => the module fails. */
bool CheckSlotAfterCrossing(NativeModuleRecord& rec)
{
    if (rec.slot < 0 || GetSlotState(rec.slot) != SlotState::Error)
        return true;
    char reason[kSlotReasonChars];
    GetSlotReason(rec.slot, reason, sizeof(reason));
    ReleaseLock(rec);
    rec.step = NativeStep::Error;
    rec.reason = reason[0] ? reason : "native_quarantined";
    return false;
}

bool InvokeStage(NativeModuleRecord& rec, char const* stage, AeternumGuardCtxFn fn)
{
    if (rec.faulted || rec.step == NativeStep::Error || rec.step == NativeStep::Stopped || !rec.osHandle)
        return false; // no re-entry after fault / error / unload
    if (rec.slot >= 0 && GetSlotState(rec.slot) == SlotState::Error)
        return CheckSlotAfterCrossing(rec); // quarantined from a packet thread: no re-entry either
    if (!fn)
        return true;  // optional slot
    bool const isInit = std::strcmp(stage, "on_init") == 0;
    AeternumStatus st = AETERNUM_ERR_INTERNAL;
    ++rec.inFlight;
    {
        CrossingScope scope(rec.slot); // module_self / host_alloc / subscribe resolve to THIS module only
        if (isInit)
            SetSubscribeWindow(rec.slot); // hooks may be staged only while on_init runs
        st = aeternum_guard_call_ctx(&rec.image, fn, rec.modCtx, &rec.fault);
        if (isInit)
            SetSubscribeWindow(-1);
    }
    --rec.inFlight;
    if (rec.fault.faulted)
        return RecordFault(rec, stage);
    if (!CheckSlotAfterCrossing(rec))
        return false;
    if (st != AETERNUM_OK)
        return Fail(rec, std::string("native_") + stage + "_failed:" + std::to_string(st));
    return true;
}

AeternumGuardCtxFn StageFn(NativeModuleRecord const& rec, std::string_view stage)
{
    if (stage == "on_load") return rec.ft.on_load;
    if (stage == "on_init") return rec.ft.on_init;
    if (stage == "on_start") return rec.ft.on_start;
    if (stage == "on_stop") return rec.ft.on_stop;
    if (stage == "on_shutdown") return rec.ft.on_shutdown;
    return nullptr;
}

bool ReadAndParseManifest(fs::path const& dir, std::string const& id, ModuleManifest& out, std::string& json,
                          ManifestReadStatus& rs)
{
    std::string err;
    rs = ReadModuleManifestFile(dir / fs::path(id) / kModuleManifestFileName, json, err);
    if (rs != ManifestReadStatus::Ok)
        return false;
    char perr[128]{};
    return ParseModuleManifestJson(json, out, perr, sizeof(perr)) == AETERNUM_OK;
}
} // namespace

char const* NativeStepName(NativeStep step) noexcept
{
    switch (step)
    {
        case NativeStep::Discovered: return "Discovered";
        case NativeStep::Validated:  return "Validated";
        case NativeStep::Loaded:     return "Loaded";
        case NativeStep::Active:     return "Active";
        case NativeStep::Stopped:    return "Stopped";
        case NativeStep::Error:      return "Error";
        default:                     return "?";
    }
}

void CloseNativeRecord(NativeModuleRecord& rec) noexcept
{
    ReleaseLock(rec);
    if (!rec.faulted && rec.inFlight == 0)
        UnmapImage(rec);
}

AeternumHostFT const* DefaultHostFT() noexcept { return &g_hostFT; }
void SetHostLogger(NativeLogFn log) noexcept { g_hostLog.store(log); }
bool IsNativeRuntimeTainted() noexcept { return NativeTaintedFlag(); }
void ResetNativeRuntimeTaintedForTest() noexcept { ResetGracefulShutdownForTest(); }

void NativeHostLogLine(int level, char const* message) noexcept
{
    NativeLogFn fn = g_hostLog.load();
    if (!fn || !message)
        return;
    fn(level >= 2 ? NativeLogLevel::Error : (level == 1 ? NativeLogLevel::Warn : NativeLogLevel::Info), message);
}
std::uint64_t NativeMapCallCount() noexcept { return g_mapCalls.load(); }

char const* NativeLibrarySuffix() noexcept
{
#if defined(_WIN32)
    return ".dll";
#else
    return ".so";
#endif
}

std::vector<std::string> DiscoverNativeModuleIds(fs::path const& root)
{
    std::vector<std::string> ids;
    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec)
        return ids;
    fs::directory_iterator it(root, ec);
    fs::directory_iterator const end;
    for (; !ec && it != end; it.increment(ec))
    {
        std::error_code e2;
        if (!it->is_directory(e2) || e2)
            continue;
        std::string name;
        try { name = it->path().filename().string(); } catch (...) { continue; } // non-ASCII => not a module id
        if (!IsSafeModuleId(name))
            continue;
        ModuleManifest m;
        std::string json;
        ManifestReadStatus rs = ManifestReadStatus::NotFound;
        bool const parsed = ReadAndParseManifest(root, name, m, json, rs);
        if (rs == ManifestReadStatus::NotFound)
            continue;
        if (parsed && m.runtime != ManifestRuntime::Native)
            continue; // static module manifest: M1 gate owns it
        if (!parsed && json.find("\"native\"") == std::string::npos && rs == ManifestReadStatus::Ok)
            continue; // unparsable and not native-looking: M1 gate owns it
        ids.push_back(std::move(name)); // native, or native-looking but invalid => fail closed in Validate
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool ValidateNativeModule(fs::path const& root, std::string const& id, NativeModuleRecord& rec, bool allowHotReload)
{
    rec.id = id;
    rec.step = NativeStep::Discovered;
    rec.reason.clear();
    ReleaseLock(rec);

    if (!aeternum_guard_platform_supported())
        return Fail(rec, "native_platform_unsupported");
    if (!IsSafeModuleId(id))
        return Fail(rec, "native_module_id_invalid");

    std::error_code ec;
    fs::path const rootCanon = fs::canonical(root, ec);
    if (ec)
        return Fail(rec, "native_root_unresolvable");
    fs::path const dirRaw = rootCanon / fs::path(id);
    if (IsReparseOrSymlink(dirRaw))
        return Fail(rec, "native_module_dir_reparse");
    if (!fs::is_directory(dirRaw, ec) || ec)
        return Fail(rec, "native_module_dir_missing");
    fs::path const dirCanon = fs::canonical(dirRaw, ec);
    if (ec || !PathEquals(dirCanon, dirRaw))
        return Fail(rec, "native_module_dir_escape");
    rec.moduleDir = dirCanon;

    fs::path const manifestPath = dirCanon / kModuleManifestFileName;
    if (IsReparseOrSymlink(manifestPath))
        return Fail(rec, "native_manifest_missing_or_reparse");
    std::string json, readError;
    ManifestReadStatus const rs = ReadModuleManifestFile(manifestPath, json, readError);
    if (rs == ManifestReadStatus::NotFound)
        return Fail(rec, "native_manifest_missing");
    if (rs != ManifestReadStatus::Ok)
        return Fail(rec, "native_" + readError);

    char err[128]{};
    if (ParseModuleManifestJson(json, rec.manifest, err, sizeof(err)) != AETERNUM_OK)
        return Fail(rec, std::string("native_manifest_invalid:") + err); // includes integrity_ambiguous_form
    ModuleManifest const& m = rec.manifest;
    rec.critical = m.critical;
    if (m.id != id)
        return Fail(rec, "native_manifest_id_mismatch:" + m.id);
    if (m.runtime != ManifestRuntime::Native)
        return Fail(rec, "native_runtime_not_native");
    if (m.abiVersion != AETERNUM_MODULE_ABI_VERSION)
        return Fail(rec, "native_abi_mismatch:" + std::to_string(m.abiVersion));
    if (m.loadPolicy == ManifestLoadPolicy::HotReloadable && !allowHotReload)
        return Fail(rec, "native_load_policy_unsupported:hot-reloadable");
    if (!m.hasLibrary)
        return Fail(rec, "native_library_missing");
    if (!IsSafeIntegrityRelativePath(m.library))
        return Fail(rec, "native_library_unsafe:" + m.library);
    if (!HasSuffixCaseInsensitive(m.library, NativeLibrarySuffix()))
        return Fail(rec, "native_library_suffix:" + m.library);

    // F5 / maintainer: on the native path a missing hash is Rejected (stricter than the M1 stub).
    std::vector<IntegrityFileEntry> const entries = CollectIntegrityEntries(m);
    if (entries.empty())
        return Fail(rec, "native_integrity_required");
    bool libraryListed = false;
    for (IntegrityFileEntry const& e : entries)
    {
        if (e.path.empty())
            return Fail(rec, "native_integrity_file_required");
        if (!IsSafeIntegrityRelativePath(e.path)) // no "..", no absolute, no backslash, charset allowlist
            return Fail(rec, "native_integrity_file_unsafe:" + e.path);
        if (e.path == m.library)
            libraryListed = true;
    }
    if (!libraryListed)
        return Fail(rec, "native_library_not_hashed:" + m.library);

    for (IntegrityFileEntry const& e : entries)
    {
        fs::path full = dirCanon;
        for (fs::path const& seg : fs::path(e.path))
        {
            full /= seg;
            if (IsReparseOrSymlink(full))
                return Fail(rec, "native_integrity_reparse:" + e.path);
        }
        fs::path const canon = fs::canonical(full, ec);
        if (ec)
            return Fail(rec, "native_integrity_file_missing:" + e.path);
        if (!PathEquals(canon, full))
            return Fail(rec, "native_integrity_escape:" + e.path);
        if (!fs::is_regular_file(canon, ec) || ec)
            return Fail(rec, "native_integrity_not_regular_file:" + e.path);

        std::string hex;
        if (e.path == m.library)
        {
            std::string why;
            if (!LockAndHashImage(canon, rec, hex, why))
                return Fail(rec, why + ":" + e.path);
            rec.libraryPath = canon;
            rec.librarySha256 = hex;
        }
        else
        {
            std::string herr;
            if (Sha256FileHex(canon, hex, herr) != Sha256FileStatus::Ok)
                return Fail(rec, "native_" + (herr.empty() ? std::string("integrity_hash_failed") : herr) + ":" + e.path);
        }
        if (!Sha256HexEqual(e.sha256, hex)) // constant-time over the 64 hex digits
            return Fail(rec, "native_sha256_mismatch:" + e.path);
    }

    rec.step = NativeStep::Validated;
    rec.reason.clear();
    return true;
}

bool LoadNativeModule(NativeModuleRecord& rec, AeternumHostFT const* host)
{
    if (rec.step != NativeStep::Validated || rec.libraryPath.empty())
        return Fail(rec, "native_not_validated"); // never map an unvalidated image
    if (!host)
        host = DefaultHostFT();

#if defined(_WIN32)
    std::string shadowError;
    if (!PrepareShadow(rec,shadowError)) return Fail(rec,std::move(shadowError));
#endif
    void* entrySym = nullptr;
    g_mapCalls.fetch_add(1);
#if defined(_WIN32)
    HMODULE h = LoadLibraryExW(rec.libraryPath.c_str(), nullptr,
                               LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!h)
        return Fail(rec, "native_map_failed:" + std::to_string(GetLastError()));
    rec.osHandle = h;
    ReleaseLock(rec); // the image section now pins the file content
    auto const* dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(h);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        UnmapImage(rec);
        return Fail(rec, "native_image_invalid");
    }
    auto const* nt = reinterpret_cast<IMAGE_NT_HEADERS const*>(reinterpret_cast<unsigned char const*>(h) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        UnmapImage(rec);
        return Fail(rec, "native_image_invalid");
    }
    rec.image.base = reinterpret_cast<std::uintptr_t>(h);
    rec.image.end = rec.image.base + nt->OptionalHeader.SizeOfImage;
    entrySym = reinterpret_cast<void*>(GetProcAddress(h, "aeternum_module_entry"));
#else
    void* h = dlopen(rec.libraryPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h)
    {
        char const* e = dlerror();
        return Fail(rec, std::string("native_map_failed:") + (e ? e : "?"));
    }
    rec.osHandle = h;
    {
        struct stat a{}, b{};
        int const fd = rec.fileLock ? static_cast<int>(rec.fileLock - 1) : -1;
        bool const same = fd >= 0 && fstat(fd, &a) == 0 && stat(rec.libraryPath.c_str(), &b) == 0
            && a.st_dev == b.st_dev && a.st_ino == b.st_ino;
        ReleaseLock(rec);
        if (!same)
        {
            UnmapImage(rec);
            return Fail(rec, "native_image_swapped");
        }
    }
    struct link_map* lm = nullptr;
    if (dlinfo(h, RTLD_DI_LINKMAP, &lm) != 0 || !lm)
    {
        UnmapImage(rec);
        return Fail(rec, "native_image_invalid");
    }
    PhdrQuery q;
    q.loadBias = static_cast<std::uintptr_t>(lm->l_addr);
    q.name = lm->l_name;
    dl_iterate_phdr(&PhdrCallback, &q);
    if (!q.found)
    {
        UnmapImage(rec);
        return Fail(rec, "native_image_invalid");
    }
    rec.image.base = q.lo;
    rec.image.end = q.hi;
    entrySym = dlsym(h, "aeternum_module_entry");
#endif
    if (!entrySym)
    {
        UnmapImage(rec); // clean load: no module code ran through a crossing
        return Fail(rec, "native_entry_missing");
    }
    if (!InImage(rec.image, entrySym))
    {
        UnmapImage(rec);
        return Fail(rec, "native_entry_outside_image");
    }

    auto const entry = reinterpret_cast<AeternumGuardEntryFn>(entrySym);
    AeternumModuleEntryResult out;
    std::memset(&out, 0, sizeof(out));
    if (rec.slot >= 0)
        SetSlotImage(rec.slot, rec.image, nullptr); // hooks / HostAlloc validate against this image
    AeternumStatus st = AETERNUM_ERR_INTERNAL;
    ++rec.inFlight;
    {
        CrossingScope scope(rec.slot);
        st = aeternum_guard_call_entry(&rec.image, entry, AETERNUM_MODULE_ABI_VERSION, host, &out, &rec.fault);
    }
    --rec.inFlight;
    if (rec.fault.faulted)
        return RecordFault(rec, "entry");
    if (!CheckSlotAfterCrossing(rec))
    {
        UnmapImage(rec); // policy violation, no fault: the image may be released
        return false;
    }

    auto reject = [&rec](std::string why) {
        UnmapImage(rec); // no fault: design 2.7 => image released
        return Fail(rec, std::move(why));
    };
    if (st != AETERNUM_OK)
        return reject("native_entry_failed:" + std::to_string(st));
    if (!aeternum_abi_compatible(out.info.abi_version, AETERNUM_MODULE_ABI_VERSION))
        return reject("native_abi_mismatch:" + std::to_string(out.info.abi_version));
    if (out.ft.struct_size < AETERNUM_MODULE_FT_V1_BASE_SIZE || out.ft.struct_size > AETERNUM_FT_MAX_STRUCT_SIZE)
        return reject("native_ft_struct_size:" + std::to_string(out.ft.struct_size));
    std::string infoId;
    if (!CopyImageString(rec.image, out.info.id, 128u, infoId))
        return reject("native_info_id_invalid");
    if (infoId != rec.id)
        return reject("native_info_id_mismatch:" + infoId);
    if (out.info.load_policy != static_cast<std::uint32_t>(rec.manifest.loadPolicy))
        return reject("native_load_policy_mismatch:" + std::to_string(out.info.load_policy));
    if ((out.info.critical != 0u) != rec.manifest.critical)
        return reject("native_critical_mismatch");

    rec.moduleFtStructSize = out.ft.struct_size;
    std::memset(&rec.ft, 0, sizeof(rec.ft));
    std::memcpy(&rec.ft, &out.ft, std::min<std::size_t>(out.ft.struct_size, sizeof(rec.ft)));
    rec.ft.struct_size = static_cast<std::uint32_t>(std::min<std::size_t>(out.ft.struct_size, sizeof(rec.ft)));
    void const* const fns[] = {
        reinterpret_cast<void const*>(rec.ft.on_load), reinterpret_cast<void const*>(rec.ft.on_init),
        reinterpret_cast<void const*>(rec.ft.on_start), reinterpret_cast<void const*>(rec.ft.on_stop),
        reinterpret_cast<void const*>(rec.ft.on_shutdown), reinterpret_cast<void const*>(rec.ft.metrics_observe) };
    for (void const* fn : fns)
        if (fn && !InImage(rec.image, fn))
            return reject("native_ft_pointer_outside_image"); // FT may only point into the module (F4.8)
    rec.modCtx = out.mod_ctx;
    if (rec.slot >= 0)
        SetSlotImage(rec.slot, rec.image, rec.modCtx);
    rec.step = NativeStep::Loaded;
    return true;
}

NativeModuleRecord const* NativeModuleRuntime::Find(std::string_view id) const noexcept
{
    for (auto const& r : _records)
        if (r.id == id)
            return &r;
    return nullptr;
}

bool NativeModuleRuntime::InvokeLifecycleForTest(std::string_view id, char const* stage)
{
    NativeReadLease lease; if (!lease) return false;
    for (auto& r : _records)
        if (r.id == id)
            return InvokeStage(r, stage, StageFn(r, stage));
    return false;
}

NativeBootResult NativeModuleRuntime::Boot(ModuleManager& mgr, NativeRuntimeOptions const& options)
{
    NativeBootResult res;
    Shutdown();
    if (_shutdownPending) { res.abortStartup=true;res.message="native_shutdown_pending";return res; }
    _options=options;_reloadStats={};_syncThread={};
    _records.clear();
    _activeOrder.clear();
    _log = options.log;
    _mgr = &mgr;
    SetHostLogger(options.log);
    // Blocco2: one boot cycle = fresh module slots (generation bump) + empty, unfrozen hook table.
    ResetModuleSlots();
    BeginHookBootCycle();
    AeternumHostFT const* host = options.hostOverride ? options.hostOverride : DefaultHostFT();

    std::error_code ec;
    if (options.manifestDir.empty() || !fs::is_directory(options.manifestDir, ec) || ec)
    {
        PublishAndFreezeHooks(); // nothing staged: table frozen empty, fast path stays count == 0
        return res; // zero native modules => vanilla boot
    }

    // Discovered: file seen, nothing mapped.
    for (std::string& id : DiscoverNativeModuleIds(options.manifestDir))
    {
        if (mgr.Find(id))
        {
            Emit(_log, NativeLogLevel::Warn, "Native module " + id + " skipped: id already owned by a static module");
            continue;
        }
        ModuleManifest peek;
        std::string json;
        ManifestReadStatus rs = ManifestReadStatus::NotFound;
        bool const critical = ReadAndParseManifest(options.manifestDir, id, peek, json, rs) && peek.critical;
        if (!mgr.Discover(id, critical))
            continue;
        NativeModuleRecord rec;
        rec.id = std::move(id);
        rec.critical = critical;
        _records.push_back(std::move(rec));
        ++res.discovered;
    }

    for (std::size_t i = 0; i < _records.size(); ++i)
    {
        NativeModuleRecord& rec = _records[i];
        auto failed = [&]() {
            RetractStagedHooks(rec.slot); // a module that does not reach Active never has live hooks
            mgr.MarkError(rec.id, rec.reason);
            ++res.failed;
            Emit(_log, NativeLogLevel::Error, "Native module " + rec.id + " " + (rec.faulted ? "FAULT CONTAINED" : "REJECTED")
                + " (" + rec.reason + ")" + (rec.critical ? " [critical]" : " - server continues"));
            if (rec.critical)
            {
                res.abortStartup = true;
                res.message = "critical native module '" + rec.id + "' failed: " + rec.reason;
            }
        };

        // Validated: manifest + canonical paths + SHA-256 of every file. No LoadLibrary/dlopen before this.
        if (!ValidateNativeModule(options.manifestDir, rec.id, rec, options.hotReload))
        {
            failed();
            if (res.abortStartup) break;
            continue;
        }
        mgr.MarkValidated(rec.id);

        // Blocco2: slot = identity for crossings (module_self), HostAlloc cookies and hook entries.
        rec.slot = ClaimModuleSlot(rec.id.c_str());
        if (rec.slot < 0)
        {
            Fail(rec, "native_slot_exhausted");
            failed();
            if (res.abortStartup) break;
            continue;
        }

        // Loaded: OS mapping of the hashed path + entry accepted. Hooks are NOT published (no hook table in Blocco1).
        if (!LoadNativeModule(rec, host))
        {
            failed();
            if (res.abortStartup) break;
            continue;
        }
        mgr.MarkLoaded(rec.id);

        if (!InvokeStage(rec, "on_load", rec.ft.on_load) || !InvokeStage(rec, "on_init", rec.ft.on_init)
            || !InvokeStage(rec, "on_start", rec.ft.on_start))
        {
            failed();
            if (res.abortStartup) break;
            continue;
        }
        rec.step = NativeStep::Active;
        SetSlotState(rec.slot, SlotState::Active); // its staged hooks go live at publication (epoch of this boot)
        mgr.MarkActive(rec.id);
        _activeOrder.push_back(i);
        ++res.active;
        Emit(_log, NativeLogLevel::Info, "Native module " + rec.id + " Active (sha256 " + rec.librarySha256 + ")");
    }
    // Publish + freeze: from here on no module can subscribe (on_init is over for everybody).
    res.hooksPublished = PublishAndFreezeHooks();
    _settledHashes.assign(_records.size(), {});
    if(options.hotReload) {
        _watcher=std::make_unique<NativeFileWatcher>();
        if(!_watcher->Start(options.manifestDir)) {
            _watcher.reset();Emit(_log,NativeLogLevel::Warn,"Native hot-reload watcher unavailable (Windows events required)");
        }
    }
    ReportPendingQuarantines();
    if (IsNativeRuntimeTainted())
        Emit(_log, NativeLogLevel::Error, "Native runtime: process TAINTED by a contained module fault - "
             "graceful shutdown armed (policy F6/Q6: " + std::to_string(kTaintedGracefulShutdownSeconds)
             + " s, issued from the world thread)");
    return res;
}

void NativeModuleRuntime::SyncSlotStates()
{
    for (auto& rec : _records)
    {
        if (rec.slot < 0 || rec.step == NativeStep::Error || rec.step == NativeStep::Stopped)
            continue;
        if (GetSlotState(rec.slot) != SlotState::Error)
            continue;
        char reason[kSlotReasonChars];
        GetSlotReason(rec.slot, reason, sizeof(reason));
        rec.step = NativeStep::Error;
        rec.reason = reason[0] ? reason : "native_quarantined";
        if (rec.reason.rfind("native_fault:", 0) == 0)
            rec.faulted = true; // contained fault in a packet hook: no on_stop, no FreeLibrary
        if (_mgr)
            _mgr->MarkError(rec.id, rec.reason);
    }
}

int NativeModuleRuntime::SlotOf(std::string_view id) const noexcept
{
    NativeModuleRecord const* r = Find(id);
    return r ? r->slot : -1;
}

NativeModuleRuntime::~NativeModuleRuntime(){Shutdown();}
bool NativeModuleRuntime::WaitForFileChangeForTest(std::chrono::milliseconds timeout){return _watcher && _watcher->WaitDirty(timeout);}
bool NativeModuleRuntime::SyncPoint() {
 if(_shutdownPending){Shutdown();return false;}
 SyncSlotStates();
 if(!_watcher)return false;
 auto thread=std::this_thread::get_id();if(_syncThread==std::thread::id{})_syncThread=thread;
 if(thread!=_syncThread)return false;
 _reloadPending=_watcher->TakeDirty() || _reloadPending;
 if(!_reloadPending)return false;
 bool waiting=false,applied=false;
 for(std::size_t i=0;i<_records.size();++i) {
  auto& old=_records[i];if(old.step!=NativeStep::Active || old.manifest.loadPolicy!=ManifestLoadPolicy::HotReloadable)continue;
  NativeModuleRecord candidate;candidate.id=old.id;
  struct CandidateClose {NativeModuleRecord& r;~CandidateClose(){CloseNativeRecord(r);}} cleanup{candidate};
  if(!ValidateNativeModule(_options.manifestDir,old.id,candidate,true) || candidate.manifest.loadPolicy!=ManifestLoadPolicy::HotReloadable || candidate.critical!=old.critical) {
   ++_reloadStats.rejected;_settledHashes[i].clear();CancelNativePause();
   Emit(_log,NativeLogLevel::Warn,"Native reload candidate rejected before teardown: "+old.id+":"+candidate.reason);continue;
  }
  if(candidate.librarySha256==old.librarySha256)continue;
  // Two global ticks see identical fully validated/pinned candidate bytes. The
  // source lock survives through shadow copying. Never compute-and-trust a hash.
  if(_settledHashes[i]!=candidate.librarySha256){_settledHashes[i]=candidate.librarySha256;waiting=true;CancelNativePause();continue;}
  if(!TryNativeWrite()){++_reloadStats.deferred;return false;}
  NativeWriteScope exclusive;
  struct TransitionGuard {
   NativeModuleRecord& old;bool committed=false,cleanStop=false;
   ~TransitionGuard() noexcept {
    if(committed)return;
    RetractStagedHooks(old.slot);old.ft={};old.modCtx=nullptr;old.step=NativeStep::Error;
    QuarantineSlot(old.slot,"native_reload_exception");
    if(cleanStop){ReclaimSlotBlocks(old.slot);CloseNativeRecord(old);} // only known-cooperative teardown
    PublishAndFreezeHooks(); // never reopen admission onto a freed old function pointer
   }
  } transition{old};
  PrepareSlotHookReload(old.slot);SetSlotState(old.slot,SlotState::Stopping);
  bool stopped=InvokeStage(old,"on_stop",old.ft.on_stop) && InvokeStage(old,"on_shutdown",old.ft.on_shutdown);
  if(!stopped || old.faulted) {
   old.step=NativeStep::Error;old.ft={};old.modCtx=nullptr;QuarantineSlot(old.slot,"native_reload_teardown_failed");
   AbandonSlotBlocks(old.slot);if(_mgr)_mgr->MarkError(old.id,"native_reload_teardown_failed");
   ++_reloadStats.rejected;PublishAndFreezeHooks();transition.committed=true;continue;
  }
  transition.cleanStop=true;
  auto leaks=GetHostAllocStats().leakedBlocks;ReclaimSlotEphemeral(old.slot);
  if(GetHostAllocStats().leakedBlocks!=leaks) {
   old.ft={};old.modCtx=nullptr;old.step=NativeStep::Error;QuarantineSlot(old.slot,"native_reload_corrupt_ephemeral");
   AbandonSlotBlocks(old.slot);++_reloadStats.rejected;PublishAndFreezeHooks();transition.committed=true;continue;
  }
  UnmapImage(old); // irreversible cooperative teardown: no rollback promise
  candidate.slot=old.slot;
  bool okay=RenewSlotForReload(old.slot);
  if(okay)okay=LoadNativeModule(candidate,_options.hostOverride?_options.hostOverride:DefaultHostFT());
  if(okay)okay=InvokeStage(candidate,"on_load",candidate.ft.on_load) && InvokeStage(candidate,"on_init",candidate.ft.on_init) && InvokeStage(candidate,"on_start",candidate.ft.on_start);
  if(okay) {
   candidate.step=NativeStep::Active;SetSlotState(candidate.slot,SlotState::Active);++_reloadStats.applied;applied=true;
  } else {
   RetractStagedHooks(candidate.slot);candidate.step=NativeStep::Error;
   if(candidate.reason.empty())candidate.reason="native_reload_generation_or_init_failed";
   if(!candidate.faulted){ReclaimSlotBlocks(candidate.slot);CloseNativeRecord(candidate);}
   else AbandonSlotBlocks(candidate.slot);
   candidate.ft={};candidate.modCtx=nullptr;QuarantineSlot(candidate.slot,candidate.reason.c_str());if(_mgr)_mgr->MarkError(candidate.id,candidate.reason);
   ++_reloadStats.rejected;
  }
  old=std::move(candidate);
  // Sole ownership transferred, including shadow directory lock and OS handle.
  candidate.osHandle=nullptr;candidate.fileLock=0;candidate.shadowDirectoryLock=0;
  candidate.shadowDirectory.clear();candidate.shadowFile.clear();candidate.inFlight=0;
  PublishAndFreezeHooks();transition.committed=true;_settledHashes[i].clear();
 }
 _reloadPending=waiting;return applied;
}

void NativeModuleRuntime::Shutdown()
{
    ShutdownEcs(); // join readers/free core HostAlloc before any module-slot reset or reclamation
    if(_watcher){_watcher->Stop();_watcher.reset();}
    _shutdownPending=true;
    if(!TryNativeWrite())return;
    NativeWriteScope exclusive;
    _shutdownPending=false;_reloadPending=false;
    // Root admission already closed and drained: unpublish, then cooperative teardown.
    // Pending shutdown never waits on a callback which might need the same tick.
    UnpublishHooks();
    SyncSlotStates();
    for (auto it = _activeOrder.rbegin(); it != _activeOrder.rend(); ++it)
    {
        NativeModuleRecord& rec = _records[*it];
        if (rec.step != NativeStep::Active)
        {
            if (rec.slot >= 0 && rec.faulted)
                AbandonSlotBlocks(rec.slot);
            continue;
        }
        SetSlotState(rec.slot, SlotState::Stopping);
        if (InvokeStage(rec, "on_stop", rec.ft.on_stop))
            InvokeStage(rec, "on_shutdown", rec.ft.on_shutdown);
        if (!rec.faulted && rec.step == NativeStep::Active && rec.inFlight == 0 && GetSlotInFlight(rec.slot) == 0)
        {
            std::uint32_t const reclaimed = ReclaimSlotBlocks(rec.slot); // leftovers of a CLEAN module only
            if (reclaimed)
                Emit(_log, NativeLogLevel::Warn, "Native module " + rec.id + " leaked " + std::to_string(reclaimed)
                     + " host_alloc block(s): reclaimed by the host after on_shutdown");
            UnmapImage(rec); // F3: only clean, no prior fault, in_flight == 0
            rec.step = NativeStep::Stopped;
            SetSlotState(rec.slot, SlotState::Stopped);
        }
        else
        {
            if (rec.slot >= 0 && rec.faulted)
                AbandonSlotBlocks(rec.slot);
            Emit(_log, NativeLogLevel::Warn, "Native module " + rec.id + " image kept mapped at shutdown (" + rec.reason + ")");
        }
    }
    _activeOrder.clear();
    for (auto& rec : _records)
        ReleaseLock(rec); // Error records: image (if any) intentionally leaked; only the file lock is closed
    ReportPendingQuarantines();
}

} // namespace aeternum::sdk::internal::native
