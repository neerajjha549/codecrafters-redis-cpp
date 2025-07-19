#pragma once
#include <string>
#include <vector>

namespace RESP {
    bool parse(const std::string& input, std::vector<std::string>& out);
    std::string bulk(const std::string& s);
    std::string simple(const std::string& s);
    std::string error(const std::string& msg);
    std::string null_bulk();
}
