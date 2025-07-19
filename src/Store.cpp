#include "Store.hpp"

std::unordered_map<std::string, Store::ValueEntry> Store::db;

void Store::set(const std::string& key, const std::string& value, long long px_ms) {
    ValueEntry entry;
    entry.value = value;
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

    value = it->second.value;
    return true;
}

bool Store::is_expired(const ValueEntry& entry) {
    if (!entry.has_expiry) return false;
    return std::chrono::steady_clock::now() > entry.expiry;
}
