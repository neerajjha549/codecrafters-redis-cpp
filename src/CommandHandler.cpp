#include "CommandHandler.hpp"
#include "RESP.hpp"
#include "Store.hpp"
#include <algorithm>

std::string to_upper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string CommandHandler::handle(const std::string& raw_input) {
    std::vector<std::string> parts;
    if (!RESP::parse(raw_input, parts)) {
        return RESP::error("invalid RESP format");
    }

    if (parts.empty()) return RESP::error("empty command");
    std::string cmd = to_upper(parts[0]);

    if (cmd == "PING") {
        return RESP::simple("PONG");
    } else if (cmd == "ECHO") {
        if (parts.size() < 2) return RESP::error("wrong number of arguments for 'echo'");
        return RESP::bulk(parts[1]);
    } else if (cmd == "SET") {
        if (parts.size() != 3) return RESP::error("wrong number of arguments for 'set'");
        Store::set(parts[1], parts[2]);
        return RESP::simple("OK");
    } else if (cmd == "GET") {
        if (parts.size() != 2) return RESP::error("wrong number of arguments for 'get'");
        std::string value;
        if (Store::get(parts[1], value)) {
            return RESP::bulk(value);
        } else {
            return RESP::null_bulk();
        }
    } else {
        return RESP::error("unknown command");
    }
}
