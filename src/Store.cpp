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

int Store::lpush(const std::string& key, const std::vector<std::string>& values) {
    auto it = db.find(key);

    if (it == db.end() || is_expired(it->second)) {
        // New list
        ValueEntry entry;
        entry.type = ValueType::LIST;
        std::vector<std::string> new_list;
        new_list.reserve(values.size());
        // Insert elements in the correct order for LPUSH:
        // The last element of 'values' should be the first in 'new_list',
        // and the first element of 'values' should be the last in 'new_list'
        // when inserting by pushing to back.
        // To achieve the correct LPUSH order (first given element at head),
        // we can reverse the input values or insert at the beginning.
        // For a new list, building it in reverse from the input values
        // and then reversing the new_list itself or inserting from rbegin()
        // to rend() for *lpush* (meaning last given value is first in the list
        // which would then be reversed) is conceptually complicated.
        // The simplest approach is to insert elements from 'values'
        // at the beginning of 'new_list' one by one, effectively prepending.
        for (const auto& val : values) {
            new_list.insert(new_list.begin(), val);
        }
        entry.data = std::move(new_list);
        db[key] = entry;
        return values.size();
    }

    if (it->second.type != ValueType::LIST) {
        return -1;
    }

    auto& list = std::get<std::vector<std::string>>(it->second.data);
    // Iterate through 'values' in forward order and insert each at the beginning of 'list'.
    // This ensures that the elements are prepended in the correct LPUSH order.
    for (const auto& val : values) {
        list.insert(list.begin(), val);
    }

    return list.size();
}

bool Store::is_expired(const ValueEntry& entry) {
    if (!entry.has_expiry) return false;
    return std::chrono::steady_clock::now() > entry.expiry;
}
