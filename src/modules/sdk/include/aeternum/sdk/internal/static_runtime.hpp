/*
 * Aeternum Module SDK — INTERNAL pure static planner (header-only, Catch2-friendly).
 * Not a public API. Topology, cycles, profile compatibility, critical policy.
 */
#ifndef AETERNUM_SDK_INTERNAL_STATIC_RUNTIME_HPP
#define AETERNUM_SDK_INTERNAL_STATIC_RUNTIME_HPP

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aeternum::sdk::internal
{
enum class PlanStatus : unsigned char
{
    Ok = 0,
    Cycle,
    MissingDependency,
    ProfileIncompatible,
    CriticalFailure
};

struct PlannedModule
{
    std::string id;
    bool critical = false;
    bool enabled = true;
    std::string disableReason;
    std::vector<std::string> dependencies;
    std::vector<std::string> supportedProfiles; // empty = all
};

struct PlanResult
{
    PlanStatus status = PlanStatus::Ok;
    bool abortStartup = false;
    std::string message;
    std::vector<std::string> startOrder; // dependencies first
    std::vector<PlannedModule> modules;  // all known, with enabled flags
};

inline bool ProfileAllowed(PlannedModule const& m, std::string_view activeProfile)
{
    if (m.supportedProfiles.empty())
        return true;
    if (activeProfile.empty())
        return false; // module declared a list but host has no active profile
    for (auto const& p : m.supportedProfiles)
        if (p == activeProfile)
            return true;
    return false;
}

inline PlanResult PlanStaticModules(std::vector<PlannedModule> input, std::string_view activeProfile)
{
    PlanResult out;
    out.modules = std::move(input);

    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < out.modules.size(); ++i)
        index[out.modules[i].id] = i;

    // Profile compatibility
    for (auto& m : out.modules)
    {
        if (!m.enabled)
            continue;
        if (!ProfileAllowed(m, activeProfile))
        {
            m.enabled = false;
            m.disableReason = "profile_incompatible";
            if (m.critical)
            {
                out.status = PlanStatus::ProfileIncompatible;
                out.abortStartup = true;
                out.message = "critical module '" + m.id + "' incompatible with active profile";
                return out;
            }
        }
    }

    // Missing dependencies (only among enabled)
    for (auto& m : out.modules)
    {
        if (!m.enabled)
            continue;
        for (auto const& dep : m.dependencies)
        {
            auto it = index.find(dep);
            if (it == index.end())
            {
                m.enabled = false;
                m.disableReason = "missing_dependency:" + dep;
                if (m.critical)
                {
                    out.status = PlanStatus::MissingDependency;
                    out.abortStartup = true;
                    out.message = "critical module '" + m.id + "' missing dependency '" + dep + "'";
                    return out;
                }
                break;
            }
            if (!out.modules[it->second].enabled)
            {
                m.enabled = false;
                m.disableReason = "dependency_disabled:" + dep;
                if (m.critical)
                {
                    out.status = PlanStatus::MissingDependency;
                    out.abortStartup = true;
                    out.message = "critical module '" + m.id + "' dependency disabled '" + dep + "'";
                    return out;
                }
                break;
            }
        }
    }

    // Build graph of enabled modules only
    std::vector<std::string> enabledIds;
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> indeg;
    for (auto const& m : out.modules)
    {
        if (!m.enabled)
            continue;
        enabledIds.push_back(m.id);
        indeg[m.id] = 0;
        adj[m.id] = {};
    }
    for (auto const& m : out.modules)
    {
        if (!m.enabled)
            continue;
        for (auto const& dep : m.dependencies)
        {
            // dep must start before m
            adj[dep].push_back(m.id);
            indeg[m.id]++;
        }
    }

    // Kahn topo
    std::vector<std::string> queue;
    for (auto const& id : enabledIds)
        if (indeg[id] == 0)
            queue.push_back(id);
    std::sort(queue.begin(), queue.end());

    while (!queue.empty())
    {
        std::string id = queue.front();
        queue.erase(queue.begin());
        out.startOrder.push_back(id);
        auto& outs = adj[id];
        std::sort(outs.begin(), outs.end());
        for (auto const& n : outs)
        {
            if (--indeg[n] == 0)
            {
                queue.push_back(n);
                std::sort(queue.begin(), queue.end());
            }
        }
    }

    if (out.startOrder.size() != enabledIds.size())
    {
        out.status = PlanStatus::Cycle;
        out.abortStartup = true; // cycles are always fatal for the static set
        out.message = "dependency cycle detected among enabled modules";
        out.startOrder.clear();
        return out;
    }

    out.status = PlanStatus::Ok;
    out.message = "ok";
    return out;
}

inline std::vector<std::string> ReverseOrder(std::vector<std::string> const& startOrder)
{
    std::vector<std::string> stop = startOrder;
    std::reverse(stop.begin(), stop.end());
    return stop;
}
}

#endif
