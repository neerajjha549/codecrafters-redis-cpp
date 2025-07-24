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

int Store::rpush(const std::string& key, const std::string& value) {
    auto it = db.find(key);

    if (it == db.end() || is_expired(it->second)) {
        // Create new list
        ValueEntry entry;
        entry.type = ValueType::LIST;
        entry.data = std::vector<std::string>{ value };
        db[key] = entry;
        return 1;
    }

    if (it->second.type != ValueType::LIST) {
        // For this stage, we assume key does not exist already or is a list
        return -1;
    }

    auto& vec = std::get<std::vector<std::string>>(it->second.data);
    vec.push_back(value);
    return vec.size();
}

bool Store::is_expired(const ValueEntry& entry) {
    if (!entry.has_expiry) return false;
    return std::chrono::steady_clock::now() > entry.expiry;
}
