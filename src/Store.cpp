#include "Store.hpp"

std::unordered_map<std::string, Store::ValueEntry> Store::db;

void Store::set(const std::string& key, const std::string& value, long long px_ms) {
    ValueEntry entry;
    entry.type = ValueType::STRING;
    entry.data = value;

    if (px_ms > 0) {
        entry.has_expiry = true;
        entry.expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(px_ms);
    }

    db[key] = entry;
}

bool Store::get(const std::string& key, std::string& value) {
    auto it = db.find(key);
    if (it == db.end()) return false;

    if (is_expired(it->second)) {
        db.erase(it);
        return false;
    }

    if (it->second.type != ValueType::STRING) return false;

    value = std::get<std::string>(it->second.data);
    return true;
}

int Store::rpush(const std::string& key, const std::vector<std::string>& values) {
    auto it = db.find(key);

    if (it == db.end() || is_expired(it->second)) {
        ValueEntry entry;
        entry.type = ValueType::LIST;
        std::deque<std::string> new_list;
        for (const auto& val : values) {
            new_list.push_back(val);  // O(1)
        }
        entry.data = std::move(new_list);
        db[key] = entry;
        return values.size();
    }

    if (it->second.type != ValueType::LIST) {
        return -1;
    }

    auto& list = std::get<std::deque<std::string>>(it->second.data);
    for (const auto& val : values) {
        list.push_back(val);  // O(1)
    }

    return list.size();
}

bool Store::lrange(const std::string& key, int start, int end, std::vector<std::string>& out) {
    auto it = db.find(key);
    out.clear();

    if (it == db.end() || is_expired(it->second)) {
        db.erase(key);
        return true;  // Return empty list
    }

    if (it->second.type != ValueType::LIST) {
        return false;  // WRONGTYPE error
    }

    const auto& list = std::get<std::deque<std::string>>(it->second.data);
    int len = static_cast<int>(list.size());

    // Convert negative indexes
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    // Clamp bounds
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start >= len || start > end) return true;
    if (end >= len) end = len - 1;

    for (int i = start; i <= end; ++i) {
        out.push_back(list[i]);
    }

    return true;
}

int Store::lpush(const std::string& key, const std::vector<std::string>& values) {
    auto it = db.find(key);

    if (it == db.end() || is_expired(it->second)) {
        ValueEntry entry;
        entry.type = ValueType::LIST;
        std::deque<std::string> new_list;
        for (const auto& val : values) {
            new_list.push_front(val);  // O(1)
        }
        entry.data = std::move(new_list);
        db[key] = entry;
        return values.size();
    }

    if (it->second.type != ValueType::LIST) {
        return -1;
    }

    auto& list = std::get<std::deque<std::string>>(it->second.data);
    for (const auto& val : values) {
        list.push_front(val);  // O(1)
    }

    return list.size();
}

int Store::llen(const std::string& key, bool& valid_type) {
    valid_type = true;

    auto it = db.find(key);
    if (it == db.end() || is_expired(it->second)) {
        db.erase(key);
        return 0;
    }

    if (it->second.type != ValueType::LIST) {
        valid_type = false;
        return 0;
    }

    const auto& list = std::get<std::deque<std::string>>(it->second.data);
    return list.size();
}

bool Store::is_expired(const ValueEntry& entry) {
    if (!entry.has_expiry) return false;
    return std::chrono::steady_clock::now() > entry.expiry;
}
