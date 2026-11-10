#ifndef TRINITYCORE_CRASH_DUMP_BUDGET_H
#define TRINITYCORE_CRASH_DUMP_BUDGET_H

#include "Define.h"
#include <cstdint>
#include <filesystem>
#include <string>

namespace Trinity::Crash
{
struct DumpStoreConfig
{
    std::filesystem::path Directory;          // default: <exe>/Crashes
    uint64 BudgetBytes = 256ull * 1024ull * 1024ull; // 256 MB
    uint32 RetentionDays = 30;
};

/// Disk budget + retention for crash dump / last-chance files (.dmp/.txt/.marker).
class TC_COMMON_API DumpStore
{
public:
    void Configure(DumpStoreConfig cfg);
    DumpStoreConfig const& GetConfig() const { return _cfg; }

    uint64 DirectoryBytes() const;
    uint32 FileCount() const;
    /// Delete oldest files until under budget; drop files older than retention. Returns deleted count.
    uint32 EnforceBudgetAndRetention();

    /// Safe test hook: write a small marker file (NOT a real crash). Catch2 / soak only.
    bool WriteTestMarker(std::string_view tag);

    /// Resolve default Crashes dir next to the running executable (best-effort).
    static std::filesystem::path DefaultDirectory();

private:
    DumpStoreConfig _cfg;
};

TC_COMMON_API DumpStore& GetDumpStore();
}

#endif
