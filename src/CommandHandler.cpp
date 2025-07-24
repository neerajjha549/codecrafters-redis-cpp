#include "CommandHandler.hpp"

#include <algorithm>
#include <cctype>

// Convert string to uppercase (case-insensitive matching)
std::string to_upper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

std::string CommandHandler::handle(const std::string &raw_input) {
  std::vector<std::string> parts;
  if (!RESP::parse(raw_input, parts)) {
    return RESP::error("invalid RESP format");
  }

  if (parts.empty()) return RESP::error("empty command");

  std::string cmd = to_upper(parts[0]);

  if (cmd == "PING") {
    return RESP::simple("PONG");
  } else if (cmd == "ECHO") {
    if (parts.size() < 2)
      return RESP::error("wrong number of arguments for 'echo'");
    return RESP::bulk(parts[1]);
  } else if (cmd == "SET") {
    if (parts.size() < 3)
      return RESP::error("wrong number of arguments for 'set'");

    std::string key = parts[1];
    std::string value = parts[2];
    long long px = -1;

    // Check for optional PX argument
    if (parts.size() == 5) {
      std::string option = to_upper(parts[3]);
      if (option != "PX") {
        return RESP::error("syntax error");
      }
      try {
        px = std::stoll(parts[4]);
        if (px < 0) return RESP::error("PX value must be non-negative");
      } catch (...) {
        return RESP::error("value is not an integer or out of range");
      }
    } else if (parts.size() != 3) {
      return RESP::error("syntax error");
    }

    Store::set(key, value, px);
    return RESP::simple("OK");
  } else if (cmd == "GET") {
    if (parts.size() != 2)
      return RESP::error("wrong number of arguments for 'get'");

    std::string value;
    if (Store::get(parts[1], value)) {
      return RESP::bulk(value);
    } else {
      return RESP::null_bulk();
    }
  } else if (cmd == "RPUSH") {
    if (parts.size() < 3) {
      return RESP::error("wrong number of arguments for 'rpush'");
    }

    std::string key = parts[1];
    std::vector<std::string> values(
        parts.begin() + 2, parts.end());  // Collect all elements after the key

    int result = Store::rpush(key, values);
    if (result == -1) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    }

    return RESP::integer(result);
  } else if (cmd == "LRANGE") {
    if (parts.size() != 4) {
      return RESP::error("wrong number of arguments for 'lrange'");
    }

    std::string key = parts[1];
    int start, end;
    try {
      start = std::stoi(parts[2]);
      end = std::stoi(parts[3]);
    } catch (...) {
      return RESP::error("invalid index");
    }

    std::vector<std::string> result;
    bool ok = Store::lrange(key, start, end, result);
    if (!ok) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    }

    return RESP::array(result);
  } else if (cmd == "LPUSH") {
    if (parts.size() < 3) {
      return RESP::error("wrong number of arguments for 'lpush'");
    }

    std::string key = parts[1];
    std::vector<std::string> values(parts.begin() + 2, parts.end());

    int result = Store::lpush(key, values);
    if (result == -1) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    }

    return RESP::integer(result);
  } else if (cmd == "LLEN") {
    if (parts.size() != 2) {
      return RESP::error("wrong number of arguments for 'llen'");
    }

    std::string key = parts[1];
    bool valid_type = true;
    int len = Store::llen(key, valid_type);

    if (!valid_type) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    }

    return RESP::integer(len);
  } else if (cmd == "LPOP") {
    if (parts.size() < 2 || parts.size() > 3) {
      return RESP::error("wrong number of arguments for 'lpop'");
    }

    std::string key = parts[1];
    int count = 1;

    if (parts.size() == 3) {
      try {
        count = std::stoi(parts[2]);
        if (count < 0) {
          return RESP::error("value is out of range");
        }
      } catch (...) {
        return RESP::error("invalid count");
      }
    }

    std::vector<std::string> removed;
    bool ok = Store::lpop(key, count, removed);

    if (!ok) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    }

    if (parts.size() == 3) {
      return RESP::array(removed);  // RESP array for multiple pop
    } else {
      return removed.empty() ? RESP::null_bulk()
                             : RESP::bulk(removed[0]);  // Bulk string or null
    }
  } else if (cmd == "BLPOP") {
    if (parts.size() != 3)
      return RESP::error("wrong number of arguments for 'blpop'");

    std::string key = parts[1];
    int timeout = 0;

    try {
      timeout = std::stoi(parts[2]);
    } catch (...) {
      return RESP::error("invalid timeout");
    }

    std::vector<std::string> popped;
    bool ok = Store::lpop(key, 1, popped);

    if (ok && !popped.empty()) {
      return RESP::array({key, popped[0]});
    } else if (!ok) {
      return RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
    } else {
      return "__BLOCK__";  // special signal to server
    }
  } else {
    return RESP::error("unknown command");
  }
}
