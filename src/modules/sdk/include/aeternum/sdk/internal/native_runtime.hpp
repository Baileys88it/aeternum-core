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
 * Aeternum IE-M3 Blocco1 - INTERNAL OS-level native module loader (.dll / .so), on-boot-only.
 *
 * Order (F5, fail-closed), driven against the M1 ModuleManager FSM:
 *   Discovered : <manifestDir>/<id>/module.manifest.json with "runtime":"native" seen. No mapping.
 *   Validated  : manifest + canonical paths (no "..", no absolute, no symlink/junction escaping the root)
 *                + SHA-256 (EVP) of EVERY integrity file, constant-time compare; the native image must be
 *                one of the hashed files; missing hash => Rejected. NO LoadLibrary/dlopen before this point.
 *   Loaded     : LoadLibraryExW(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32) /
 *                dlopen(RTLD_NOW|RTLD_LOCAL) of the SAME canonical path just hashed (Windows: file kept
 *                open deny-write from hash to map); only `aeternum_module_entry` resolved; entry called
 *                inside the F1 guard; abi/struct_size/id/load_policy/critical checked. No hooks published.
 *   Active     : on_load -> on_init -> on_start through the guard. Blocco2: packet hooks may be staged ONLY while
 *                on_init runs; they are published (and the hook table frozen) once, at the end of Boot().
 *   Error      : any failure. Rejected = Error with reason "native_..."; never FATAL. A contained fault
 *                leaves the image mapped (no FreeLibrary), no re-entry, process flagged "tainted".
 * FreeLibrary/dlclose only on clean shutdown, no prior fault, in_flight == 0.
 *
 * Pure std + OpenSSL + rapidjson (via manifest.hpp). No Trinity types. Boot/shutdown only (allocates).
 * Build switch: CMake AETERNUM_NATIVE_MODULE_RUNTIME (default OFF) wires this into ModuleHost.
 */
#ifndef AETERNUM_SDK_INTERNAL_NATIVE_RUNTIME_HPP
#define AETERNUM_SDK_INTERNAL_NATIVE_RUNTIME_HPP

#include "aeternum/abi/AeternumABI.h"
#include "aeternum/sdk/internal/module_manager.hpp"
#include "aeternum/sdk/internal/native_guard.h"
#include "aeternum/sdk/internal/native_reload.hpp"
#include <thread>
#include "aeternum/sdk/manifest.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace aeternum::sdk::internal::native
{

enum class NativeStep : unsigned char
{
    Discovered = 0,
    Validated,
    Loaded,
    Active,
    Stopped, // clean shutdown, image released
    Error    // rejected or failed; reason says which
};

char const* NativeStepName(NativeStep step) noexcept;

enum class NativeLogLevel : unsigned char
{
    Info = 0,
    Warn,
    Error
};

using NativeLogFn = void (*)(NativeLogLevel level, char const* message);

struct NativeModuleRecord
{
    std::string id;
    bool critical = false;
    NativeStep step = NativeStep::Discovered;
    std::string reason;                    // "native_<cause>[:detail]" when step == Error
    ModuleManifest manifest;
    std::filesystem::path moduleDir;       // canonical <root>/<id>
    std::filesystem::path libraryPath;     // canonical path that was hashed AND mapped
    std::string librarySha256;
    std::filesystem::path sourceLibraryPath; // original is never mapped on Windows
    std::filesystem::path shadowDirectory;   // exclusively created, non-recursive cleanup only
    std::filesystem::path shadowFile;
    std::uintptr_t shadowDirectoryLock = 0;

    // OS / ABI state (host side only)
    void* osHandle = nullptr;              // HMODULE / dlopen handle
    std::uintptr_t fileLock = 0;           // Windows: deny-write HANDLE held from hash to map; POSIX: fd + 1
    AeternumModuleFT ft{};                 // copy of the module FT (min(struct_size, sizeof))
    std::uint32_t moduleFtStructSize = 0;  // struct_size reported by the module
    void* modCtx = nullptr;
    AeternumGuardImage image{};            // [base, end) for the fault filter
    AeternumGuardFault fault{};            // last contained fault
    bool faulted = false;                  // sticky: no re-entry, no FreeLibrary
    std::uint32_t inFlight = 0;            // crossings in progress (boot thread only in Blocco1)
    int slot = -1;                         // Blocco2: module slot (native_host.hpp); -1 = standalone Validate/Load
};

struct NativeRuntimeOptions
{
    std::filesystem::path manifestDir;           // same root as ManifestBootOptions::manifestDir
    AeternumHostFT const* hostOverride = nullptr; // Catch2 only; nullptr => DefaultHostFT()
    bool hotReload = false; // explicit opt-in, Windows native events only
    NativeLogFn log = nullptr;                    // host logger; nullptr => silent
};

struct NativeBootResult
{
    bool abortStartup = false; // a critical native module failed (M1 critical semantics)
    std::string message;
    std::uint32_t discovered = 0;
    std::uint32_t active = 0;
    std::uint32_t failed = 0;  // rejected + error
    std::uint32_t hooksPublished = 0; // Blocco2: packet hook entries live after the freeze
};

/** OS loader + FT lifecycle for native modules of one boot cycle. */
class NativeModuleRuntime
{
public:
    NativeModuleRuntime() = default;
    ~NativeModuleRuntime();
    // Owner/global-world thread only. Never blocks waiting for active callbacks.
    bool SyncPoint();
    bool WaitForFileChangeForTest(std::chrono::milliseconds timeout);
    bool ShutdownPending() const noexcept { return _shutdownPending; }
    struct ReloadStats {std::uint64_t applied=0,rejected=0,deferred=0;};
    ReloadStats ReloadObservations() const noexcept {return _reloadStats;}
    NativeModuleRuntime(NativeModuleRuntime const&) = delete;
    NativeModuleRuntime& operator=(NativeModuleRuntime const&) = delete;

    /** Discover -> Validate -> Load -> on_load/on_init/on_start -> Active. Ids already in `mgr` are skipped. */
    NativeBootResult Boot(ModuleManager& mgr, NativeRuntimeOptions const& options);

    /** Reverse order: on_stop/on_shutdown (guarded) then FreeLibrary only when clean. Records are kept. */
    void Shutdown();

    std::vector<NativeModuleRecord> const& Records() const noexcept { return _records; }
    NativeModuleRecord const* Find(std::string_view id) const noexcept;

    /** Test hook: re-invoke a lifecycle slot; must be refused (false, no call) after a fault/Error. */
    bool InvokeLifecycleForTest(std::string_view id, char const* stage);

    /** Blocco2: fold quarantines raised on packet / host-service paths into the records + FSM (boot/world thread). */
    void SyncSlotStates();
    /** Module slot of a record (-1 if unknown). */
    int SlotOf(std::string_view id) const noexcept;

private:
    std::vector<NativeModuleRecord> _records;
    std::vector<std::size_t> _activeOrder;
    NativeRuntimeOptions _options;
    std::unique_ptr<NativeFileWatcher> _watcher;
    std::vector<std::string> _settledHashes;
    bool _reloadPending=false, _shutdownPending=false;
    ReloadStats _reloadStats{};
    std::thread::id _syncThread{};
    NativeLogFn _log = nullptr;
    ModuleManager* _mgr = nullptr;
};

// ---- pure helpers (exposed for Catch2) ----

/** Sorted ids of <root>/<id>/module.manifest.json that declare "runtime":"native" (or fail to parse). */
std::vector<std::string> DiscoverNativeModuleIds(std::filesystem::path const& root);

/** F5 Validated step. Never maps anything. On success rec.step == Validated and the image file is locked. */
bool ValidateNativeModule(std::filesystem::path const& root, std::string const& id, NativeModuleRecord& rec, bool allowHotReload = false);

/** Loaded step: map the validated path, resolve the entry, call it guarded, check info/FT. */
bool LoadNativeModule(NativeModuleRecord& rec, AeternumHostFT const* host);

/** Release the file lock; unmap only if the record never faulted (F1/F3). For standalone Validate/Load use. */
void CloseNativeRecord(NativeModuleRecord& rec) noexcept;

/** Host FT given to native modules (struct_size = sizeof(AeternumHostFT), ABI v1 + appended fields). */
AeternumHostFT const* DefaultHostFT() noexcept;
void SetHostLogger(NativeLogFn log) noexcept;

/** Process "tainted" after any contained native fault (maintainer Q6 / F6: tainted + 60 s graceful shutdown, native_host.hpp). */
bool IsNativeRuntimeTainted() noexcept;
void ResetNativeRuntimeTaintedForTest() noexcept;

/** Number of LoadLibraryExW / dlopen calls issued by this loader (proves "no map before Validated"). */
std::uint64_t NativeMapCallCount() noexcept;

/** Platform suffix of a native image: ".dll" (Windows) / ".so" (POSIX). */
char const* NativeLibrarySuffix() noexcept;

} // namespace aeternum::sdk::internal::native

#endif /* AETERNUM_SDK_INTERNAL_NATIVE_RUNTIME_HPP */
