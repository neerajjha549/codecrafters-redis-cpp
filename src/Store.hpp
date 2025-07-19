#pragma once
#include <unordered_map>
#include <string>
#include <chrono>

class Store {
public:
    static void set(const std::string& key, const std::string& value, long long px_ms = -1);
    static bool get(const std::string& key, std::string& value);

private:
    struct ValueEntry {
        std::string value;
        std::chrono::steady_clock::time_point expiry;
        bool has_expiry = false;
    };

    static std::unordered_map<std::string, ValueEntry> db;

    static bool is_expired(const ValueEntry& entry);
};
