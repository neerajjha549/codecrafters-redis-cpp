#include "Store.hpp"

std::unordered_map<std::string, std::string> Store::db;

void Store::set(const std::string& key, const std::string& value) {
    db[key] = value;
}

bool Store::get(const std::string& key, std::string& value) {
    auto it = db.find(key);
    if (it != db.end()) {
        value = it->second;
        return true;
    }
    return false;
}
