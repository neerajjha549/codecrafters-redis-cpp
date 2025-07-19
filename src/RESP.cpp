#include "RESP.hpp"
#include <cctype>

namespace RESP {
    bool parse(const std::string& input, std::vector<std::string>& out) {
        size_t pos = 0;
        if (input[pos] != '*') return false;
        pos++;
        size_t newline = input.find("\r\n", pos);
        int count = std::stoi(input.substr(pos, newline - pos));
        pos = newline + 2;
        out.clear();
        for (int i = 0; i < count; ++i) {
            if (input[pos++] != '$') return false;
            newline = input.find("\r\n", pos);
            int len = std::stoi(input.substr(pos, newline - pos));
            pos = newline + 2;
            out.push_back(input.substr(pos, len));
            pos += len + 2;
        }
        return true;
    }

    std::string bulk(const std::string& s) {
        return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
    }

    std::string simple(const std::string& s) {
        return "+" + s + "\r\n";
    }

    std::string error(const std::string& msg) {
        return "-ERR " + msg + "\r\n";
    }

    std::string null_bulk() {
        return "$-1\r\n";
    }
}
