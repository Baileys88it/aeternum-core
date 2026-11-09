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
 * Aeternum M1-Blocco3 - INTERNAL Module Manager FSM (header-only, Catch2-friendly).
 *
 * Canonical states (MODULE_MANAGER_DESIGN.md):
 *   Discovered -> Validated -> Loaded -> Active
 *                 \-> Error   \-> Error  \-> Error
 *   Active -> Error (runtime quarantine; recorded here, no hot-reload in M1)
 *
 * Pure std. No Trinity types: lint FAIL surface under src/modules/sdk/include.
 * Host wiring: ModuleHost.cpp HostDiscoverAndStart (boot-only). No World.cpp changes.
 * WITH_DYNAMIC_LINKING=OFF / no FreeLibrary path in M1.
 */
#ifndef AETERNUM_SDK_INTERNAL_MODULE_MANAGER_HPP
#define AETERNUM_SDK_INTERNAL_MODULE_MANAGER_HPP

#include "aeternum/sdk/internal/manifest_gate.hpp"
#include "aeternum/sdk/internal/static_runtime.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aeternum::sdk::internal
{

enum class ModuleFsmState : unsigned char
{
    Discovered = 0,
    Validated,
    Loaded,
    Active,
    Error
};

inline char const* ModuleFsmStateName(ModuleFsmState state) noexcept
{
    switch (state)
    {
        case ModuleFsmState::Discovered: return "Discovered";
        case ModuleFsmState::Validated:  return "Validated";
        case ModuleFsmState::Loaded:     return "Loaded";
        case ModuleFsmState::Active:     return "Active";
        case ModuleFsmState::Error:      return "Error";
        default:                         return "?";
    }
}

struct ModuleFsmRecord
{
    std::string id;
    bool critical = false;
    ModuleFsmState state = ModuleFsmState::Discovered;
    std::string reason; // meaningful in Error (and optionally kept after recovery is added later)
};

/**
 * In-memory FSM registry for one boot cycle.
 * Illegal transitions return false and leave state unchanged (no silent corruption).
 */
class ModuleManager
{
public:
    void Clear()
    {
        _records.clear();
    }

    std::vector<ModuleFsmRecord> const& Records() const noexcept
    {
        return _records;
    }

    std::size_t Size() const noexcept
    {
        return _records.size();
    }

    std::size_t CountInState(ModuleFsmState state) const noexcept
    {
        std::size_t n = 0;
        for (auto const& r : _records)
            if (r.state == state)
                ++n;
        return n;
    }

    ModuleFsmRecord const* Find(std::string_view id) const noexcept
    {
        for (auto const& r : _records)
            if (r.id == id)
                return &r;
        return nullptr;
    }

    /** * -> Discovered. Fails on empty id or duplicate id. */
    bool Discover(std::string id, bool critical)
    {
        if (id.empty())
            return false;
        if (Find(id))
            return false;
        ModuleFsmRecord rec;
        rec.id = std::move(id);
        rec.critical = critical;
        rec.state = ModuleFsmState::Discovered;
        _records.push_back(std::move(rec));
        return true;
    }

    /** Discovered -> Validated */
    bool MarkValidated(std::string_view id)
    {
        ModuleFsmRecord* r = FindMut(id);
        if (!r || r->state != ModuleFsmState::Discovered)
            return false;
        r->state = ModuleFsmState::Validated;
        r->reason.clear();
        return true;
    }

    /** Validated -> Loaded */
    bool MarkLoaded(std::string_view id)
    {
        ModuleFsmRecord* r = FindMut(id);
        if (!r || r->state != ModuleFsmState::Validated)
            return false;
        r->state = ModuleFsmState::Loaded;
        r->reason.clear();
        return true;
    }

    /** Loaded -> Active */
    bool MarkActive(std::string_view id)
    {
        ModuleFsmRecord* r = FindMut(id);
        if (!r || r->state != ModuleFsmState::Loaded)
            return false;
        r->state = ModuleFsmState::Active;
        r->reason.clear();
        return true;
    }

    /**
     * Discovered | Validated | Loaded | Active -> Error.
     * Already-Error stays Error but refreshes reason (last fault wins).
     */
    bool MarkError(std::string_view id, std::string reason)
    {
        ModuleFsmRecord* r = FindMut(id);
        if (!r)
            return false;
        if (r->state != ModuleFsmState::Discovered &&
            r->state != ModuleFsmState::Validated &&
            r->state != ModuleFsmState::Loaded &&
            r->state != ModuleFsmState::Active &&
            r->state != ModuleFsmState::Error)
            return false;
        r->state = ModuleFsmState::Error;
        r->reason = std::move(reason);
        return true;
    }

private:
    ModuleFsmRecord* FindMut(std::string_view id) noexcept
    {
        for (auto& r : _records)
            if (r.id == id)
                return &r;
        return nullptr;
    }

    std::vector<ModuleFsmRecord> _records;
};

/**
 * After ApplyManifestGate: rejected modules -> Error (reason from gate).
 * Leaves non-rejected modules in Discovered for the planner step.
 */
inline void ApplyGateResultsToManager(ModuleManager& mgr, std::vector<PlannedModule> const& planned,
                                      std::vector<ManifestGateResult> const& results)
{
    std::size_t const n = planned.size() < results.size() ? planned.size() : results.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        if (results[i].outcome != ManifestGateOutcome::Rejected)
            continue;
        mgr.MarkError(planned[i].id, results[i].reason.empty() ? std::string("manifest_rejected") : results[i].reason);
    }
}

/**
 * After PlanStaticModules: enabled + still Discovered -> Validated;
 * disabled + still Discovered -> Error(disableReason). Already-Error left alone.
 */
inline void ApplyPlanToManager(ModuleManager& mgr, PlanResult const& plan)
{
    for (auto const& m : plan.modules)
    {
        ModuleFsmRecord const* rec = mgr.Find(m.id);
        if (!rec)
            continue;
        if (rec->state == ModuleFsmState::Error)
            continue;
        if (m.enabled)
            mgr.MarkValidated(m.id);
        else
            mgr.MarkError(m.id, m.disableReason.empty() ? std::string("disabled") : m.disableReason);
    }
}

} // namespace aeternum::sdk::internal

#endif /* AETERNUM_SDK_INTERNAL_MODULE_MANAGER_HPP */