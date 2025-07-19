#pragma once
#include <string>
#include <unordered_map>

class Store {
public:
    static void set(const std::string& key, const std::string& value);
    static bool get(const std::string& key, std::string& value);
private:
    static std::unordered_map<std::string, std::string> db;
};
