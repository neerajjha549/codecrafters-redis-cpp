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
        // Create new list with multiple elements
        ValueEntry entry;
        entry.type = ValueType::LIST;
        entry.data = values;
        db[key] = entry;
        return values.size();
    }

    if (it->second.type != ValueType::LIST) {
        // Incorrect operation if the key is not a list
        return -1;
    }

    auto& list = std::get<std::vector<std::string>>(it->second.data);
    list.insert(list.end(), values.begin(), values.end());
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

    const auto& list = std::get<std::vector<std::string>>(it->second.data);
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


bool Store::is_expired(const ValueEntry& entry) {
    if (!entry.has_expiry) return false;
    return std::chrono::steady_clock::now() > entry.expiry;
}
