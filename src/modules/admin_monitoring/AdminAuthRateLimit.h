/*
 * Aeternum Admin M8 — sliding-window rate limit for failed auth (401).
 * Header-only, Catch2-tested. Loopback-only server still benefits (local abuse / scripts).
 */
#ifndef AETERNUM_ADMIN_AUTH_RATE_LIMIT_H
#define AETERNUM_ADMIN_AUTH_RATE_LIMIT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace AdminMonitoring
{
class AuthFailureRateLimiter
{
public:
    AuthFailureRateLimiter(std::size_t maxFailures = 10, std::chrono::seconds window = std::chrono::seconds(60))
        : _maxFailures(maxFailures ? maxFailures : 10), _window(window) { }

    void Configure(std::size_t maxFailures, std::chrono::seconds window)
    {
        std::lock_guard<std::mutex> g(_mutex);
        _maxFailures = maxFailures ? maxFailures : 10;
        _window = window.count() > 0 ? window : std::chrono::seconds(60);
    }

    /// Returns true if the key is currently blocked (should respond 429).
    bool IsBlocked(std::string const& key, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        std::lock_guard<std::mutex> g(_mutex);
        Prune(key, now);
        auto it = _hits.find(key);
        if (it == _hits.end())
            return false;
        return it->second.size() >= _maxFailures;
    }

    void RecordFailure(std::string const& key, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        std::lock_guard<std::mutex> g(_mutex);
        Prune(key, now);
        _hits[key].push_back(now);
    }

    std::size_t FailureCount(std::string const& key, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        std::lock_guard<std::mutex> g(_mutex);
        Prune(key, now);
        auto it = _hits.find(key);
        return it == _hits.end() ? 0 : it->second.size();
    }

private:
    void Prune(std::string const& key, std::chrono::steady_clock::time_point now)
    {
        auto it = _hits.find(key);
        if (it == _hits.end())
            return;
        auto& q = it->second;
        while (!q.empty() && (now - q.front()) > _window)
            q.pop_front();
        if (q.empty())
            _hits.erase(it);
    }

    std::mutex _mutex;
    std::size_t _maxFailures;
    std::chrono::seconds _window;
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> _hits;
};
}
#endif
