#include "MetricErrorCollector.h"
#include <algorithm>

namespace Trinity::MetricLocal
{
ErrorWarningCollector& ErrorWarningCollector::Instance()
{
    static ErrorWarningCollector inst;
    return inst;
}

void ErrorWarningCollector::SetLimits(std::size_t maxCategories, std::size_t maxMessages, std::size_t maxMessageChars)
{
    std::scoped_lock lock(_mutex);
    _maxCategories = maxCategories ? maxCategories : 1;
    _maxMessages = maxMessages ? maxMessages : 1;
    _maxMessageChars = maxMessageChars ? maxMessageChars : 32;
}

void ErrorWarningCollector::Note(uint8 level, std::string_view category, std::string_view message)
{
    // LogLevel: WARN=4 ERROR=5 FATAL=6
    if (level < 4 || level > 6)
        return;

    std::scoped_lock lock(_mutex);
    std::string cat(category.empty() ? "unknown" : category);
    std::string msg(message);
    if (msg.size() > _maxMessageChars)
        msg.resize(_maxMessageChars);
    if (level == 4)
    {
        ++_warnTotal;
        if (_warnByCat.size() < _maxCategories || _warnByCat.count(cat))
            ++_warnByCat[cat];
    }
    else if (level == 5)
    {
        ++_errorTotal;
        if (_errorByCat.size() < _maxCategories || _errorByCat.count(cat))
            ++_errorByCat[cat];
    }
    else
    {
        ++_fatalTotal;
        if (_errorByCat.size() < _maxCategories || _errorByCat.count(cat))
            ++_errorByCat[cat];
    }

    std::string key;
    key.reserve(4 + cat.size() + msg.size());
    key.push_back(char('0' + level));
    key.push_back('|');
    key.append(cat);
    key.push_back('|');
    key.append(msg);
    if (_msgCounts.size() < _maxMessages || _msgCounts.count(key))
        ++_msgCounts[key];
}

ErrorWarningCollector::Snapshot ErrorWarningCollector::BuildUnlocked() const
{
    Snapshot snap;
    snap.WarnTotal = _warnTotal;
    snap.ErrorTotal = _errorTotal;
    snap.FatalTotal = _fatalTotal;

    auto fillCats = [](std::unordered_map<std::string, uint64> const& src, std::vector<CategoryCount>& dst)
    {
        dst.reserve(src.size());
        for (auto const& [k, v] : src)
            dst.push_back(CategoryCount{ k, v });
        std::sort(dst.begin(), dst.end(),
            [](CategoryCount const& a, CategoryCount const& b)
            {
                if (a.Count != b.Count)
                    return a.Count > b.Count;
                return a.Category < b.Category;
            });
    };
    fillCats(_warnByCat, snap.WarnByCategory);
    fillCats(_errorByCat, snap.ErrorByCategory);

    snap.TopMessages.reserve(_msgCounts.size());
    for (auto const& [key, count] : _msgCounts)
    {
        // key = L|cat|msg
        if (key.size() < 3 || key[1] != '|')
            continue;
        TopMessage tm;
        tm.Level = uint8(key[0] - '0');
        auto second = key.find('|', 2);
        if (second == std::string::npos)
            continue;
        tm.Category = key.substr(2, second - 2);
        tm.Message = key.substr(second + 1);
        tm.Count = count;
        snap.TopMessages.push_back(std::move(tm));
    }
    std::sort(snap.TopMessages.begin(), snap.TopMessages.end(),
        [](TopMessage const& a, TopMessage const& b)
        {
            if (a.Count != b.Count)
                return a.Count > b.Count;
            if (a.Level != b.Level)
                return a.Level > b.Level;
            return a.Message < b.Message;
        });
    return snap;
}

ErrorWarningCollector::Snapshot ErrorWarningCollector::Peek() const
{
    std::scoped_lock lock(_mutex);
    return BuildUnlocked();
}

ErrorWarningCollector::Snapshot ErrorWarningCollector::Flush()
{
    std::scoped_lock lock(_mutex);
    Snapshot snap = BuildUnlocked();
    _warnTotal = _errorTotal = _fatalTotal = 0;
    _warnByCat.clear();
    _errorByCat.clear();
    _msgCounts.clear();
    return snap;
}

void NoteLogForDailySummary(uint8 level, std::string_view category, std::string_view message)
{
    ErrorWarningCollector::Instance().Note(level, category, message);
}
}
