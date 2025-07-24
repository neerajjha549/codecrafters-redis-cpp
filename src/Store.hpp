#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <variant>
#include <vector>

class Store {
public:
    static void set(const std::string& key, const std::string& value, long long px_ms = -1);
    static bool get(const std::string& key, std::string& value);

    static int rpush(const std::string& key, const std::vector<std::string>& values);

private:
    enum class ValueType { STRING, LIST };

    struct ValueEntry {
        std::variant<std::string, std::vector<std::string>> data;
        std::chrono::steady_clock::time_point expiry;
        bool has_expiry = false;

        ValueType type;
    };

    static std::unordered_map<std::string, ValueEntry> db;

    static bool is_expired(const ValueEntry& entry);
};
